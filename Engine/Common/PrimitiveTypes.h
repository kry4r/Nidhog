#pragma once
//��������
#include <cstdint>

//unsigned integers �޷�������
using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

// signed integers ����������
using s64 = int64_t;
using s32 = int32_t;
using s16 = int16_t;
using s8 = int8_t;


//��Ч����������Ϊÿ��-1
constexpr u64 u64_invalid_id{ 0xffff'ffff'ffff'ffff };
constexpr u32 u32_invalid_id{ 0xffff'ffff };
constexpr u16 u16_invalid_id{ 0xffff };
constexpr u8 u8_invalid_id{ 0xff };


//float 32λ
using f32 = float;