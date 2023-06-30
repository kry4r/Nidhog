
#include "Test.h"

#pragma comment(lib, "Engine.lib")


#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#elif TEST_WINDOW
#include "TestWindow.h"
#elif TEST_RENDERER
#include"TestRenderer.h"
#else
#error One of the tests need to be enabled
#endif
#ifdef _WIN64
#include <Windows.h>

#include <filesystem>

// TODO: 复制!
std::filesystem::path
set_current_directory_to_executable_path()
{
    // 将工作目录设置为可执行路径
    wchar_t path[MAX_PATH];
    const uint32_t length{ GetModuleFileName(0, &path[0], MAX_PATH) };
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return {};
    std::filesystem::path p{ path };
    std::filesystem::current_path(p.parent_path());
    return std::filesystem::current_path();
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    set_current_directory_to_executable_path();
    engine_test test{};
    if (test.initialize())
    {
        MSG msg{};
        bool is_running{ true };
        while (is_running)
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                is_running &= (msg.message != WM_QUIT);
            }

            test.run();
        }
    }
    test.shutdown();
    return 0;
}

#else
int main() 
{
	//检查内存泄漏（此处为堆检查/检查是否释放分配的内存）
#if _Debug
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	engine_test test{};
	if (test.initialize()) {
		test.run();
	}
	test.shutdown();
}

#endif //_WIN64