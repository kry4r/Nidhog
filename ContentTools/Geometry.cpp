#include "Geometry.h"


namespace nidhog::tools
{
	namespace
	{
        using namespace math;
        using namespace DirectX;

        //������������������˵õ�����
        void recalculate_normals(mesh& m)
        {
            const u32 num_indices{ (u32)m.raw_indices.size() };
            m.normals.resize(num_indices);

            for (u32 i{ 0 }; i < num_indices; ++i)
            {
                const u32 i0{ m.raw_indices[i] };
                const u32 i1{ m.raw_indices[++i] };
                const u32 i2{ m.raw_indices[++i] };

                XMVECTOR v0{ XMLoadFloat3(&m.positions[i0]) };
                XMVECTOR v1{ XMLoadFloat3(&m.positions[i1]) };
                XMVECTOR v2{ XMLoadFloat3(&m.positions[i2]) };

                XMVECTOR e0{ v1 - v0 };
                XMVECTOR e1{ v2 - v0 };
                XMVECTOR n{ XMVector3Normalize(XMVector3Cross(e0, e1)) };

                XMStoreFloat3(&m.normals[i], n);
                m.normals[i - 1] = m.normals[i];
                m.normals[i - 2] = m.normals[i];
            }
        }

        //ֻ�Ǽ򵥸��ߺ�������Щ�Ƕ�Ӧ�ñ���Ϊ��soft����Щ��hard
        //��Ҫʹ��cos���жϣ������
        void process_normals(mesh& m, f32 smoothing_angle)
        {
            const f32 cos_alpha{ XMScalarCos(pi - smoothing_angle * pi / 180.f) };
            const bool is_hard_edge{ XMScalarNearEqual(smoothing_angle, 180.f, epsilon) };
            const bool is_soft_edge{ XMScalarNearEqual(smoothing_angle, 0.f, epsilon) };
            const u32 num_indices{ (u32)m.raw_indices.size() };
            const u32 num_vertices{ (u32)m.positions.size() };
            assert(num_indices && num_vertices);

            m.indices.resize(num_indices);

            //Ϊ�ӿ��ٶȣ�ֻ����һ������
            utl::vector<utl::vector<u32>> idx_ref(num_vertices);
            for (u32 i{ 0 }; i < num_indices; ++i)
                idx_ref[m.raw_indices[i]].emplace_back(i);

            for (u32 i{ 0 }; i < num_vertices; ++i)
            {
                auto& refs{ idx_ref[i] };
                u32 num_refs{ (u32)refs.size() };
                for (u32 j{ 0 }; j < num_refs; ++j)
                {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    //���ض��¶��������
                    vertex& v{ m.vertices.emplace_back() };
                    v.position = m.positions[m.raw_indices[refs[j]]];

                    //�ж��Ƿ�������εĶ������ƽ��
                    XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
                    if (!is_hard_edge)
                    {
                        for (u32 k{ j + 1 }; k < num_refs; ++k)
                        {
                            // ��ֵ��ʾ����֮��Ƕȵ����ҡ�
                            f32 cos_theta{ 0.f };
                            XMVECTOR n2{ XMLoadFloat3(&m.normals[refs[k]]) };
                            if (!is_soft_edge)
                            {
                                // NOTE: ��������������п�����n1�ĳ���
                                //       ��Ϊ�����������ѭ�������з����仯��
                                //       ���Ǽٶ�n2Ϊ��λ����
                                //       cos(angle) = dot(n1, n2) / (||n1||*||n2||)
                                XMStoreFloat(&cos_theta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
                            }

                            //��鷨��֮��н��ж��Ƿ�����ӻ�ֿ�

                            //��������������ƽ��
                            if (is_soft_edge || cos_theta >= cos_alpha)
                            {
                                n1 += n2;
                                //ͬ������
                                m.indices[refs[k]] = m.indices[refs[j]];
                                refs.erase(refs.begin() + k);
                                //������������
                                --num_refs;
                                --k;
                            }
                        }
                    }
                    XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
                }
            }
        }

        void process_uvs(mesh& m)
        {
            //��žɶ���
            utl::vector<vertex> old_vertices;
            old_vertices.swap(m.vertices);
            utl::vector<u32> old_indices(m.indices.size());
            old_indices.swap(m.indices);

            const u32 num_vertices{ (u32)old_vertices.size() };
            const u32 num_indices{ (u32)old_indices.size() };
            assert(num_vertices && num_indices);

            //Ϊ�ӿ��ٶȣ�ֻ����һ����������ÿ�����㣩
            utl::vector<utl::vector<u32>> idx_ref(num_vertices);
            for (u32 i{ 0 }; i < num_indices; ++i)
                idx_ref[old_indices[i]].emplace_back(i);

            for (u32 i{ 0 }; i < num_vertices; ++i)
            {
                auto& refs{ idx_ref[i] };
                u32 num_refs{ (u32)refs.size() };
                for (u32 j{ 0 }; j < num_refs; ++j)
                {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    vertex& v{ old_vertices[old_indices[refs[j]]] };
                    v.uv = m.uv_sets[0][refs[j]];
                    m.vertices.emplace_back(v);

                    //��ֶ���
                    for (u32 k{ j + 1 }; k < num_refs; ++k)
                    {
                        v2& uv1{ m.uv_sets[0][refs[k]] };
                        if (XMScalarNearEqual(v.uv.x, uv1.x, epsilon) &&
                            XMScalarNearEqual(v.uv.y, uv1.y, epsilon))
                        {
                            m.indices[refs[k]] = m.indices[refs[j]];
                            refs.erase(refs.begin() + k);
                            --num_refs;
                            --k;
                        }
                    }
                }
            }
        }


        //�����������
        void pack_vertices_static(mesh& m)
        {
            const u32 num_vertices{ (u32)m.vertices.size() };
            assert(num_vertices);
            m.packed_vertices_static.reserve(num_vertices);

            for (u32 i{ 0 }; i < num_vertices; ++i)
            {
                vertex& v{ m.vertices[i] };
                const u8 signs{ (u8)((v.normal.z > 0.f) << 1) };
                const u16 normal_x{ (u16)pack_float<16>(v.normal.x, -1.f, 1.f) };
                const u16 normal_y{ (u16)pack_float<16>(v.normal.y, -1.f, 1.f) };
                // TODO: ͨ����־����������������Ϣ�����Geometry.h����X/Y�������z���������
                m.packed_vertices_static
                    .emplace_back(packed_vertex::vertex_static
                        {
                            v.position, {0, 0, 0}, signs,
                            {normal_x, normal_y}, {},
                            v.uv
                        });
            }
        }

        //Ҫ��ÿ��������������������uv�Ƕ������ζ��Ե�
        void process_vertices(mesh& m, const geometry_import_settings& settings)
        {
            assert((m.raw_indices.size() % 3) == 0);
            if (settings.calculate_normals || m.normals.empty())
            {
                //���û�з�������㷨��
                recalculate_normals(m);
            }
            //֮������
            process_normals(m, settings.smoothing_angle);

            if (!m.uv_sets.empty())
            {
                //���uv��Ϊ�գ�����uv
                process_uvs(m);
            }

            //����ɫ���������ķ�ʽ�����������
            pack_vertices_static(m);
        }

        u64 get_mesh_size(const mesh& m)
        {
            const u64 num_vertices{ m.vertices.size() };
            const u64 vertex_buffer_size{ sizeof(packed_vertex::vertex_static) * num_vertices };
            const u64 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            const u64 index_buffer_size{ index_size * m.indices.size() };
            constexpr u64 su32{ sizeof(u32) };
            const u64 size{
                su32 + m.name.size() + // mesh�㼶�����Լ�����ص�
                su32 + // lod id
                su32 + // �����С
                su32 + // ������
                su32 + // ����size (16 bit or 32 bit)
                su32 + // ��������
                sizeof(f32) + // LOD��ֵ
                vertex_buffer_size + // room for vertices
                index_buffer_size // room for indices
            };

            return size;
        }

        u64 get_scene_size(const scene& scene)
        {
            constexpr u64 su32{ sizeof(u32) };
            u64 size
            {
                su32 +              // ���ֳ���
                scene.name.size() + // ���������ַ�������ص�
                su32                // LOD�㼶
            };

            for (auto& lod : scene.lod_groups)
            {
                u64 lod_size
                {
                    su32 + lod.name.size() + // LOD�㼶�����Լ�����ص�
                    su32                     // ��LOD��mesh�ı��
                };

                for (auto& m : lod.meshes)
                {
                    //����mesh��Ҫ���ٿռ�
                    lod_size += get_mesh_size(m);
                }

                size += lod_size;
            }

            return size;
        }

        void pack_mesh_data(const mesh& m, u8* const buffer, u64& at)
        {
            constexpr u64 su32{ sizeof(u32) };
            u32 s{ 0 };
            // mesh name
            s = (u32)m.name.size();
            memcpy(&buffer[at], &s, su32); at += su32;
            memcpy(&buffer[at], m.name.c_str(), s); at += s;
            // lod id
            s = m.lod_id;
            memcpy(&buffer[at], &s, su32); at += su32;
            // vertex size
            constexpr u32 vertex_size{ sizeof(packed_vertex::vertex_static) };
            s = vertex_size;
            memcpy(&buffer[at], &s, su32); at += su32;
            // number of vertices
            const u32 num_vertices{ (u32)m.vertices.size() };
            s = num_vertices;
            memcpy(&buffer[at], &s, su32); at += su32;
            // index size (16 bit or 32 bit)
            const u32 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            s = index_size;
            memcpy(&buffer[at], &s, su32); at += su32;
            // number of indices
            const u32 num_indices{ (u32)m.indices.size() };
            s = num_indices;
            memcpy(&buffer[at], &s, su32); at += su32;
            // LOD threshold
            memcpy(&buffer[at], &m.lod_threshold, sizeof(f32)); at += sizeof(f32);
            // vertex data
            s = vertex_size * num_vertices;
            memcpy(&buffer[at], m.packed_vertices_static.data(), s); at += s;
            // index data
            s = index_size * num_indices;
            void* data{ (void*)m.indices.data() };
            utl::vector<u16> indices;

            if (index_size == sizeof(u16))
            {
                indices.resize(num_indices);
                for (u32 i{ 0 }; i < num_indices; ++i) indices[i] = (u16)m.indices[i];
                data = (void*)indices.data();
            }
            memcpy(&buffer[at], data, s); at += s;
        }


        bool split_meshes_by_material(u32 material_idx, const mesh& m, mesh& submesh)
        {
            submesh.name = m.name;
            submesh.lod_threshold = m.lod_threshold;
            submesh.lod_id = m.lod_id;
            submesh.material_used.emplace_back(material_idx);
            submesh.uv_sets.resize(m.uv_sets.size());

            const u32 num_polys{ (u32)m.raw_indices.size() / 3 };
            utl::vector<u32> vertex_ref(m.positions.size(), u32_invalid_id);

            for (u32 i{ 0 }; i < num_polys; ++i)
            {
                const u32 mtl_idx{ m.material_indices[i] };
                if (mtl_idx != material_idx) continue;

                const u32 index{ i * 3 };
                for (u32 j = index; j < index + 3; ++j)
                {
                    const u32 v_idx{ m.raw_indices[j] };
                    if (vertex_ref[v_idx] != u32_invalid_id)
                    {
                        submesh.raw_indices.emplace_back(vertex_ref[v_idx]);
                    }
                    else
                    {
                        submesh.raw_indices.emplace_back((u32)submesh.positions.size());
                        vertex_ref[v_idx] = submesh.raw_indices.back();
                        submesh.positions.emplace_back(m.positions[v_idx]);
                    }

                    if (m.normals.size())
                    {
                        submesh.normals.emplace_back(m.normals[j]);
                    }

                    if (m.tangents.size())
                    {
                        submesh.tangents.emplace_back(m.tangents[j]);
                    }

                    for (u32 k{ 0 }; k < m.uv_sets.size(); ++k)
                    {
                        if (m.uv_sets[k].size())
                        {
                            submesh.uv_sets[k].emplace_back(m.uv_sets[k][j]);
                        }
                    }
                }
            }

            assert((submesh.raw_indices.size() % 3) == 0);
            return !submesh.raw_indices.empty();
        }

        void split_meshes_by_material(scene& scene)
        {
            for (auto& lod : scene.lod_groups)
            {
                utl::vector<mesh> new_meshes;

                for (auto& m : lod.meshes)
                {
                    // If more than one material is used in this mesh
                    // then split it into submeshes.
                    const u32 num_materials{ (u32)m.material_used.size() };
                    if (num_materials > 1)
                    {
                        for (u32 i{ 0 }; i < num_materials; ++i)
                        {
                            mesh submesh{};
                            if (split_meshes_by_material(m.material_used[i], m, submesh))
                            {
                                new_meshes.emplace_back(submesh);
                            }
                        }
                    }
                    else
                    {
                        new_meshes.emplace_back(m);
                    }
                }

                new_meshes.swap(lod.meshes);
            }
        }


	}//����namespace
    void process_scene(scene& scene, const geometry_import_settings& settings)
    {
        split_meshes_by_material(scene);
        //��������������
        for (auto& lod : scene.lod_groups)
            for (auto& m : lod.meshes)
            {
                process_vertices(m, settings);
            }
    }


