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

        using script_registry = std::unordered_map<size_t, detail::script_creator>;

        script_registry&registry()
        {
            // NOTE: ���ھ�̬���ݵĳ�ʼ��˳�����ǽ������̬�������ں�����
            //       ͨ�����ַ�ʽ�����ǿ���ȷ�������ڷ���֮ǰ�Ѿ���ʼ����
            static script_registry reg;
            return reg;
        }
#ifdef USE_WITH_EDITOR
        utl::vector<std::string>&
            script_names()
        {
            // NOTE: ���ھ�̬���ݵĳ�ʼ��˳�����ǽ������̬�������ں�����
            //       ͨ�����ַ�ʽ�����ǿ���ȷ�������ڷ���֮ǰ�Ѿ���ʼ����
            static utl::vector<std::string> names;
            return names;
        }
#endif

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
            bool result{ registry().insert(script_registry::value_type{tag, func}).second };
            assert(result);
            return result;
        }
        script_creator
            get_script_creator(size_t tag)
        {
            auto script = nidhog::script::registry().find(tag);
            assert(script != nidhog::script::registry().end() && script->first == tag);
            return script->second;
        }

#ifdef USE_WITH_EDITOR
        u8 add_script_name(const char* name)
        {
            script_names().emplace_back(name);
            return true;
        }
#endif // USE_WITH_EDITOR
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
        const id::id_type index{ (id::id_type)entity_scripts.size() };
        entity_scripts.emplace_back(info.script_creator(entity));
        assert(entity_scripts.back()->get_id() == entity.get_id());
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

#ifdef USE_WITH_EDITOR
#include <atlsafe.h>

extern "C" __declspec(dllexport)
LPSAFEARRAY get_script_names()
{
    const u32 size{ (u32)nidhog::script::script_names().size() };
    if (!size) return nullptr;
    CComSafeArray<BSTR> names(size);
    for (u32 i{ 0 }; i < size; ++i)
    {
        names.SetAt(i, A2BSTR_EX(nidhog::script::script_names()[i].c_str()), false);
    }
    return names.Detach();
}
#endif // USE_WITH_ED
