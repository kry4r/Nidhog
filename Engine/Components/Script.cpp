#include "Script.h"
#include "Entity.h"

namespace nidhog::script
{
    namespace 
    {
        //与Transform定义的类似的数据结构来管理
        //我们将会使用双重索引，使entity_scripts更加紧凑
        //若是使用带孔数组循环检查，会导致cache miss和 Branch misprediction问题
        //使用带孔数组的同时，引入另一个紧凑数组，带孔数组是紧凑数组的映射
        utl::vector<detail::script_ptr>     entity_scripts;
        utl::vector<id::id_type>            id_mapping;

        utl::vector<id::generation_type>    generations;
        utl::vector<script_id>              free_ids;

        using script_registery = std::unordered_map<size_t, detail::script_creator>;

        script_registery&registery()
        {
            // NOTE: 由于静态数据的初始化顺序，我们将这个静态变量放在函数中
            //       通过这种方式，我们可以确定数据在访问之前已经初始化。
            static script_registery reg;
            return reg;
        }

        bool exists(script_id id)
        {
            //判断id是否有效
            assert(id::is_valid(id));
            //得到id索引
            const id::id_type index{ id::index(id) };
            assert(index < generations.size() && id_mapping[index] < entity_scripts.size());
            //判断generation是否一致
            assert(generations[index] == id::generation(id));
            return (generations[index] == id::generation(id)) &&
                entity_scripts[id_mapping[index]] &&
                entity_scripts[id_mapping[index]]->is_valid();
        }
    }
    namespace detail
    {
        //使用哈希表来注册
        u8 register_script(size_t tag, script_creator func) 
        {
            bool result{ registery().insert(script_registery::value_type{tag, func}).second };
            assert(result);
            return result;
        }
    }
    component create(init_info info, game_entity::entity entity)
    {
        assert(entity.is_valid());
        assert(info.script_creator);
    
        script_id id{};
        //和entity中的检查类似
        if (free_ids.size() > id::min_deleted_elements)
        {
            id = free_ids.front();
            assert(!exists(id));
            free_ids.pop_back();
            id = script_id{ id::new_generation(id) };
            ++generations[id::index(id)];
        }
        else
        {
            id = script_id{ (id::id_type)id_mapping.size() };
            id_mapping.emplace_back();
            generations.push_back(0);
        }

        assert(id::is_valid(id));
        //直接用info创建script实例
        entity_scripts.emplace_back(info.script_creator(entity));
        assert(entity_scripts.back()->get_id() == entity.get_id());
        const id::id_type index{ (id::id_type)entity_scripts.size() };
        id_mapping[id::index(id)] = index;
        return component{ id };
    }
    void remove(component c)
    {
        assert(c.is_valid() && exists(c.get_id()));
        const script_id id{ c.get_id() };
        const id::id_type index{ id_mapping[id::index(id)] };
        const script_id last_id{ entity_scripts.back()->script().get_id() };
        utl::erase_unordered(entity_scripts, index);
        //进行交换后，重置id_mapping中的索引
        id_mapping[id::index(last_id)] = index;
        id_mapping[id::index(id)] = id::invalid_id;
    }


}
