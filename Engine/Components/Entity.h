#pragma once
#include"ComponentsCommon.h"


namespace nidhog {
    //定义宏来包含其他的namespace
#define INIT_INFO(component) namespace component { struct init_info; }

    INIT_INFO(transform);

#undef INIT_INFO

    namespace game_entity {
        struct entity_info
        {
            //在entity_info中包含信息
            transform::init_info* transform{ nullptr };
        };
        //创建与删除entity，判断entity是否存在
        entity create_game_entity(const entity_info& info);
        void remove_game_entity(entity e);
        bool is_alive(entity e);
    }
}