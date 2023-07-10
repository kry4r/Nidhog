#include "FbxImporter.h"
#include "Geometry.h"

// ��������κ�linker������ȷ�����漸��
// 1) FBX SDK 2020.2 ���߸��µİ汾
// 2) ��ȷinclude fbxsdk.h
// 3) ����ĵ�ַȷ����ȷ
#if _DEBUG
#pragma comment (lib, "D:\\FBX_SDK_2020_2\\lib\\vs2019\\x64\\debug\\libfbxsdk-md.lib")
#pragma comment (lib, "D:\\FBX_SDK_2020_2\\lib\\vs2019\\x64\\debug\\libxml2-md.lib")
#pragma comment (lib, "D:\\FBX_SDK_2020_2\\lib\\vs2019\\x64\\debug\\zlib-md.lib")
#else
#pragma comment (lib, "D:\\FBX_SDK_2020_2\\lib\\vs2019\\x64\\release\\libfbxsdk-md.lib")
#pragma comment (lib, "D:\\FBX_SDK_2020_2\\lib\\vs2019\\x64\\release\\libxml2-md.lib")
#pragma comment (lib, "D:\\FBX_SDK_2020_2\\lib\\vs2019\\x64\\release\\zlib-md.lib")
#endif
// LNK4099 PDB ����ͨ�� installing FBX SDK PDBs (separate download) ����
// ֱ�ӽ���������������

namespace nidhog::tools {
    namespace {

        std::mutex fbx_mutex{};

    } // ����namesapce

    bool fbx_context::initialize_fbx()
    {
        assert(!is_valid());

        _fbx_manager = FbxManager::Create();
        if (!_fbx_manager)
        {
            return false;
        }
        //Determine which parts to import
        FbxIOSettings* ios{ FbxIOSettings::Create(_fbx_manager, IOSROOT) };
        assert(ios);
        _fbx_manager->SetIOSettings(ios);

        return true;
    }

