#pragma once

#include "CommonHeaders.h"

#ifdef _WIN64
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif
//平台特定头文件
#include <Windows.h>

namespace nidhog::platform {

    //接收消息
    using window_proc = LRESULT(*)(HWND, UINT, WPARAM, LPARAM);
    using window_handle = HWND;

    //窗口初始化信息
    struct window_init_info
    {
        window_proc     callback{ nullptr };
        window_handle   parent{ nullptr };
        const wchar_t*  caption{ nullptr };
        s32             left{ 0 };
        s32             top{ 0 };
        s32             width{ 1920 };
        s32             height{ 1080 };
    };
}
#endif