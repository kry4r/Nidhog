#pragma once
#pragma once
#include "CommonHeaders.h"

namespace nidhog::id {

    using id_type = u32; //32位id
    namespace detail{
        constexpr u32 generation_bits{ 8 }; //保留八位的generation位
        constexpr u32 index_bits{ sizeof(id_type) * 8 - generation_bits };//id索引位
        constexpr id_type index_mask{ (id_type{1} << index_bits) - 1 };//索引位掩码
        constexpr id_type generation_mask{ (id_type{1} << generation_bits) - 1 };//生成位掩码
    }//detail namespace
    
    constexpr id_type invalid_id{ id_type(-1) };//id掩码
    constexpr u32 min_deleted_elements{ 1024 };//在开始写回删除元素位置之前删除的元素数量

    using generation_type = std::conditional_t<detail::generation_bits <= 16, std::conditional_t<detail::generation_bits <= 8, u8, u16>, u32>;//储存的generation的类型·
    //加上两个断言判断，防止数据溢出
    static_assert(sizeof(generation_type) * 8 >= detail::generation_bits);
    static_assert((sizeof(id_type) - sizeof(generation_type)) > 0);

    constexpr bool
        is_valid(id_type id)
    {
        return id != invalid_id;
    }

    constexpr id_type
        index(id_type id)
    {
        id_type index{ id & detail::index_mask };
        assert(index != detail::index_mask);
        return index;
    }

    constexpr id_type
        generation(id_type id)
    {
        return (id >> detail::index_bits) & detail::generation_mask;
    }

    constexpr id_type
        new_generation(id_type id)//函数以得到id
    {
        const id_type generation{ id::generation(id) + 1 };
        assert(generation < (((u64)1 << detail::generation_bits) - 1));//计算最大值并进行判断
        return index(id) | (generation << detail::index_bits);
    }

#if _DEBUG
    namespace detail {
        struct id_base
        {
            constexpr explicit id_base(id_type id) : _id{ id } {}
            constexpr operator id_type() const { return _id; }
        private:
            id_type _id;
        };
    }//detail namespace
#define DEFINE_TYPED_ID(name)                                   \
        struct name final : id::detail::id_base               \
        {                                                       \
            constexpr explicit name(id::id_type id)             \
                : id_base{ id } {}                              \
            constexpr name() : id_base{ 0 } {}                  \
        };
#else
    //一个用来继承id type的宏
#define DEFINE_TYPED_ID(name) using name = id::id_type;
#endif

}