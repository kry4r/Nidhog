﻿#include "Platform.h"
#include "PlatformTypes.h"


#ifdef _WIN64
namespace nidhog::platform {
    namespace
    {
        struct window_info
        {
            HWND    hwnd{ nullptr };
            RECT    client_area{ 0, 0, 1920, 1080 };
            RECT    fullscreen_area{};
            POINT   top_left{ 0, 0 };
            DWORD   style{ WS_VISIBLE };
            bool    is_fullscreen{ false };
            bool    is_closed{ false };
        };


        utl::free_list<window_info> windows;
        
        window_info& get_from_id(window_id id)
        {
            assert(id < windows.size());
            assert(windows[id].hwnd);
            return windows[id];
        }


        window_info& get_from_handle(window_handle handle)
        {
            //通过这个函数来获取窗口handle
            const window_id id{ (id::id_type)GetWindowLongPtr(handle, GWLP_USERDATA) };
            return get_from_id(id);
        }

        LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
        {
            window_info* info{ nullptr };
            //通过每次的message来判断
            switch (msg)
            {
            case WM_DESTROY:
                get_from_handle(hwnd).is_closed = true;
                break;
            case WM_EXITSIZEMOVE:
                info = &get_from_handle(hwnd);
                break;
            case WM_SIZE:
                if (wparam == SIZE_MAXIMIZED)
                {
                    info = &get_from_handle(hwnd);
                }
                break;
            case WM_SYSCOMMAND:
                if (wparam == SC_RESTORE)
                {
                    info = &get_from_handle(hwnd);
                }
                break;
            default:
                break;
            }

            if (info)
            {
                assert(info->hwnd);
                GetClientRect(info->hwnd, info->is_fullscreen ? &info->fullscreen_area : &info->client_area);
            }
            LONG_PTR long_ptr{ GetWindowLongPtr(hwnd, 0) };
            return long_ptr
                ? ((window_proc)long_ptr)(hwnd, msg, wparam, lparam)
                : DefWindowProc(hwnd, msg, wparam, lparam);
        }



        void
            resize_window(const window_info& info, const RECT& area)
        {
            // 调整窗口大小以获得正确的设备大小
            RECT window_rect{ area };
            AdjustWindowRect(&window_rect, info.style, FALSE);

            const s32 width{ window_rect.right - window_rect.left };
            const s32 height{ window_rect.bottom - window_rect.top };

            MoveWindow(info.hwnd, info.top_left.x, info.top_left.y, width, height, true);
        }

        void
            resize_window(window_id id, u32 width, u32 height)
        {
            window_info& info{ get_from_id(id) };

            // NOTE: 当我们在关卡编辑器中托管窗口时，我们只需更新内部数据
            //       即客户端区域
            if (info.style & WS_CHILD)
            {
                GetClientRect(info.hwnd, &info.client_area);
            }
            else
            {
                RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
                area.bottom = area.top + height;
                area.right = area.left + width;

                resize_window(info, area);
            }
        }

        void
            set_window_fullscreen(window_id id, bool is_fullscreen)
        {
            window_info& info{ get_from_id(id) };
            if (info.is_fullscreen != is_fullscreen)
            {
                info.is_fullscreen = is_fullscreen;

                if (is_fullscreen)
                {
                    // 存储当前窗口尺寸
                    // 以便在切换出全屏状态时可以恢复这些尺寸
                    GetClientRect(info.hwnd, &info.client_area);
                    RECT rect;
                    GetWindowRect(info.hwnd, &rect);
                    info.top_left.x = rect.left;
                    info.top_left.y = rect.top;
                    SetWindowLongPtr(info.hwnd, GWL_STYLE, 0);
                    ShowWindow(info.hwnd, SW_MAXIMIZE);
                }
                else
                {
                    info.style = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
                    SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
                    resize_window(info, info.client_area);
                    ShowWindow(info.hwnd, SW_SHOWNORMAL);
                }
            }
        }

        bool is_window_fullscreen(window_id id)
        {
            return get_from_id(id).is_fullscreen;
        }

        window_handle get_window_handle(window_id id)
        {
            return get_from_id(id).hwnd;
        }

        void set_window_caption(window_id id, const wchar_t* caption)
        {
            window_info& info{ get_from_id(id) };
            SetWindowText(info.hwnd, caption);
        }

