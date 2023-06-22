#include "D3D12Resources.h"
#include "D3D12Core.h"


namespace nidhog::graphics::d3d12
{
	//////////////////////////// DESCRIPTOR HEAP //////////////////////////////////
	bool descriptor_heap::initialize(u32 capacity, bool is_shader_visible)
	{
		//一般在我们创建与加载纹理时使用，可能在不同的threat下发生
		//于是使用mutex
        std::lock_guard lock{ _mutex };
        //检查heap大小是否过大
        assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
        //对于着色器来说，大小又是另一个值
        assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER &&
            capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));

        //对于depth和render target，当然着色器不可见
        if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV ||
            _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        {
            is_shader_visible = false;
        }

        release();

        //访问Main device
        ID3D12Device *const device{ core::device() };
        assert(device);

        D3D12_DESCRIPTOR_HEAP_DESC desc{};
        desc.Flags = is_shader_visible
            ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NumDescriptors = capacity;
        desc.Type = _type;
        desc.NodeMask = 0;

        //通过desc创建heap
        HRESULT hr{ S_OK };
        DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
        if (FAILED(hr)) return false;

        //已经被释放的handle，用智能指针管理
        _free_handles = std::move(std::make_unique<u32[]>(capacity));
        _capacity = capacity;
        _size = 0;

        for (u32 i{ 0 }; i < capacity; ++i) _free_handles[i] = i;
        DEBUG_OP(for (u32 i{ 0 }; i < frame_buffer_count; ++i) assert(_deferred_free_indices[i].empty()));

        _descriptor_size = device->GetDescriptorHandleIncrementSize(_type);
        _cpu_start = _heap->GetCPUDescriptorHandleForHeapStart();
        _gpu_start = is_shader_visible ?
            _heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

        return true;

	}

	void descriptor_heap::release()
	{
        assert(!_size);
        core::deferred_release(_heap);
	}

	void descriptor_heap::process_deferred_free(u32 frame_idx)
	{
        std::lock_guard lock{ _mutex };
        assert(frame_idx < frame_buffer_count);

        utl::vector<u32>& indices{ _deferred_free_indices[frame_idx] };
        if (!indices.empty())
        {
            for (auto index : indices)
            {
                --_size;
                _free_handles[_size] = index;
            }
            indices.clear();
        }
	}

	descriptor_handle descriptor_heap::allocate()
	{
        std::lock_guard lock{ _mutex };
        assert(_heap);
        assert(_size < _capacity);

        const u32 index{ _free_handles[_size] };
        const u32 offset{ index * _descriptor_size };
        ++_size;

        descriptor_handle handle;
        //分配内存并通过增加偏移量添加到heap头
        handle.cpu.ptr = _cpu_start.ptr + offset;
        if (is_shader_visible())
        {
            handle.gpu.ptr = _gpu_start.ptr + offset;
        }

        DEBUG_OP(handle.container = this);
        DEBUG_OP(handle.index = index);
        return handle;
	}

	void descriptor_heap::free(descriptor_handle& handle)
	{
        //就是很多的assert
        if (!handle.is_valid()) return;
        std::lock_guard lock{ _mutex };
        assert(_heap && _size);
        assert(handle.container == this);
        assert(handle.cpu.ptr >= _cpu_start.ptr);
        assert((handle.cpu.ptr - _cpu_start.ptr) % _descriptor_size == 0);
        assert(handle.index < _capacity);
        const u32 index{ (u32)(handle.cpu.ptr - _cpu_start.ptr) / _descriptor_size };
        assert(handle.index == index);

        //只需要得到index handle就能知道在哪一帧
        const u32 frame_idx{ core::current_frame_index() };
        _deferred_free_indices[frame_idx].push_back(index);
        //设置一个flag，让渲染器知道有待处理的删除
        core::set_deferred_releases_flag();
        handle = {};
	}
}