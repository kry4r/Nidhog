#pragma once
#include"ComponentsCommon.h"

namespace nidhog::script {


    struct init_info
    {
        //ָ��ű��Ĵ�������ָ��
        detail::script_creator script_creator;
    };
    //����entity��Tansform
    component create(init_info info, game_entity::entity entity);
    void remove(component c);
}
