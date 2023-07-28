#pragma once

#include "D3D12CommonHeaders.h"

namespace nidhog::graphics::d3d12
{
	class descriptor_heap;

	struct descriptor_handle {
		D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
		u32              index{ u32_invalid_id };
		//ȷ��handle���
		[[nodiscard]] constexpr bool is_valid() const { return cpu.ptr != 0; }
		[[nodiscard]] constexpr bool is_shader_visible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
	private:
		friend class descriptor_heap;
		descriptor_heap* container{ nullptr };
		//u32              index{ u32_invalid_id };
#endif
	};

	class descriptor_heap
	{
	public:
		explicit descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type{ type } {}
		DISABLE_COPY_AND_MOVE(descriptor_heap);
		~descriptor_heap() { assert(!_heap);}

		bool initialize(u32 capacity, bool is_shader_visible);
		void release();
		void process_deferred_free(u32 frame_idx);

		[[nodiscard]] descriptor_handle allocate();
		void free(descriptor_handle& handle);

		//��Ӷ�private��Ա��ֻ������
		[[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE type() const { return _type; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return _cpu_start; }
		[[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return _gpu_start; }
		[[nodiscard]] constexpr ID3D12DescriptorHeap *const heap() const { return _heap; }
		[[nodiscard]] constexpr u32 capacity() const { return _capacity; }
		[[nodiscard]] constexpr u32 size() const { return _size; }
		[[nodiscard]] constexpr u32 descriptor_size() const { return _descriptor_size; }
		[[nodiscard]] constexpr bool is_shader_visible() const { return _gpu_start.ptr != 0; }

	private: 
		//������ɫ���Ƿ�ɼ��������GPU/System memory
		ID3D12DescriptorHeap*               _heap;
		//ָ��heap��ָ��
		D3D12_CPU_DESCRIPTOR_HANDLE         _cpu_start{};
		D3D12_GPU_DESCRIPTOR_HANDLE         _gpu_start{};

		std::unique_ptr<u32[]>              _free_handles{};

		//�����ڷ����λ�õ�����
		utl::vector<u32>                    _deferred_free_indices[frame_buffer_count]{};
		//������
		std::mutex                          _mutex{};
		u32                                 _capacity{ 0 };
		u32                                 _size{ 0 };
		//�ض�heap��������С������
		u32                                 _descriptor_size{};
		const D3D12_DESCRIPTOR_HEAP_TYPE    _type{};
	};

	// �ṩParameter to create buffer resource
	struct d3d12_buffer_init_info
	{
		ID3D12Heap1* heap{ nullptr };
		const void* data{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1     allocation_info{};
		D3D12_RESOURCE_STATES               initial_state{};
		D3D12_RESOURCE_FLAGS                flags{ D3D12_RESOURCE_FLAG_NONE };
		u32                                 size{ 0 };
		u32                                 stride{ 0 };
		u32                                 element_count{ 0 };
		u32                                 alignment{ 0 };
		bool                                create_uav{ false };
	};

	class d3d12_buffer
	{
	public:
		d3d12_buffer() = default;
		explicit d3d12_buffer(d3d12_buffer_init_info info, bool is_cpu_accessible);
		//NOTE : �����˿�������Ҫʵ��һ���ƶ�
		DISABLE_COPY(d3d12_buffer);
		constexpr d3d12_buffer(d3d12_buffer&& o)
			: _buffer{ o._buffer }, _gpu_address{ o._gpu_address }, _size{ o._size }
		{
			o.reset();
		}

		constexpr d3d12_buffer& operator=(d3d12_buffer&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				release();
				move(o);
			}

			return *this;
		}

		~d3d12_buffer() { release(); }

		void release();
		[[nodiscard]] constexpr ID3D12Resource* const buffer() const { return _buffer; }
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return _gpu_address; }
		[[nodiscard]] constexpr u32 size() const { return _size; }

	private:
		constexpr void move(d3d12_buffer& o)
		{
			_buffer = o._buffer;
			_gpu_address = o._gpu_address;
			_size = o._size;
			o.reset();
		}

		constexpr void reset()
		{
			_buffer = nullptr;
			_gpu_address = 0;
			_size = 0;
		}

		ID3D12Resource* _buffer{ nullptr };
		D3D12_GPU_VIRTUAL_ADDRESS   _gpu_address{ 0 };
		u32                         _size{ 0 };
	};

	//NOTE: Manager of buffer,Transform data entour each frame
	//		Similar to linear stack allocator
	class constant_buffer
	{
	public:
		constant_buffer() = default;
		explicit constant_buffer(d3d12_buffer_init_info info);
		DISABLE_COPY_AND_MOVE(constant_buffer);
		~constant_buffer() { release(); }

		void release()
		{
			_buffer.release();
			_cpu_address = nullptr;
			_cpu_offset = 0;
		}

		constexpr void clear() { _cpu_offset = 0; }
		[[nodiscard]] u8* const allocate(u32 size);

		template<typename T>
		[[nodiscard]] T* const allocate()
		{
			return (T* const)allocate(sizeof(T));
		}

		[[nodiscard]] constexpr ID3D12Resource* const buffer() const { return _buffer.buffer(); }
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return _buffer.gpu_address(); }
		[[nodiscard]] constexpr u32 size() const { return _buffer.size(); }
		[[nodiscard]] constexpr u8* const cpu_address() const { return _cpu_address; }

		//caculate gpu virtual adress
		template<typename T>
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address(T* const allocation)
		{
			std::lock_guard lock{ _mutex };
			assert(_cpu_address);
			if (!_cpu_address) return {};
			const u8* const address{ (const u8* const)allocation };
			assert(address <= _cpu_address + _cpu_offset);
			assert(address >= _cpu_address);
			const u64 offset{ (u64)(address - _cpu_address) };
			return _buffer.gpu_address() + offset;
		}

		[[nodiscard]] constexpr static d3d12_buffer_init_info get_default_init_info(u32 size)
		{
			assert(size);
			d3d12_buffer_init_info info{};
			info.size = size;
			info.alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
			return info;
		}

	private:
		d3d12_buffer    _buffer{};
		u8* _cpu_address{ nullptr };
		u32             _cpu_offset{ 0 };
		std::mutex      _mutex{};
	};

