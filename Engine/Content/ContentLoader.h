#pragma once
#include "CommonHeaders.h"

//���û�ж�����������Ķ���������Ҫ������Щ����
#if !defined(SHIPPING)
namespace nidhog::content
{
	bool load_game();
	void unload_game();
	bool load_engine_shaders(std::unique_ptr<u8[]>& shaders, u64& size);
}
#endif // !defined(SHIPPING)