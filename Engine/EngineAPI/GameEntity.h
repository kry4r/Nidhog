#pragma once

#include "..\Components\ComponentsCommon.h"
#include "TransfromComponent.h"
#include "ScriptComponent.h"

namespace nidhog
{
    namespace game_entity
    {
        //传递ID值
        DEFINE_TYPED_ID(entity_id);

        class entity
        {
        public:
            constexpr explicit entity(entity_id id) : _id{ id } {}
            constexpr entity() : _id{ id::invalid_id } {}
            constexpr entity_id get_id() const { return _id; }
            constexpr bool is_valid() const { return id::is_valid(_id); }//check id

            transform::component transform() const;
            script::component script() const;
        private:
            entity_id _id;
        };
    }
    
    namespace script
    {
        class entity_script:public game_entity::entity
        {
        public:
            virtual ~entity_script() = default;
            virtual void begin_play(){}//开始时更新脚本
            virtual void update(float){}//每个frame更新脚本,float为时间，second per frame
            protected:
            //基类，不想被其他访问并实例化，构造函数放在protected中
            constexpr explicit entity_script(game_entity::entity entity):game_entity::entity{entity.get_id()}{}
            
        };
        //不讲这些明确暴露给游戏代码，于是使用一个namespace
        namespace detail
        {
            //使用unique指针来定义该指针
            using script_ptr =std::unique_ptr<entity_script>;
            //需要将其作为参数传递给Component，所有定义一个基本函数指针类型
            using script_creator = script_ptr(*)(game_entity::entity entity);
            //注册函数所需的数据类型
            using string_hash = std::hash<std::string>;
            //注册函数
            u8 register_script(size_t, script_creator);

#ifdef USE_WITH_EDITOR
            extern "C" __declspec(dllexport)
#endif //USE_WITH_EDITOR
            script_creator get_script_creator(size_t tag);

            template<class script_class>
            script_ptr create_script(game_entity::entity entity)
            {
                assert(entity.is_valid());
                //创建一个脚本实例并返回一个指向脚本的指针
                return std::make_unique<script_class>(entity);
            }
#ifdef USE_WITH_EDITOR
            u8 add_script_name(const char* name);
            //用来自动帮我们填充register信息的宏
#define REGISTER_SCRIPT(TYPE)                                           \
        namespace {                                                     \
        const u8 _reg_##TYPE                                            \
        { nidhog::script::detail::register_script(                      \
              nidhog::script::detail::string_hash()(#TYPE),             \
              &nidhog::script::detail::create_script<TYPE>) };          \
        const u8 _name_##TYPE                                           \
        { nidhog::script::detail::add_script_name(#TYPE) };             \
        }                                                               

#else
#define REGISTER_SCRIPT(TYPE)                                           \
        namespace {                                                     \
        const u8 _reg_##TYPE                                            \
        { nidhog::script::detail::register_script(                      \
              nidhog::script::detail::string_hash()(#TYPE),             \
              &nidhog::script::detail::create_script<TYPE>) };          \
        }

#endif // USE_WITH_EDITOR
        }
    }
}