	class structured_buffer
	{
	public:
		structured_buffer() = default;
		explicit structured_buffer(const d3d12_buffer_init_info& info);
		DISABLE_COPY(structured_buffer);
		constexpr structured_buffer(structured_buffer&& o)
			: _buffer{ std::move(o._buffer) }, _uav{ o._uav }, _uav_shader_visible{ o._uav_shader_visible }
			, _stride{ o._stride }
		{
			o.reset();
		}

		constexpr structured_buffer& operator=(structured_buffer&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				release();
				move(o);
			}

			return *this;
		}

		~structured_buffer() { release(); }

		void release();

		void clear_uav(id3d12_graphics_command_list* const cmd_list, const u32* const values) const
		{
			cmd_list->ClearUnorderedAccessViewUint(_uav_shader_visible.gpu, _uav.cpu,
													buffer(), values, 0, nullptr);
		}

		void clear_uav(id3d12_graphics_command_list* const cmd_list, const f32* const values) const
		{
			cmd_list->ClearUnorderedAccessViewFloat(_uav_shader_visible.gpu, _uav.cpu,
													buffer(), values, 0, nullptr);
		}

		[[nodiscard]] constexpr ID3D12Resource* buffer() const { return _buffer.buffer(); }
		[[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return _buffer.gpu_address(); }
		[[nodiscard]] constexpr u32 size() const { return _buffer.size(); }
		[[nodiscard]] constexpr descriptor_handle uav() const { return _uav; }
		[[nodiscard]] constexpr descriptor_handle uav_shader_visible() const { return _uav_shader_visible; }

		[[nodiscard]] constexpr static d3d12_buffer_init_info get_default_init_info(u32 stride, u32 element_count)
		{
			assert(stride && element_count);
			d3d12_buffer_init_info info{};
			info.size = stride * element_count;
			info.stride = stride;
			info.element_count = element_count;
			info.alignment = stride;
			info.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			return info;
		}

	private:

		constexpr void move(structured_buffer& o)
		{
			_buffer = std::move(o._buffer);
			_uav = o._uav;
			_uav_shader_visible = o._uav_shader_visible;
			_stride = o._stride;
			o.reset();
		}

		constexpr void reset()
		{
			_uav = {};
			_uav_shader_visible = {};
			_stride = 0;
		}

		d3d12_buffer        _buffer{};
		descriptor_handle   _uav{};
		descriptor_handle   _uav_shader_visible{};
		u32                 _stride{ 0 };
	};



