#pragma once

#include "CommonHeaders.h"
#include "Graphics\Renderer.h"
#include "Platform\Window.h"

//d3d12特定头文件////////////////////////////////////
#include <dxgi1_6.h>
#include <d3d12.h>


//对于Componet的类似智能指针
#include <wrl.h>
////////////////////////////////////////////////////





////////////////////////链接库//////////////////////
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
////////////////////////////////////////////////////

namespace nidhog::graphics::d3d12
{
    constexpr u32 frame_buffer_count{ 3 };
    //这两种常用，使用类型别名
    using id3d12_device = ID3D12Device8;
    using id3d12_graphics_command_list = ID3D12GraphicsCommandList6;
}

// Assert that COM call to D3D API succeeded
// 断言对 D3D API 的 COM 调用是否成功
// 分别注明在哪个文件，哪一行，哪条语句

#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)                                   \
if(FAILED(x)) {                                     \
    char line_number[32];                           \
    sprintf_s(line_number, "%u", __LINE__);         \
    OutputDebugStringA("Error in: ");               \
    OutputDebugStringA(__FILE__);                   \
    OutputDebugStringA("\nLine: ");                 \
    OutputDebugStringA(line_number);                \
    OutputDebugStringA("\n");                       \
    OutputDebugStringA(#x);                         \
    OutputDebugStringA("\n");                       \
    __debugbreak();                                 \
}
#endif // !DXCall
#else
#ifndef DXCall
#define DXCall(x) x
#endif // !DXCall
#endif // _DEBUG



#ifdef _DEBUG
// 设置 COM 对象的名称并在 Visual Studio 的输出面板中输出调试字符串
#define NAME_D3D12_OBJECT(obj, name) obj->SetName(name); OutputDebugString(L"::D3D12 Object Created: "); OutputDebugString(name); OutputDebugString(L"\n");
// 用于命名某项东西之后在命名末尾添加索引
#define NAME_D3D12_OBJECT_INDEXED(obj, n, name)     \
{                                                   \
wchar_t full_name[128];                             \
if (swprintf_s(full_name, L"%s[%u]", name, n) >0 ){ \
    obj->SetName(full_name);                        \
    OutputDebugString(L"::D3D12 Object Created: "); \
    OutputDebugString(full_name);                   \
    OutputDebugString(L"\n");                       \
}}
#else
#define NAME_D3D12_OBJECT(x, name)
#endif // _DEBUG