#pragma once
#include"ComponentsCommon.h"

namespace nidhog::script {


    struct init_info
    {
        //指向脚本的创建函数指针
        detail::script_creator script_creator;
    };
    //创建entity的Tansform
    component create(init_info info, game_entity::entity entity);
    void remove(component c);
}