	struct d3d12_texture_init_info
	{
		ID3D12Heap1*                        heap{ nullptr };
		ID3D12Resource*                     resource{ nullptr };
		D3D12_SHADER_RESOURCE_VIEW_DESC*    srv_desc{ nullptr };
		D3D12_RESOURCE_DESC*                desc{ nullptr };
		D3D12_RESOURCE_ALLOCATION_INFO1     allocation_info{};
		D3D12_RESOURCE_STATES               initial_state{};
		D3D12_CLEAR_VALUE                   clear_value{};
	};

	//����
	class d3d12_texture
	{
	public:
		constexpr static u32 max_mips{ 14 };      // ���֧��16k�ֱ���
		d3d12_texture() = default;
		explicit d3d12_texture(d3d12_texture_init_info info);
		//���ø��ƣ���֤ÿ��instanceΨһ
		DISABLE_COPY(d3d12_texture);
		constexpr d3d12_texture(d3d12_texture&& o)
			: _resource{ o._resource }, _srv{ o._srv }
		{
			o.reset();
		}

		constexpr d3d12_texture& operator=(d3d12_texture&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				release();
				move(o);
			}
			return *this;
		}

		~d3d12_texture() { release(); }
		void release();
		[[nodiscard]] constexpr ID3D12Resource *const resource() const { return _resource; }
		[[nodiscard]] constexpr descriptor_handle srv() const { return _srv; }

	private:
		constexpr void move(d3d12_texture& o)
		{
			_resource = o._resource;
			_srv = o._srv;
			o.reset();
		}

		constexpr void reset()
		{
			_resource = nullptr;
			_srv = {};
		}
		ID3D12Resource*     _resource{ nullptr };
		descriptor_handle   _srv;
	};


	class d3d12_render_texture
	{
	public:
		d3d12_render_texture() = default;
		explicit d3d12_render_texture(d3d12_texture_init_info info);
		DISABLE_COPY(d3d12_render_texture);
		constexpr d3d12_render_texture(d3d12_render_texture&& o)
			: _texture{ std::move(o._texture) }, _mip_count{ o._mip_count }
		{
			for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
			o.reset();
		}

		constexpr d3d12_render_texture& operator=(d3d12_render_texture&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				release();
				move(o);
			}
			return *this;
		}

		~d3d12_render_texture() { release(); }

		void release();
		[[nodiscard]] constexpr u32 mip_count() const { return _mip_count; }
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv(u32 mip_index) const { assert(mip_index < _mip_count); return _rtv[mip_index].cpu; }
		[[nodiscard]] constexpr descriptor_handle srv() const { return _texture.srv(); }
		[[nodiscard]] constexpr ID3D12Resource *const resource() const { return _texture.resource(); }
	private:
		constexpr void move(d3d12_render_texture& o)
		{
			_texture = std::move(o._texture);
			_mip_count = o._mip_count;
			for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = o._rtv[i];
			o.reset();
		}

		constexpr void reset()
		{
			for (u32 i{ 0 }; i < _mip_count; ++i) _rtv[i] = {};
			_mip_count = 0;
		}

		d3d12_texture       _texture{};
		descriptor_handle   _rtv[d3d12_texture::max_mips]{};
		u32                 _mip_count{ 0 };
	};

	class d3d12_depth_buffer
	{
	public:
		d3d12_depth_buffer() = default;
		explicit d3d12_depth_buffer(d3d12_texture_init_info info);
		DISABLE_COPY(d3d12_depth_buffer);
		constexpr d3d12_depth_buffer(d3d12_depth_buffer&& o)
			: _texture{ std::move(o._texture) }, _dsv{ o._dsv }
		{
			o._dsv = {};
		}

		constexpr d3d12_depth_buffer& operator=(d3d12_depth_buffer&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				_texture = std::move(o._texture);
				_dsv = o._dsv;
				o._dsv = {};
			}
			return *this;
		}

		~d3d12_depth_buffer() { release(); }

		void release();
		[[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE dsv() const { return _dsv.cpu; }
		[[nodiscard]] constexpr descriptor_handle srv() const { return _texture.srv(); }
		[[nodiscard]] constexpr ID3D12Resource *const resource() const { return _texture.resource(); }

	private:
		d3d12_texture       _texture{};
		descriptor_handle   _dsv{};
	};
}