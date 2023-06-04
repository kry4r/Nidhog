#pragma once

//定义vector与deque
#define USE_STL_VECTOR 1
#define USE_STL_DEQUE 1

#if USE_STL_VECTOR
#include <vector>
namespace nidhog::utl {
	template<typename T>
	using vector = std::vector<T>;
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