#include "Common.h"
#include "CommonHeaders.h"
#include "..\Engine\Components\Script.h"
#include "..\Graphics\Renderer.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"


#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#endif

#include <Windows.h>

using namespace nidhog;

namespace {
    HMODULE game_code_dll{ nullptr };
    using _get_script_creator = nidhog::script::detail::script_creator(*)(size_t);
    _get_script_creator get_script_creator{ nullptr };
    using _get_script_names = LPSAFEARRAY(*)(void);
    _get_script_names get_script_names{ nullptr };
    //添加一个用来存放render surface的数组
    utl::vector<graphics::render_surface> surfaces;
} // anonymous namespace

EDITOR_INTERFACE u32 LoadGameCodeDll(const char* dll_path)
{
    if (game_code_dll) return FALSE;
    game_code_dll = LoadLibraryA(dll_path);
    assert(game_code_dll);

    get_script_creator = (_get_script_creator)GetProcAddress(game_code_dll, "get_script_creator");
    get_script_names = (_get_script_names)GetProcAddress(game_code_dll, "get_script_names");
    //判断返回指针是否有效
    return (game_code_dll && get_script_creator && get_script_names) ? TRUE : FALSE;
}

EDITOR_INTERFACE u32 UnloadGameCodeDll()
{
    if (!game_code_dll) return FALSE;
    assert(game_code_dll);
    int result{ FreeLibrary(game_code_dll) };
    assert(result);
    game_code_dll = nullptr;
    return TRUE;
}
EDITOR_INTERFACE script::detail::script_creator GetScriptCreator(const char* name)
{
    return (game_code_dll && get_script_creator) ? get_script_creator(script::detail::string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY GetScriptNames()
{
    return (game_code_dll && get_script_names) ? get_script_names() : nullptr;
}


EDITOR_INTERFACE u32 CreateRenderSurface(HWND host, s32 width, s32 height)
{
    assert(host);
    //设置info
    platform::window_init_info info{ nullptr, host, nullptr, 0, 0, width, height };
    //初始化对象
    graphics::render_surface surface{ platform::create_window(&info), {} };
    assert(surface.window.is_valid());
    //添加入数组
    surfaces.emplace_back(surface);
    //返回值作为其id
    return (u32)surfaces.size() - 1;
}

EDITOR_INTERFACE void RemoveRenderSurface(u32 id)
{
    assert(id < surfaces.size());
    //通过现有函数来remove
    platform::remove_window(surfaces[id].window.get_id());
}

EDITOR_INTERFACE HWND GetWindowHandle(u32 id)
{
    assert(id < surfaces.size());
    return (HWND)surfaces[id].window.handle();
}

EDITOR_INTERFACE void ResizeRenderSurface(u32 id)
{
    assert(id < surfaces.size());
    surfaces[id].window.resize(0, 0);
}
