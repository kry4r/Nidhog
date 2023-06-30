#pragma once
#include "CommonHeaders.h"

//如果没有定义用来运输的东西，就是要下面这些方法
#if !defined(SHIPPING)
namespace nidhog::content
{
	bool load_game();
	void unload_game();
	bool load_engine_shaders(std::unique_ptr<u8[]>& shaders, u64& size);
}
#endif // !defined(SHIPPING)