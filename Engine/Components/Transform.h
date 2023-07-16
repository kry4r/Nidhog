#pragma once
#include"ComponentsCommon.h"

namespace nidhog::transform {


    struct init_info
    {
        f32 position[3]{};//λ��
        f32 rotation[4]{};//��ת
        f32 scale[3]{ 1.f, 1.f, 1.f };//����
    };
    //����entity��Tansform
    component create(init_info info, game_entity::entity entity);
    void remove(component c);
    void get_transform_matrices(const game_entity::entity_id id, math::m4x4& world, math::m4x4& inverse_world);
}