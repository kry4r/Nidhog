#pragma once

#include "..\Components\ComponentsCommon.h"
#include "TransfromComponent.h"
#include "ScriptComponent.h"

namespace nidhog
{
    namespace game_entity
    {
        //����IDֵ
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
            virtual void begin_play(){}//��ʼʱ���½ű�
            virtual void update(float){}//ÿ��frame���½ű�,floatΪʱ�䣬second per frame
            protected:
            //���࣬���뱻�������ʲ�ʵ���������캯������protected��
            constexpr explicit entity_script(game_entity::entity entity):game_entity::entity{entity.get_id()}{}
            
        };
        //������Щ��ȷ��¶����Ϸ���룬����ʹ��һ��namespace
        namespace detail
        {
            //ʹ��uniqueָ���������ָ��
            using script_ptr =std::unique_ptr<entity_script>;
            //��Ҫ������Ϊ�������ݸ�Component�����ж���һ����������ָ������
            using script_creator = script_ptr(*)(game_entity::entity entity);
            //ע�ắ���������������
            using string_hash = std::hash<std::string>;
            //ע�ắ��
            u8 register_script(size_t, script_creator);

#ifdef USE_WITH_EDITOR
            extern "C" __declspec(dllexport)
#endif //USE_WITH_EDITOR
            script_creator get_script_creator(size_t tag);

            template<class script_class>
            script_ptr create_script(game_entity::entity entity)
            {
                assert(entity.is_valid());
                //����һ���ű�ʵ��������һ��ָ��ű���ָ��
                return std::make_unique<script_class>(entity);
            }
#ifdef USE_WITH_EDITOR
            u8 add_script_name(const char* name);
            //�����Զ����������register��Ϣ�ĺ�
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
