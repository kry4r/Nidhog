#pragma once
#include "D3D12CommonHeaders.h"

namespace nidhog::graphics::d3d12
{
	class d3d12_surface
	{
	public:
		constexpr static DXGI_FORMAT default_back_buffer_format{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
		constexpr static u32 buffer_count{ 3 };
		explicit d3d12_surface(platform::window window) : _window(window)
		{
			assert(_window.handle());
		}
		//复制拷贝rt目标data中的所有内容
#if USE_STL_VECTOR
		DISABLE_COPY(d3d12_surface);
		constexpr d3d12_surface(d3d12_surface&& o)
			: _swap_chain{ o._swap_chain }, _window{ o._window }, _current_bb_index{ o._current_bb_index }
			, _viewport{ o._viewport }, _scissor_rect{ o._scissor_rect }, _allow_tearing{ o._allow_tearing }
			, _present_flags{ o._present_flags }
		{
			for (u32 i{ 0 }; i < buffer_count; ++i)
			{
				_render_target_data[i].resource = o._render_target_data[i].resource;
				_render_target_data[i].rtv = o._render_target_data[i].rtv;
			}

			o.reset();
		}

		constexpr d3d12_surface& operator=(d3d12_surface&& o)
		{
			assert(this != &o);
			if (this != &o)
			{
				release();
				move(o);
			}

			return *this;
		}
#else
		DISABLE_COPY_AND_MOVE(d3d12_surface);
#endif // USE_STL_VECTOR

		~d3d12_surface() { release(); }

		//创建Swap chain
		void create_swap_chain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue);
		void present() const;
		void resize();
	
		constexpr u32 width() const { return (u32)_viewport.Width; }
		constexpr u32 height() const { return (u32)_viewport.Height; }
		constexpr ID3D12Resource *const back_buffer() const { return _render_target_data[_current_bb_index].resource; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv() const { return _render_target_data[_current_bb_index].rtv.cpu; }
		constexpr const D3D12_VIEWPORT& viewport() const { return _viewport; }
		constexpr const D3D12_RECT& scissor_rect() const { return _scissor_rect; }


	private:
		void finalize();
		void release();

#if USE_STL_VECTOR
		//move赋值函数
		//一个移动到另一个，之后重置原有的
		constexpr void move(d3d12_surface& o)
		{
			_swap_chain = o._swap_chain;
			for (u32 i{ 0 }; i < buffer_count; ++i)
			{
				_render_target_data[i] = o._render_target_data[i];
			}
			_window = o._window;
			_current_bb_index = o._current_bb_index;
			_allow_tearing = o._allow_tearing;
			_present_flags = o._present_flags;
			_viewport = o._viewport;
			_scissor_rect = o._scissor_rect;

			o.reset();
		}

		constexpr void reset()
		{
			_swap_chain = nullptr;
			for (u32 i{ 0 }; i < buffer_count; ++i)
			{
				_render_target_data[i] = {};
			}
			_window = {};
			_current_bb_index = 0;
			_allow_tearing = 0;
			_present_flags = 0;
			_viewport = {};
			_scissor_rect = {};
		}
#endif

		struct render_target_data
		{
			//由resouce和渲染目标组成
			ID3D12Resource* resource{ nullptr };
			descriptor_handle rtv{};
		};


		// NOTE: when adding new member data here, don't forget to update the move constructor
		//       as well as the move() and reset() functions. This is to have the correct behavior
		//       when using std::vector (from STL)

		//指向swap chain接口的指针
		IDXGISwapChain4*    _swap_chain{ nullptr };
		//用来储存render target
		render_target_data  _render_target_data[buffer_count]{};
		platform::window    _window{};
		mutable u32         _current_bb_index{ 0 };
		//允许画面撕裂来提高帧率
		u32                 _allow_tearing{ 0 };
		u32                 _present_flags{ 0 };
		//视口
		D3D12_VIEWPORT      _viewport{};
		//裁剪视矩
		D3D12_RECT          _scissor_rect{};
	};
}