    void fbx_context::load_fbx_file(const char* file)
    {
        assert(_fbx_manager && !_fbx_scene);
        _fbx_scene = FbxScene::Create(_fbx_manager, "Importer Scene");
        if (!_fbx_scene)
        {
            return;
        }

        FbxImporter* importer{ FbxImporter::Create(_fbx_manager, "Importer") };
        if (!(importer &&
            importer->Initialize(file, -1, _fbx_manager->GetIOSettings()) &&
            importer->Import(_fbx_scene)))
        {
            return;
        }

        importer->Destroy();

        // Get scene scale in meters.
        _scene_scale = (f32)_fbx_scene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m);
    }

    void fbx_context::get_scene(FbxNode* root /*= nullptr*/)
    {
        assert(is_valid());

        if (!root)
        {
            root = _fbx_scene->GetRootNode();
            if (!root) return;
        }

        const s32 num_nodes{ root->GetChildCount() };
        for (s32 i{ 0 }; i < num_nodes; ++i)
        {
            FbxNode* node{ root->GetChild(i) };
            if (!node) continue;
            //��������child-node����ȡÿ���ڵ�LOD�е�Hierarchies
            lod_group lod{};
            get_meshes(node, lod.meshes, 0, -1.f);
            if (lod.meshes.size())
            {
                lod.name = lod.meshes[0].name;
                _scene->lod_groups.emplace_back(lod);
            }
        }
    }
    //ʹ��һ��vector������
    void fbx_context::get_meshes(FbxNode* node, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_threshold)
    {
        assert(node && lod_id != u32_invalid_id);
        bool is_lod_group{ false };

        if (const s32 num_attributes{ node->GetNodeAttributeCount() })
        {
            for (s32 i{ 0 }; i < num_attributes; ++i)
            {
                FbxNodeAttribute* attribute{ node->GetNodeAttributeByIndex(i) };
                const FbxNodeAttribute::EType attribute_type{ attribute->GetAttributeType() };
                //���Attribute���������Ͳ�ͬ���ò��ú���
                if (attribute_type == FbxNodeAttribute::eMesh)
                {
                    get_mesh(attribute, meshes, lod_id, lod_threshold);
                }
                else if (attribute_type == FbxNodeAttribute::eLODGroup)
                {
                    get_lod_group(attribute);
                    is_lod_group = true;
                }
            }
        }

        //���Ǽ���mesh�����ԣ�Ҫô����һ��LOD attribute��Ҫô����mesh attribute
        //�������LOD group�Ļ�������������ӽڵ㣬Ѱ����Mesh���Ƿ������������mesh�����ṹ���壩
        if (!is_lod_group)
        {
            if (const s32 num_children{ node->GetChildCount() })
            {
                for (s32 i{ 0 }; i < num_children; ++i)
                {
                    get_meshes(node->GetChild(i), meshes, lod_id, lod_threshold);
                }
            }
        }
    }

    //Ϊ��ȡmesh�������ǻ��贫��LOD��ֵ 
    //����mesh������ʹ��attribute
    void fbx_context::get_mesh(FbxNodeAttribute* attribute, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_threshold)
    {
        assert(attribute);

        FbxMesh* fbx_mesh{ (FbxMesh*)attribute };
        if (fbx_mesh->RemoveBadPolygons() < 0) return;

        // Triangulate the mesh if needed.
        FbxGeometryConverter gc{ _fbx_manager };
        fbx_mesh = (FbxMesh*)gc.Triangulate(fbx_mesh, true);
        if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0) return;

        FbxNode* const node{ fbx_mesh->GetNode() };

        mesh m;
        m.lod_id = lod_id;
        m.lod_threshold = lod_threshold;
        m.name = (node->GetName()[0] != '\0') ? node->GetName() : fbx_mesh->GetName();

        if (get_mesh_data(fbx_mesh, m))
        {
            meshes.emplace_back(m);
        }
    }


    void fbx_context::get_lod_group(FbxNodeAttribute* attribute)
    {
        assert(attribute);

        FbxLODGroup* lod_grp{ (FbxLODGroup*)attribute };
        FbxNode* const node{ lod_grp->GetNode() };
        lod_group lod{};
        lod.name = (node->GetName()[0] != '\0') ? node->GetName() : lod_grp->GetName();
        // NOTE: number of LODs is exclusive the base mesh (LOD 0)
        const s32 num_nodes{ node->GetChildCount() };
        assert(num_nodes > 0 && lod_grp->GetNumThresholds() == (num_nodes - 1));

        for (s32 i{ 0 }; i < num_nodes; ++i)
        {
            f32 lod_threshold{ -1.f };
            if (i > 0)
            {
                FbxDistance threshold;
                lod_grp->GetThreshold(i - 1, threshold);
                lod_threshold = threshold.value() * _scene_scale;
            }
            //����ֻ��Ҫ���ӽڵ��ϵ���get_meshes�Ϳ��Ի�ȡ���������֧���������ݣ���ø�Ϊ��
            get_meshes(node->GetChild(i), lod.meshes, (u32)lod.meshes.size(), lod_threshold);
        }

        if (lod.meshes.size()) _scene->lod_groups.emplace_back(lod);
    }


    bool fbx_context::get_mesh_data(FbxMesh* fbx_mesh, mesh& m)
    {
        assert(fbx_mesh);

        //�趨����Transfrom�任���
        FbxNode* const node{ fbx_mesh->GetNode() };
        FbxAMatrix geometricTransform;

        geometricTransform.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
        geometricTransform.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
        geometricTransform.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));

        FbxAMatrix transform{ node->EvaluateGlobalTransform() * geometricTransform };
        //ת�þ����޸���Ӱ����
        FbxAMatrix inverse_transpose{ transform.Inverse().Transpose() };

        const s32 num_polys{ fbx_mesh->GetPolygonCount() };
        if (num_polys <= 0) return false;

        // Get vertices
        const s32 num_vertices{ fbx_mesh->GetControlPointsCount() };
        FbxVector4* vertices{ fbx_mesh->GetControlPoints() };
        const s32 num_indices{ fbx_mesh->GetPolygonVertexCount() };
        s32* indices{ fbx_mesh->GetPolygonVertices() };

        assert(num_vertices > 0 && vertices && num_indices > 0 && indices);
        if (!(num_vertices > 0 && vertices && num_indices > 0 && indices)) return false;

        m.raw_indices.resize(num_indices);
        utl::vector vertex_ref(num_vertices, u32_invalid_id);

        for (s32 i{ 0 }; i < num_indices; ++i)
        {
            const u32 v_idx{ (u32)indices[i] };
            // ���������������㣬��ֻ��Ҫ�������index
            // ���û������������µ�index���ϻ���
            if (vertex_ref[v_idx] != u32_invalid_id)
            {
                m.raw_indices[i] = vertex_ref[v_idx];
            }
            else
            {
                FbxVector4 v = transform.MultT(vertices[v_idx]) * _scene_scale;
                m.raw_indices[i] = (u32)m.positions.size();
                vertex_ref[v_idx] = m.raw_indices[i];
                m.positions.emplace_back((f32)v[0], (f32)v[1], (f32)v[2]);
            }
        }

        assert(m.raw_indices.size() % 3 == 0);

        // ��ȡÿ��model�Ĳ����������Ա�����֮�󰴲��ʲ��mesh
        assert(num_polys > 0);
        FbxLayerElementArrayTemplate<s32>* mtl_indices;
        if (fbx_mesh->GetMaterialIndices(&mtl_indices))
        {
            for (s32 i{ 0 }; i < num_polys; ++i)
            {
                const s32 mtl_index{ mtl_indices->GetAt(i) };
                assert(mtl_index >= 0);
                m.material_indices.emplace_back((u32)mtl_index);
                if (std::find(m.material_used.begin(), m.material_used.end(), (u32)mtl_index) == m.material_used.end())
                {
                    m.material_used.emplace_back((u32)mtl_index);
                }
            }
        }

        // Importing normals is ON by default
        const bool import_normals{ !_scene_data->settings.calculate_normals };
        // Importing tangents is OFF by default
        const bool import_tangents{ !_scene_data->settings.calculate_tangents };

        // Import normals
        if (import_normals)
        {
            FbxArray<FbxVector4> normals;
            // Calculate normals using FBX's built-in method, but only if no normal data is already there.
            if (fbx_mesh->GenerateNormals() &&
                fbx_mesh->GetPolygonVertexNormals(normals) && normals.Size() > 0)
            {
                const s32 num_normals{ normals.Size() };
                for (s32 i{ 0 }; i < num_normals; ++i)
                {
                    FbxVector4 n{ inverse_transpose.MultT(normals[i]) };
                    n.Normalize();
                    m.normals.emplace_back((f32)n[0], (f32)n[1], (f32)n[2]);
                }
            }
            else
            {
                // something went wrong with importing normals from FBX.
                // Fall back to our own normal calculation method.
                _scene_data->settings.calculate_normals = true;
            }
        }

        // Import tangents
        if (import_tangents)
        {
            FbxLayerElementArrayTemplate<FbxVector4>* tangents{ nullptr };
            // Calculate tangents using FBX's built-in method, but only if no tangent data is already there.
            if (fbx_mesh->GenerateTangentsData() &&
                fbx_mesh->GetTangents(&tangents) &&
                tangents && tangents->GetCount() > 0)
            {
                const s32 num_tangent{ tangents->GetCount() };
                for (s32 i{ 0 }; i < num_tangent; ++i)
                {
                    // TODO: not sure if this transformation is correct.
                    FbxVector4 t{ tangents->GetAt(i) };
                    const f32 handedness{ (f32)t[3] };
                    t[3] = 0.0;
                    t = transform.MultT(t);
                    t.Normalize();
                    m.tangents.emplace_back((f32)t[0], (f32)t[1], (f32)t[2], handedness);
                }
            }
            else
            {
                // something went wrong with importing tangents from FBX.
                // Fall back to our own tangent calculation method.
                _scene_data->settings.calculate_tangents = true;
            }
        }

        // Get UVs
        FbxStringList uv_names;
        fbx_mesh->GetUVSetNames(uv_names);
        const s32 uv_set_count{ uv_names.GetCount() };
        // NOTE: it's ok if we don't have a uv set. For example, some emissive objects don't need a uv map
        m.uv_sets.resize(uv_set_count);

        for (s32 i{ 0 }; i < uv_set_count; ++i)
        {
            FbxArray<FbxVector2> uvs;
            if (fbx_mesh->GetPolygonVertexUVs(uv_names.GetStringAt(i), uvs))
            {
                const s32 num_uvs{ uvs.Size() };
                for (s32 j{ 0 }; j < num_uvs; ++j)
                {
                    m.uv_sets[i].emplace_back((f32)uvs[j][0], (f32)uvs[j][1]);
                }
            }
        }

        return true;
    }

    //����FBX context���������ݣ�pack�������ݲ����ظ�editor
    EDITOR_INTERFACE void ImportFbx(const char* file, scene_data* data)
    {
        assert(file && data);
        scene scene{};

        // NOTE: �κ��漰ʹ�� FBX SDK �����ݶ�Ӧ���ǵ��̵߳ģ����߳�֧�ֲ����У�
        {
            std::lock_guard lock{ fbx_mutex };
            fbx_context fbx_context{ file, &scene, data };
            if (fbx_context.is_valid())
            {
                fbx_context.get_scene();
            }
            else
            {
                // TODO: send failure log message to editor
                return;
            }
        }

        process_scene(scene, data->settings);
        pack_data(scene, *data);
    }
}
