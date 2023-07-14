#include "D3D12Core.h"
#include "D3D12Surface.h"
#include "D3D12Shaders.h"
#include "D3D12GPass.h"
#include "D3D12PostProcess.h"
#include "D3D12Upload.h"
#include "D3D12Content.h"

using namespace Microsoft::WRL;

namespace nidhog::graphics::d3d12::core
{

	namespace
	{
		class d3d12_command
		{
		public:
			d3d12_command() = default; 
			//禁止拷贝与移动构造
			DISABLE_COPY_AND_MOVE(d3d12_command);
			explicit d3d12_command(id3d12_device* const device, D3D12_COMMAND_LIST_TYPE type)
			{
				HRESULT hr{ S_OK };
				D3D12_COMMAND_QUEUE_DESC desc{};
				desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				desc.NodeMask = 0;
				desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
				desc.Type = type; //选择cmd list的种类

				DXCall(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_cmd_queue)));
				if (FAILED(hr)) goto _error;
				//为cmd_queue命名，取决于类型
				NAME_D3D12_OBJECT(_cmd_queue,
					type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
					L"GFX Command Queue" :
					type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
					L"Compute Command Queue" : L"Command Queue");

				for (u32 i{ 0 }; i < frame_buffer_count; ++i)
				{
					command_frame& frame{ _cmd_frames[i] };
					DXCall(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmd_allocator)));
					if (FAILED(hr)) goto _error;
					//命名后添加索引
					NAME_D3D12_OBJECT_INDEXED(frame.cmd_allocator, i,
						type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
						L"GFX Command Allocator" :
						type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
						L"Compute Command Allocator" : L"Command Allocator");
				}


				DXCall(hr = device->CreateCommandList(0, type, _cmd_frames[0].cmd_allocator, nullptr, IID_PPV_ARGS(&_cmd_list)));
				if (FAILED(hr)) goto _error;
				DXCall(_cmd_list->Close());
				NAME_D3D12_OBJECT(_cmd_list,
					type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
					L"GFX Command List" :
					type == D3D12_COMMAND_LIST_TYPE_COMPUTE ?
					L"Compute Command List" : L"Command List");

				//为fence创建实例并调用
				DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
				if (FAILED(hr)) goto _error;
				NAME_D3D12_OBJECT(_fence, L"D3D12 Fence");

				//创建一个windows event
				_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
				assert(_fence_event);
				if (!_fence_event) goto _error;

				return;

			_error:
				release();
			}

			//析构函数，检查资源是否均释放
			~d3d12_command()
			{
				assert(!_cmd_queue && !_cmd_list && !_fence);
			}


			// 等待当前帧发出信号并重置 command list/allocator.
			void begin_frame()
			{
				//先通过索引确定在处理哪个帧
				command_frame& frame{ _cmd_frames[_frame_index] };
				frame.wait(_fence_event, _fence);
				//之后重置这些
				DXCall(frame.cmd_allocator->Reset());
				//告诉其一个默认的pipeline state——》 
				//能够告诉GPU应该使用什么资源以及shader
				//此时为空
				DXCall(_cmd_list->Reset(frame.cmd_allocator, nullptr));
			}

			// 用新的fence_value向fence1发出信号
			void end_frame(const d3d12_surface& surface)
			{
				DXCall(_cmd_list->Close());
				ID3D12CommandList* const cmd_lists[]{ _cmd_list };
				_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), &cmd_lists[0]);

				//  swap chain buffers 和frame buffers 同步进行
				surface.present();

				//frame结束后，添加一个fence value到末尾
				u64& fence_value{ _fence_value };
				++fence_value;
				command_frame& frame{ _cmd_frames[_frame_index] };
				frame.fence_value = fence_value;
				_cmd_queue->Signal(_fence, fence_value);

				//帧结束时增加索引
				_frame_index = (_frame_index + 1) % frame_buffer_count;
			}

			//等待GPU完成所有工作
			void flush()
			{
				for (u32 i{ 0 }; i < frame_buffer_count; ++i)
				{
					_cmd_frames[i].wait(_fence_event, _fence);
				}
				_frame_index = 0;
			}

			void release()
			{
				flush();
				core::release(_fence);
				_fence_value = 0;

				CloseHandle(_fence_event);
				_fence_event = nullptr;

				core::release(_cmd_queue);
				core::release(_cmd_list);

				for (u32 i{ 0 }; i < frame_buffer_count; ++i)
				{
					_cmd_frames[i].release();
				}
			}

			//获取指向这三者的指针
			[[nodiscard]] constexpr ID3D12CommandQueue *const command_queue() const { return _cmd_queue; }
			[[nodiscard]] constexpr id3d12_graphics_command_list *const command_list() const { return _cmd_list; }
			[[nodiscard]] constexpr u32 frame_index() const { return _frame_index; }

		private:

			struct command_frame
			{
				ID3D12CommandAllocator* cmd_allocator{ nullptr };
				u64                     fence_value{ 0 };

				void wait(HANDLE fence_event, ID3D12Fence1* fence)
				{
					assert(fence && fence_event);
					// 如果当前的fence值仍然小于“fence_value”
					// 那么我们就知道 GPU 还没有完成命令列表的执行
					// 因为它还没有到达“_cmd_queue->Signal()”命令
					if (fence->GetCompletedValue() < fence_value)
					{
						// 我们让fence创建一个事件，当fence的当前值等于“fence_value”时发出信号
						DXCall(fence->SetEventOnCompletion(fence_value, fence_event));
						// 等到fence触发当前值达到“fence_value”的事件
						// 表示命令队列执行完毕
						WaitForSingleObject(fence_event, INFINITE);
					}
				}

				void release()
				{
					core::release(cmd_allocator);
					fence_value = 0;
				}
			};

			ID3D12CommandQueue*				_cmd_queue{ nullptr };
			id3d12_graphics_command_list*	_cmd_list{ nullptr };
			ID3D12Fence1*					_fence{ nullptr };
			u64								_fence_value{ 0 };
			command_frame					_cmd_frames[frame_buffer_count]{};//不会改变的常数
			HANDLE							_fence_event{ nullptr };
			u32								_frame_index{ 0 };
		};
		using surface_collection = utl::free_list<d3d12_surface>;

		id3d12_device*					main_device;
		IDXGIFactory7*					dxgi_factory{ nullptr };
		d3d12_command					gfx_command;
		surface_collection				surfaces;
		d3dx::d3d12_resource_barrier    resource_barriers{};

		//定义heap种类
		descriptor_heap					rtv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
		descriptor_heap					dsv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
		descriptor_heap					srv_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
		descriptor_heap					uav_desc_heap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

		//每个framebuffer添加指针，以便release
		utl::vector<IUnknown*>			deferred_releases[frame_buffer_count]{};
		u32								deferred_releases_flag[frame_buffer_count]{};
		std::mutex						deferred_releases_mutex{};


		//定义最低功能级别
		constexpr D3D_FEATURE_LEVEL		minimum_feature_level{ D3D_FEATURE_LEVEL_11_0 };

		constexpr DXGI_FORMAT render_target_format{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };


		bool failed_init()
		{
			shutdown();
			return false;
		}


		// 获取支持最低功能级别的第一个性能最高的适配器。
		// NOTE: 此函数可以在功能上进行扩展
		// 例如，检查是否连接了任何输出设备
		// 枚举支持的分辨率
		// 为用户提供一种方法来选择在多适配器设置中使用哪个适配器等
		// 详情见官方文档
		IDXGIAdapter4* determine_main_adapter()
		{
			IDXGIAdapter4* adapter{ nullptr };

			//按性能降序获取adapter
			for (u32 i{ 0 };
				dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
				++i)
			{
				// 选择第一个支持最低功能级别的adapter
				if (SUCCEEDED(D3D12CreateDevice(adapter, minimum_feature_level, __uuidof(ID3D12Device), nullptr)))
				{
					return adapter;
				}
				release(adapter);
			}

			return nullptr;
		}

		D3D_FEATURE_LEVEL get_max_feature_level(IDXGIAdapter4* adapter)
		{
			constexpr D3D_FEATURE_LEVEL feature_levels[4]{
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_12_1,
			};

			//检查并填充该数据结构
			D3D12_FEATURE_DATA_FEATURE_LEVELS feature_level_info{};
			feature_level_info.NumFeatureLevels = _countof(feature_levels);
			feature_level_info.pFeatureLevelsRequested = feature_levels;

			ComPtr<ID3D12Device> device;
			DXCall(D3D12CreateDevice(adapter, minimum_feature_level, IID_PPV_ARGS(&device)));
			DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feature_level_info, sizeof(feature_level_info)));
			return feature_level_info.MaxSupportedFeatureLevel;
		}

		void __declspec(noinline) process_deferred_releases(u32 frame_idx)
		{
			std::lock_guard lock{ deferred_releases_mutex };

			// NOTE: 我们一开始就清除这个flag
			//       如果我们最后清除它，那么它可能会覆盖其他我们尝试设置的线程
			//       如果在处理项目之前发生覆盖那就还行
			deferred_releases_flag[frame_idx] = 0;

			rtv_desc_heap.process_deferred_free(frame_idx);
			dsv_desc_heap.process_deferred_free(frame_idx);
			srv_desc_heap.process_deferred_free(frame_idx);
			uav_desc_heap.process_deferred_free(frame_idx);

			//遍历资源并release
			utl::vector<IUnknown*>& resources{ deferred_releases[frame_idx] };
			if (!resources.empty())
			{
				for (auto& resource : resources) release(resource);
				resources.clear();
			}
		}

	}//匿名namespace

	namespace detail 
	{
		void deferred_release(IUnknown* resource)
		{
			const u32 frame_idx{ current_frame_index() };
			std::lock_guard lock{ deferred_releases_mutex };
			deferred_releases[frame_idx].push_back(resource);
			set_deferred_releases_flag();
		}
	} // detail namespace

	bool initialize()
	{
		// determine what is the maximum feature level that is supported
		// 确定支持的最大功能级别是什么
		// create a ID3D12Device (this a virtual adapter).
		// 创建一个 D3D12Device（显卡的虚拟表示）

		if (main_device) shutdown();
		u32 dxgi_factory_flags{ 0 };

#ifdef _DEBUG
		// Enable debugging layer. Requires "Graphics Tools" optional feature
		// 启用调试层。 需要“Graphics Tools”可选功能
		{
			ComPtr<ID3D12Debug3> debug_interface;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface))))
			{
				debug_interface->EnableDebugLayer();
#if 0
#pragma message("WARNING: GPU_based validation is enabled. This will considerably slow down the renderer!")
				debug_interface->SetEnableGPUBasedValidation(1);
#endif
			}
			else
			{
				OutputDebugStringA("Warning: D3D12 Debug interface is not available. Verify that Graphics Tools optional feature is installed on this system.\n");
			}

			dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
		}
