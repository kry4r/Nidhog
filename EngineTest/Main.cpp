#pragma comment(lib,"engine.lib")




//���ڲ���Component
#define TEST_ENTITY_COMPONENTS 1

#if TEST_ENTITY_COMPONENTS
#include "TestEntityComponents.h"
#else
#error One of the tests need to be enabled
#endif

int main() 
{
	//����ڴ�й©���˴�Ϊ�Ѽ��/����Ƿ��ͷŷ�����ڴ棩
#if _Debug
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	engine_test test{};
	if (test.initialize()) {
		test.run();
	}
	test.shutdown();
}