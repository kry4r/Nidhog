#pragma once
#include"ComponentsCommon.h"

namespace nidhog::transform {


    struct init_info
    {
        f32 position[3]{};//位置
        f32 rotation[4]{};//旋转
        f32 scale[3]{ 1.f, 1.f, 1.f };//缩放
    };
    //创建entity的Tansform
    component create(init_info info, game_entity::entity entity);
    void remove(component c);
    void get_transform_matrices(const game_entity::entity_id id, math::m4x4& world, math::m4x4& inverse_world);
}