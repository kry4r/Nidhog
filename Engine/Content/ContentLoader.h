#pragma once
#include "CommonHeaders.h"

//���û�ж�����������Ķ���������Ҫ������Щ����
#if !defined(SHIPPING)
namespace nidhog::content
{
	bool load_game();
	void unload_game();
}
#endif // !defined(SHIPPING)