#ifdef _WIN64
#ifndef  WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // ! WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>


#ifndef USE_WITH_EDITOR

extern bool engine_initialize();
extern void engine_update();
extern void engine_shutdown();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
{
	//检测内存泄漏
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    //初始化引擎
    if (engine_initialize())
    {
        MSG msg{};
        bool is_running{ true };
        while (is_running)
        {
            //调用引擎的更新函数
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                //从消息队列中读取移除和调度消息
                //知道进程中没有消息
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                is_running &= (msg.message != WM_QUIT);
            }

            engine_update();
        }
    }
    engine_shutdown();
    return 0;
}












#endif // USE_WITH_EDITOR
#endif // _WIN64