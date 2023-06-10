#pragma once
#include "CommonHeaders.h"

//如果没有定义用来运输的东西，就是要下面这些方法
#if !defined(SHIPPING)
namespace nidhog::content
{
	bool load_game();
	void unload_game();
}
#endif // !defined(SHIPPING)