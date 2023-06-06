#pragma once
#include"ComponentsCommon.h"


namespace nidhog {
    //�����������������namespace
#define INIT_INFO(component) namespace component { struct init_info; }

    INIT_INFO(transform);
    INIT_INFO(script);

#undef INIT_INFO

    namespace game_entity {
        struct entity_info
        {
            //��entity_info�а�����Ϣ
            transform::init_info* transform{ nullptr };
            script::init_info* script{nullptr};
        };
        //������ɾ��entity���ж�entity�Ƿ����
        entity create(entity_info info);
        void remove(entity_id id);
        bool is_alive(entity_id id);
    }
}