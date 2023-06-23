#pragma once


//定义vector与deque
#define USE_STL_VECTOR 0
#define USE_STL_DEQUE 1

#if USE_STL_VECTOR
#include <vector>
namespace nidhog::utl {
	template<typename T>
	using vector = std::vector<T>;
	//交换一个元素与末尾元素，之后删除处于末尾的元素
	template<typename T>
	void erase_unordered(std::vector<T>& v, size_t index)
	{
		if(v.size() > 1)
		{ 
			std::iter_swap(v.begin() + index, v.end() - 1);
			v.pop_back();
		}
		else
		{
			v.clear();
		}
	}
}

#else
#include "Vector.h"

namespace nidhog::utl {
	template<typename T>
	void erase_unordered(vector<T>& v, size_t index)
	{
		v.erase_unordered(index);
	}
}


#endif

#if USE_STL_DEQUE
#include <deque>
namespace nidhog::utl {
	template<typename T>
	using deque = std::deque<T>;
}
#endif


namespace nidhog::utl {

	// TODO: 实现自己的容器

}


#include "FreeList.h"