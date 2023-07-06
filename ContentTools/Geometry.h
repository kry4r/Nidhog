#pragma once

#include "ToolsCommon.h"

namespace nidhog::tools
{
    namespace packed_vertex {
        //结构体基本属性
        struct vertex_static
        {
            math::v3    position;
            u8          reserved[3];
            u8          t_sign; // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1)
            u16         normal[2];
            u16         tangent[2];
            math::v2    uv;
        };
    }//packed_vertex

    struct vertex
    {
        math::v4 tangent{};
        math::v3 position{};
        math::v3 normal{};
        math::v2 uv{};
    };

    //用来在几何管线间传输数据
    struct mesh
    {
        // 初始化数据
        utl::vector<math::v3>               positions;
        utl::vector<math::v3>               normals;
        utl::vector<math::v4>               tangents;
        utl::vector<utl::vector<math::v2>>  uv_sets;
        utl::vector<u32>                    material_indices;
        utl::vector<u32>                    material_used;

        utl::vector<u32>                    raw_indices;

        // 中间数据
        utl::vector<vertex>                 vertices;
        utl::vector<u32>                    indices;

        // 输出数据
        std::string                         name;
        utl::vector<packed_vertex::vertex_static> packed_vertices_static;
        f32                                 lod_threshold{ -1.f };
        u32                                 lod_id{ u32_invalid_id };
    };


    struct lod_group
    {
        std::string         name;
        //定义了mesh的每个lod的级别
        utl::vector<mesh>   meshes;
    };
    struct scene
    {
        std::string             name;
        utl::vector<lod_group>  lod_groups;
    };

    //几何体导入方式
    struct geometry_import_settings
    {
        f32 smoothing_angle;
        //法线
        u8  calculate_normals;
        //切线
        u8  calculate_tangents;
        //左手系 ？ 右手系
        u8  reverse_handedness;
        //嵌入贴图导入
        u8  import_embeded_textures;
        //动画导入
        u8  import_animations;
    };

    //场景数据
    struct scene_data
    {
        u8* buffer;
        u32                      buffer_size;
        //几何体导入方式
        geometry_import_settings settings;
    };

    void process_scene(scene& scene, const geometry_import_settings& settings);
    void pack_data(const scene& scene, scene_data& data);
}
