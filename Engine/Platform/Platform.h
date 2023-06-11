#pragma once

#include "CommonHeaders.h"
#include "Window.h"

namespace nidhog::platform {
    //初始化window信息
    struct window_init_info;

    window create_window(const window_init_info* const init_info = nullptr);
    void remove_window(window_id id);
}