    void pack_data(const scene& scene, scene_data& data)
    {
        constexpr u64 su32{ sizeof(u32) };
        //��ȡscene��С
        const u64 scene_size{ get_scene_size(scene) };
        //������buffer�У������ڴ��Դ���editor
        data.buffer_size = (u32)scene_size;
        data.buffer = (u8*)CoTaskMemAlloc(scene_size);
        assert(data.buffer);
        
        u8* const buffer{ data.buffer };
        u64 at{ 0 };//ÿ����ӵ�ƫ����
        u32 s{ 0 };//��ʱ���������ڽ�����u32��Ա����д��buffer
        
        // scene name
        s = (u32)scene.name.size();
        memcpy(&buffer[at], &s, su32); at += su32;
        memcpy(&buffer[at], scene.name.c_str(), s); at += s;
        // number of LODs
        s = (u32)scene.lod_groups.size();
        memcpy(&buffer[at], &s, su32); at += su32;
        
        for (auto& lod : scene.lod_groups)
        {
            // LOD name
            s = (u32)lod.name.size();
            memcpy(&buffer[at], &s, su32); at += su32;
            memcpy(&buffer[at], lod.name.c_str(), s); at += s;
            // number of meshes in this LOD
            s = (u32)lod.meshes.size();
            memcpy(&buffer[at], &s, su32); at += su32;
        
            for (auto& m : lod.meshes)
            {
                pack_mesh_data(m, buffer, at);
            }
        }
    }
}