        math::u32v4  get_window_size(window_id id)
        {
            window_info& info{ get_from_id(id) };
            RECT& area{ info.is_fullscreen ? info.fullscreen_area : info.client_area };
            return { (u32)area.left, (u32)area.top , (u32)area.right , (u32)area.bottom };
        }

        bool is_window_closed(window_id id)
        {
            return get_from_id(id).is_closed;
        }

    }

    window create_window(const window_init_info* const init_info /* = nullptr */)
    {
        //如果找到这个指针且不为空，之后我们就可以使用其包含的信息
        window_proc callback{ init_info ? init_info->callback : nullptr };
        window_handle parent{ init_info ? init_info->parent : nullptr };



        //设置一个window类（填写struct，和vulkan差不多

        WNDCLASSEX wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = internal_window_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = callback ? sizeof(callback) : 0;
        wc.hInstance = 0;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
        wc.lpszMenuName = NULL;
        wc.lpszClassName = L"NidhogWindow";
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
        //注册window类

        RegisterClassEx(&wc);

        window_info info{};
        info.client_area.right = (init_info && init_info->width) ? info.client_area.left + init_info->width : info.client_area.right;
        info.client_area.bottom = (init_info && init_info->height) ? info.client_area.top + init_info->height : info.client_area.bottom;
        info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

        RECT rect{ info.client_area };

        //调整窗口大小以获得正确的设备大小

        //检查初始化信息
        const wchar_t* caption{ (init_info && init_info->caption) ? init_info->caption : L"Nidhog Game" };
        const s32 left{ init_info ? init_info->left : info.top_left.x };
        const s32 top{ init_info ? init_info->top : info.top_left.y };
        const s32 width{ rect.right - rect.left };
        const s32 height{ rect.bottom - rect.top };



        //创建一个window类的实例

        info.hwnd = CreateWindowEx(
            0,                // 扩展样式
            wc.lpszClassName, // 类名
            caption,          // 实例标题
            info.style,       // 窗口风格
            left, top,        // 初始化窗口位置
            width, height,    // 初始化窗口大小
            parent,           // parent窗口句柄
            NULL,             // 菜单句柄（handle to menu）
            NULL,             // 这个应用的实例
            NULL);            // 额外创建参数


        if (info.hwnd)
        {
            DEBUG_OP(SetLastError(0));
            //需要个函数来添加windowid
            const window_id id{ windows.add(info) };
            SetWindowLongPtr(info.hwnd, GWLP_USERDATA, (LONG_PTR)id);
            // 在extra字节中设置指向窗口回调函数的指针
            // 用于处理窗口的消息
            if (callback) SetWindowLongPtr(info.hwnd, 0, (LONG_PTR)callback);
            assert(GetLastError() == 0);
            ShowWindow(info.hwnd, SW_SHOWNORMAL);
            UpdateWindow(info.hwnd);
            return window{ id };
        }
        return {};
    }
    void remove_window(window_id id)
    {
        window_info& info{ get_from_id(id) };
        DestroyWindow(info.hwnd);
        windows.remove(id);
    }

#else
#error "must implement at least one platform"
#endif  // _WIND64



    void window::set_fullscreen(bool is_fullscreen) const
    {
        assert(is_valid());
        set_window_fullscreen(_id, is_fullscreen);
    }
    
    bool window::is_fullscreen() const
    {
        assert(is_valid());
        return is_window_fullscreen(_id);
    }
    
    void* window::handle() const
    {
        assert(is_valid());
        return get_window_handle(_id);
    }
    
    void window::set_caption(const wchar_t* caption) const
    {
        assert(is_valid());
        set_window_caption(_id, caption);
    }
    
    math::u32v4 window::size() const
    {
        assert(is_valid());
        return get_window_size(_id);
    }
    
    void window::resize(u32 width, u32 height) const
    {
        assert(is_valid());
        resize_window(_id, width, height);
    }
    
    u32 window::width() const
    {
        math::u32v4 s{ size() };
        return s.z - s.x;
    }
    
    u32 window::height() const
    {
        math::u32v4 s{ size() };
        return s.w - s.y;
    }
    
    bool window::is_closed() const
    {
        assert(is_valid());
        return is_window_closed(_id);
    }
}