#endif // _DEBUG

		HRESULT hr{ S_OK };//检查错误代码
		CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory));
		if (FAILED(hr)) return failed_init();

		// determine which adapter (i.e. graphics card) to use, if any
		// 如果适配器太多，选择使用的适配器

		ComPtr<IDXGIAdapter4> main_adapter;
		main_adapter.Attach(determine_main_adapter());
		if (!main_adapter) return failed_init();

		D3D_FEATURE_LEVEL max_feature_level{ get_max_feature_level(main_adapter.Get()) };
		assert(max_feature_level >= minimum_feature_level);
		if (max_feature_level < minimum_feature_level) return failed_init();

		DXCall(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&main_device)));
		if (FAILED(hr)) return failed_init();


#ifdef _DEBUG
		{
			//设置警告的严重性
			ComPtr<ID3D12InfoQueue> info_queue;
			DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));

			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		}
#endif // _DEBUG

		//一些heap的初始化
		bool result{ true };
		result &= rtv_desc_heap.initialize(512, false);
		result &= dsv_desc_heap.initialize(512, false);
		result &= srv_desc_heap.initialize(4096, true);
		result &= uav_desc_heap.initialize(512, false);
		if (!result) return failed_init();

		new (&gfx_command) d3d12_command(main_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		if (!gfx_command.command_queue()) return failed_init();

		if (!(shaders::initialize() && gpass::initialize() && fx::initialize() && upload::initialize() && content::initialize()))
			return failed_init();

		NAME_D3D12_OBJECT(main_device, L"Main D3D12 Device");
		NAME_D3D12_OBJECT(rtv_desc_heap.heap(), L"RTV Descriptor Heap");
		NAME_D3D12_OBJECT(dsv_desc_heap.heap(), L"DSV Descriptor Heap");
		NAME_D3D12_OBJECT(srv_desc_heap.heap(), L"SRV Descriptor Heap");
		NAME_D3D12_OBJECT(uav_desc_heap.heap(), L"UAV Descriptor Heap");


		return true;

	}
	void shutdown()
	{

		gfx_command.release();
		// NOTE: 我们最后不会调用process_deferred_releases
		// 因为某些资源（例如Swap chain）在其依赖资源被释放之前无法释放
		for (u32 i{ 0 }; i < frame_buffer_count; ++i)
		{
			process_deferred_releases(i);
		}
		// shutdown modules
		content::shutdown();
		upload::shutdown();
		fx::shutdown();
		gpass::shutdown();
		shaders::shutdown();

		release(dxgi_factory);

		// NOTE: 某些模块在关闭时会释放其description
		//       We process those by calling process_deferred_free once more
		rtv_desc_heap.process_deferred_free(0);
		dsv_desc_heap.process_deferred_free(0);
		srv_desc_heap.process_deferred_free(0);
		uav_desc_heap.process_deferred_free(0);
		
		rtv_desc_heap.release();
		dsv_desc_heap.release();
		srv_desc_heap.release();
		uav_desc_heap.release();

		// NOTE: 某些类型仅在shutdown/reset/clear期间对其资源使用延迟释放
		//		 为了最终释放这些资源，我们再次调用 process_deferred_releases
		process_deferred_releases(0);

#ifdef _DEBUG
		{
			{
				ComPtr<ID3D12InfoQueue> info_queue;
				DXCall(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
				info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
			}

			ComPtr<ID3D12DebugDevice2> debug_device;
			DXCall(main_device->QueryInterface(IID_PPV_ARGS(&debug_device)));
			release(main_device);
			DXCall(debug_device->ReportLiveDeviceObjects(
				D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
		}
#endif // _DEBUG

		release(main_device);
	}

	id3d12_device *const device() { return main_device; }

	descriptor_heap& rtv_heap() { return rtv_desc_heap; }

	descriptor_heap& dsv_heap() { return dsv_desc_heap; }

	descriptor_heap& srv_heap() { return srv_desc_heap; }

	descriptor_heap& uav_heap() { return uav_desc_heap; }


	u32 current_frame_index() { return gfx_command.frame_index(); }



	void set_deferred_releases_flag() { deferred_releases_flag[current_frame_index()] = 1; }

	surface create_surface(platform::window window)
	{
		surface_id id{ surfaces.add(window) };
		surfaces[id].create_swap_chain(dxgi_factory, gfx_command.command_queue());
		return surface{ id };
	}

	void remove_surface(surface_id id)
	{
		gfx_command.flush();
		surfaces.remove(id);
	}

	void resize_surface(surface_id id, u32, u32)
	{
		gfx_command.flush();
		surfaces[id].resize();
	}

	u32	surface_width(surface_id id)
	{
		return surfaces[id].width();
	}

	u32 surface_height(surface_id id)
	{
		return surfaces[id].height();
	}

	void render_surface(surface_id id)

	{
		// 等待 GPU 完成command allocator
		// 在 GPU 完成后重置allocator
		// 这将释放用于存储command的内存
		gfx_command.begin_frame();
		id3d12_graphics_command_list* cmd_list{ gfx_command.command_list() };

		//判断是否有flag，如果有就进行deferred_releases
		const u32 frame_idx{ current_frame_index() };
		if (deferred_releases_flag[frame_idx])
		{
			process_deferred_releases(frame_idx);
		}

		//引用我们需要渲染的surface
		const d3d12_surface& surface{ surfaces[id] };
		//借助barrier进行state转换
		ID3D12Resource* const current_back_buffer{ surface.back_buffer() };

		d3d12_frame_info frame_info
		{
			surface.width(),
			surface.height()
		};

		gpass::set_size({ frame_info.surface_width, frame_info.surface_height });
		d3dx::d3d12_resource_barrier& barriers{ resource_barriers };
		// 记录命令
		ID3D12DescriptorHeap* const heaps[]{ srv_desc_heap.heap() };
		cmd_list->SetDescriptorHeaps(1, &heaps[0]);

		
		cmd_list->RSSetViewports(1, &surface.viewport());
		cmd_list->RSSetScissorRects(1, &surface.scissor_rect());

		// Depth prepass
		gpass::add_transitions_for_depth_prepass(barriers);
		barriers.apply(cmd_list);
		gpass::set_render_targets_for_depth_prepass(cmd_list);
		gpass::depth_prepass(cmd_list, frame_info);

		// Geometry and lighting pass
		gpass::add_transitions_for_gpass(barriers);
		barriers.apply(cmd_list);
		gpass::set_render_targets_for_gpass(cmd_list);
		gpass::render(cmd_list, frame_info);

		// Post-process
		barriers.add(current_back_buffer,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_BARRIER_FLAG_NONE);
		gpass::add_transitions_for_post_process(barriers);
		barriers.apply(cmd_list);
		// Will write to the current back buffer, so back buffer is a render target
		fx::post_process(cmd_list,surface.rtv());
		// after post process
		// render target-> present
		d3dx::transition_resource(cmd_list, current_back_buffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);

		// Presenting swap chain buffers happens in lockstep with frame buffers.
		// surface.present();


		// 
		// 完成记录commands. 现在执行 commands,
		// 发出信号并增加下一帧的fence_value
		gfx_command.end_frame(surface);
	}

	
}