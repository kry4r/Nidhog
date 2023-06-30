#include "D3D12Surface.h"
#include "D3D12Core.h"


namespace nidhog::graphics::d3d12
{
	namespace
	{
        //设置format，设置为srgb
        constexpr DXGI_FORMAT
            to_non_srgb(DXGI_FORMAT format)
        {
            if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) return DXGI_FORMAT_R8G8B8A8_UNORM;

            return format;
        }
	}//匿名namespace


	void d3d12_surface::create_swap_chain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format/*= default_back_buffer_format*/)
	{
		//检查factory和queue是否包含有效指针
		assert(factory && cmd_queue);
		release();

        //允许画面撕裂来提高帧率
        if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allow_tearing, sizeof(u32))) && _allow_tearing)
        {
            _present_flags = DXGI_PRESENT_ALLOW_TEARING;
        }
        _format = format;

        //填写swap chain的 desc
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.BufferCount = buffer_count;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.Flags = _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        desc.Format = to_non_srgb(format);
        desc.Height = _window.height();
        desc.Width = _window.width();
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Stereo = false;

        //填写完结构体，创建swap chain实例
        IDXGISwapChain1* swap_chain;
        HWND hwnd{ (HWND)_window.handle() };
        DXCall(factory->CreateSwapChainForHwnd(cmd_queue, hwnd, &desc, nullptr, nullptr, &swap_chain));
        DXCall(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
        DXCall(swap_chain->QueryInterface(IID_PPV_ARGS(&_swap_chain)));
        core::release(swap_chain);

        //获取当前后台buffer
        _current_bb_index = _swap_chain->GetCurrentBackBufferIndex();

        //使用render target heap来分配back buffer
        for (u32 i{ 0 }; i < buffer_count; ++i)
        {
            _render_target_data[i].rtv = core::rtv_heap().allocate();
        }

        finalize();
	}

    void d3d12_surface::present() const
    {
        //检查sc是否有效
        assert(_swap_chain);
        DXCall(_swap_chain->Present(0, _present_flags));
        _current_bb_index = _swap_chain->GetCurrentBackBufferIndex();
    }

    void d3d12_surface::resize()
    {

    }

    void d3d12_surface::finalize()
    {
        // 为back buffer 创建Render Target View
        for (u32 i{ 0 }; i < buffer_count; ++i)
        {
            //获取其条目之一的引用
            render_target_data& data{ _render_target_data[i] };
            //检查资源制作是否有效
            assert(!data.resource);
            DXCall(_swap_chain->GetBuffer(i, IID_PPV_ARGS(&data.resource)));
            //填充 rtv结构体
            D3D12_RENDER_TARGET_VIEW_DESC desc{};
            desc.Format = _format;
            desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            core::device()->CreateRenderTargetView(data.resource, &desc, data.rtv.cpu);
        }

        //填充swap chain结构体
        DXGI_SWAP_CHAIN_DESC desc{};
        DXCall(_swap_chain->GetDesc(&desc));
        const u32 width{ desc.BufferDesc.Width };
        const u32 height{ desc.BufferDesc.Height };
        assert(_window.width() == width && _window.height() == height);

        // 设置视口和裁剪视矩
        _viewport.TopLeftX = 0.f;
        _viewport.TopLeftY = 0.f;
        _viewport.Width = (float)width;
        _viewport.Height = (float)height;
        _viewport.MinDepth = 0.f;
        _viewport.MaxDepth = 1.f;

        _scissor_rect = { 0, 0, (s32)width, (s32)height };
    }

    void d3d12_surface::release()
    {
        for (u32 i{ 0 }; i < buffer_count; ++i)
        {
            //按顺序释放
            render_target_data& data{ _render_target_data[i] };
            core::release(data.resource);
            core::rtv_heap().free(data.rtv);
        }

        core::release(_swap_chain);
    }
}