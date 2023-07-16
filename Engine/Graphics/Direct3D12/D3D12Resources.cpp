#include "D3D12Resources.h"
#include "D3D12Core.h"

namespace nidhog::graphics::d3d12
{
	//////////////////////////// DESCRIPTOR HEAP begin //////////////////////////////////
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
        auto *const device{ core::device() };
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
        handle.index = index;
        DEBUG_OP(handle.container = this);
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
    //////////////////////////// DESCRIPTOR HEAP end//////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////////   D3D12 BUFFER begin/////////////////////////////////////////////////////////////////////////////////
    d3d12_buffer::d3d12_buffer(d3d12_buffer_init_info info, bool is_cpu_accessible)
    {
        assert(!_buffer && info.size && info.alignment);
        _size = (u32)math::align_size_up(info.size, info.alignment);
        _buffer = d3dx::create_buffer(info.data, _size, is_cpu_accessible, info.initial_state, info.flags,
            info.heap, info.allocation_info.Offset);
        _gpu_address = _buffer->GetGPUVirtualAddress();
        NAME_D3D12_OBJECT_INDEXED(_buffer, _size, L"D3D12 Buffer - size");
    }

    void d3d12_buffer::release()
    {
        core::deferred_release(_buffer);
        _gpu_address = 0;
        _size = 0;
    }
    ///////////////////////////////////////////////////////////////////////   D3D12 BUFFER end   /////////////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////// CONSTANT BUFFER  begin//////////////////////////////////////////////////////////////////////////////
    //frame间调度，设置为cpu可访问
    constant_buffer::constant_buffer(d3d12_buffer_init_info info)
        : _buffer{ info, true }
    {
        NAME_D3D12_OBJECT_INDEXED(buffer(), size(), L"Constant Buffer - size");

        D3D12_RANGE range{};
        DXCall(buffer()->Map(0, &range, (void**)(&_cpu_address)));
        assert(_cpu_address);
    }

    u8* const constant_buffer::allocate(u32 size)
    {
        std::lock_guard lock{ _mutex };
        const u32 aligned_size{ (u32)d3dx::align_size_for_constant_buffer(size) };
        assert(_cpu_offset + aligned_size <= _buffer.size());
        if (_cpu_offset + aligned_size <= _buffer.size())
        {
            u8* const address{ _cpu_address + _cpu_offset };
            _cpu_offset += aligned_size;
            return address;
        }

        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////// CONSTANT BUFFER  end//////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////   D3D12 Texture begin  ////////////////////////////////////////////////////////////////////////
    d3d12_texture::d3d12_texture(d3d12_texture_init_info info)
    {
        auto *const device{ core::device() };
        assert(device);

        D3D12_CLEAR_VALUE *const clear_value
        {
            (info.desc &&
            (info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
             info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
            ? &info.clear_value : nullptr
        };

        //resouce
        if (info.resource)
        {
            assert(!info.heap);
            _resource = info.resource;
        }
        else if (info.heap && info.desc)
        {
            assert(!info.resource);
            DXCall(device->CreatePlacedResource(
                info.heap, info.allocation_info.Offset, info.desc,
                info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
        }
        else if (info.desc)
        {
            assert(!info.heap && !info.resource);
            DXCall(device->CreateCommittedResource(
                &d3dx::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, info.desc,
                info.initial_state, clear_value, IID_PPV_ARGS(&_resource)));
        }


        //内存分配
        assert(_resource);
        _srv = core::srv_heap().allocate();
        device->CreateShaderResourceView(_resource, info.srv_desc, _srv.cpu);
    }

    void d3d12_texture::release()
    {
        core::srv_heap().free(_srv);
        core::deferred_release(_resource);
    }
    //////////////////////////// D3D12 Texture end//////////////////////////////////


    //////////////////////////// Render Texture begin//////////////////////////////////
    d3d12_render_texture::d3d12_render_texture(d3d12_texture_init_info info)
        : _texture{ info }
    {
        assert(info.desc);
        _mip_count = resource()->GetDesc().MipLevels;
        assert(_mip_count && _mip_count <= d3d12_texture::max_mips);

        descriptor_heap& rtv_heap{ core::rtv_heap() };
        D3D12_RENDER_TARGET_VIEW_DESC desc{};
        desc.Format = info.desc->Format;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;

        auto *const device{ core::device() };
        assert(device);

        for (u32 i{ 0 }; i < _mip_count; ++i)
        {
            _rtv[i] = rtv_heap.allocate();
            device->CreateRenderTargetView(resource(), &desc, _rtv[i].cpu);
            ++desc.Texture2D.MipSlice;
        }
    }

    void d3d12_render_texture::release()
    {
        for (u32 i{ 0 }; i < _mip_count; ++i) core::rtv_heap().free(_rtv[i]);
        _texture.release();
        _mip_count = 0;
    }
    //////////////////////////// Render Texture end//////////////////////////////////


    //////////////////////////// DepthBuffer begin//////////////////////////////////
    d3d12_depth_buffer::d3d12_depth_buffer(d3d12_texture_init_info info)
    {
        assert(info.desc);
        const DXGI_FORMAT dsv_format{ info.desc->Format };

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        if (info.desc->Format == DXGI_FORMAT_D32_FLOAT)
        {
            info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
            srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
        }
        //填写结构体
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.MostDetailedMip = 0;
        srv_desc.Texture2D.PlaneSlice = 0;
        srv_desc.Texture2D.ResourceMinLODClamp = 0.f;

        assert(!info.srv_desc && !info.resource);
        info.srv_desc = &srv_desc;
        _texture = d3d12_texture(info);

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
        dsv_desc.Format = dsv_format;
        dsv_desc.Texture2D.MipSlice = 0;

        _dsv = core::dsv_heap().allocate();

        auto *const device{ core::device() };
        assert(device);
        device->CreateDepthStencilView(resource(), &dsv_desc, _dsv.cpu);
    }

    void d3d12_depth_buffer::release()
    {
        core::dsv_heap().free(_dsv);
        _texture.release();
    }
    //////////////////////////// DepthBuffer end//////////////////////////////////
}