#pragma comment(lib,"engine.lib")




//用于测试Component
#define TEST_ENTITY_COMPONENTS 1

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#else
#error One of the tests need to be enabled
#endif

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