#pragma once
#include"ComponentsCommon.h"


namespace nidhog {
    //�����������������namespace
#define INIT_INFO(component) namespace component { struct init_info; }

    INIT_INFO(transform);

#undef INIT_INFO

    namespace game_entity {
        struct entity_info
        {
            //��entity_info�а�����Ϣ
            transform::init_info* transform{ nullptr };
        };
        //������ɾ��entity���ж�entity�Ƿ����
        entity create_game_entity(const entity_info& info);
        void remove_game_entity(entity e);
        bool is_alive(entity e);
    }
}