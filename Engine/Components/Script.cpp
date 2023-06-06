#include "Script.h"
#include "Entity.h"

namespace nidhog::script
{
    namespace 
    {
        //��Transform��������Ƶ����ݽṹ������
        //���ǽ���ʹ��˫��������ʹentity_scripts���ӽ���
        //����ʹ�ô�������ѭ����飬�ᵼ��cache miss�� Branch misprediction����
        //ʹ�ô��������ͬʱ��������һ���������飬���������ǽ��������ӳ��
        utl::vector<detail::script_ptr>     entity_scripts;
        utl::vector<id::id_type>            id_mapping;

        utl::vector<id::generation_type>    generations;
        utl::vector<script_id>              free_ids;

        using script_registery = std::unordered_map<size_t, detail::script_creator>;

        script_registery&registery()
        {
            // NOTE: ���ھ�̬���ݵĳ�ʼ��˳�����ǽ������̬�������ں�����
            //       ͨ�����ַ�ʽ�����ǿ���ȷ�������ڷ���֮ǰ�Ѿ���ʼ����
            static script_registery reg;
            return reg;
        }

        bool exists(script_id id)
        {
            //�ж�id�Ƿ���Ч
            assert(id::is_valid(id));
            //�õ�id����
            const id::id_type index{ id::index(id) };
            assert(index < generations.size() && id_mapping[index] < entity_scripts.size());
            //�ж�generation�Ƿ�һ��
            assert(generations[index] == id::generation(id));
            return (generations[index] == id::generation(id)) &&
                entity_scripts[id_mapping[index]] &&
                entity_scripts[id_mapping[index]]->is_valid();
        }
    }
    namespace detail
    {
        //ʹ�ù�ϣ����ע��
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
        //��entity�еļ������
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
        //ֱ����info����scriptʵ��
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
        //���н���������id_mapping�е�����
        id_mapping[id::index(last_id)] = index;
        id_mapping[id::index(id)] = id::invalid_id;
    }


}
