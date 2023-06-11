#include "Entity.h"
#include "Transform.h"
#include "Script.h"

namespace nidhog::game_entity {

    namespace {
        //实现自己的vector等数据结构
        utl::vector<transform::component>       transforms;
        utl::vector<script::component>          scripts;

        utl::vector<id::generation_type>        generations;
        utl::deque<entity_id>                   free_ids;

    } // 匿名的namespace

    entity
        create(entity_info info)
    {
        assert(info.transform); // 所有游戏实体都必须有一个transform组件
        if (!info.transform) return entity{};

        entity_id id;
        //判断是否超过上限
        if (free_ids.size() > id::min_deleted_elements)
        {
            id = free_ids.front();
            assert(!is_alive(id));
            free_ids.pop_front();
            id = entity_id{ id::new_generation(id) };
            ++generations[id::index(id)];
        }
        else
        {
            id = entity_id{ (id::id_type)generations.size() };
            generations.push_back(0);
            //transforms.resize(generations.size())
            // 重新确定组件大小
            // NOTE: 不调用Resize（），所以内存分配数量始终保持较低
            transforms.emplace_back();
            scripts.emplace_back();
        }

        const entity new_entity{ id };
        const id::id_type index{ id::index(id) };

        // Create transform component
        assert(!transforms[index].is_valid());
        transforms[index] = transform::create(*info.transform, new_entity);
        if (!transforms[index].is_valid()) return {};
        // Create Script Component
        if (info.script && info.script->script_creator)
        {
            assert(!scripts[index].is_valid());
            scripts[index] = script::create(*info.script,new_entity);
            assert(scripts[index].is_valid());
        }
        return new_entity;
    }

    void
        remove(entity_id id)
    {
        const id::id_type index{ id::index(id) };
        assert(is_alive(id));
        //检查sripts是否存在
        if (scripts[index].is_valid())
        {
            script::remove(scripts[index]);
            scripts[index] = {};
        }

        transform::remove(transforms[index]);
        transforms[index] = {};
        free_ids.push_back(id);
    }

    bool
        is_alive(entity_id id)
    {
        //一个不断检查的过程
        assert(id::is_valid(id));
        const id::id_type index{ id::index(id) };
        assert(index < generations.size());
        return (generations[index] == id::generation(id) && transforms[index].is_valid());
    }

    transform::component
        entity::transform() const
    {
        assert(is_alive(_id));
        const id::id_type index{ id::index(_id) };
        return transforms[index];
    }
    script::component entity::script() const
    {
        assert(is_alive(_id));
        const id::id_type index{id::index(_id)};
        return scripts[index];
    }



}