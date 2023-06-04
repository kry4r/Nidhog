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
    component create_transform(const init_info& info, game_entity::entity entity);
    void remove_transform(component c);
}