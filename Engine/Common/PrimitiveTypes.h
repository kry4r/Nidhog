#pragma once
//基础类型
#include <cstdint>

//unsigned integers 无符号整数
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

// signed integers 带符号整数
using s64 = int64_t;
using s32 = int32_t;
using s16 = int16_t;
using s8 = int8_t;


//无效索引，设置为每个-1
constexpr u64 u64_invalid_id{ 0xffff'ffff'ffff'ffff };
constexpr u32 u32_invalid_id{ 0xffff'ffff };
constexpr u16 u16_invalid_id{ 0xffff };
constexpr u8 u8_invalid_id{ 0xff };


//float 32位
using f32 = float;