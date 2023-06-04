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
    component create_transform(const init_info& info, game_entity::entity entity);
    void remove_transform(component c);
}