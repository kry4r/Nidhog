#pragma once

#include "ToolsCommon.h"
//进行了一定的重构
namespace nidhog::tools
{
	struct vertex
	{
		math::v4									tangent{};
		//为之后导入骨骼动画做准备
		math::v4									joint_weights{};
		math::u32v4									joint_indices{ u32_invalid_id, u32_invalid_id, u32_invalid_id, u32_invalid_id };
		math::v3									position{};
		math::v3									normal{};
		math::v2									uv{};
		u8											red{}, green{}, blue{};
		u8											pad;
	};

	namespace elements {
		//指定在顶点格式中能够找到哪些信息
		struct elements_type {
			enum type : u32 {
				position_only = 0x00,
				static_normal = 0x01,
				static_normal_texture = 0x03,
				static_color = 0x04,
				skeletal = 0x08,
				skeletal_color = skeletal | static_color,
				skeletal_normal = skeletal | static_normal,
				skeletal_normal_color = skeletal_normal | static_color,
				skeletal_normal_texture = skeletal | static_normal_texture,
				skeletal_normal_texture_color = skeletal_normal_texture | static_color,
			};
		};

		struct static_color
		{
			u8          color[3];
			u8          pad;
		};

		struct static_normal
		{
			u8          color[3];
			u8          t_sign;     // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1).
			u16         normal[2];
		};

		struct static_normal_texture
		{
			u8          color[3];
			u8          t_sign;     // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1).
			u16         normal[2];
			u16         tangent[2];
			math::v2    uv;
		};
		//about skeleton animation，都有joint_weights/indices
		struct skeletal
		{
			u8          joint_weights[3]; // normalized joint weights for up to 4 joints.
			u8          pad;
			u16         joint_indices[4];
		};

		struct skeletal_color
		{
			u8          joint_weights[3]; // normalized joint weights for up to 4 joints.
			u8          pad;
			u16         joint_indices[4];
			u8          color[3];
			u8          pad2;
		};

		struct skeletal_normal
		{
			u8          joint_weights[3];   // normalized joint weights for up to 4 joints.
			u8          t_sign;             // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1).
			u16         joint_indices[4];
			u16         normal[2];
		};

		struct skeletal_normal_color
		{
			u8          joint_weights[3];   // normalized joint weights for up to 4 joints.
			u8          t_sign;             // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1).
			u16         joint_indices[4];
			u16         normal[2];
			u8          color[3];
			u8          pad;
		};

		struct skeletal_normal_texture
		{
			u8          joint_weights[3];   // normalized joint weights for up to 4 joints.
			u8          t_sign;             // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1).
			u16         joint_indices[4];
			u16         normal[2];
			u16         tangent[2];
			math::v2    uv;
		};

		struct skeletal_normal_texture_color
		{
			u8          joint_weights[3];   // normalized joint weights for up to 4 joints.
			u8          t_sign;             // bit 0: tangent handedness * (tangent.z sign), bit 1: normal.z sign (0 means -1, 1 means +1).
			u16         joint_indices[4];
			u16         normal[2];
			u16         tangent[2];
			math::v2    uv;
			u8          color[3];
			u8          pad;
		};

	} // namespace elements


    //用来在几何管线间传输数据
	//现在每个mesh有三个buffer：indices/position/element
    struct mesh
    {
        // 初始化数据
        utl::vector<math::v3>               positions;
        utl::vector<math::v3>               normals;
        utl::vector<math::v4>               tangents;
        utl::vector<math::v3>               colors;
        utl::vector<utl::vector<math::v2>>  uv_sets;
        utl::vector<u32>                    material_indices;
        utl::vector<u32>                    material_used;

        utl::vector<u32>                    raw_indices;

        // 中间数据
        utl::vector<vertex>                 vertices;
        utl::vector<u32>                    indices;

        // 输出数据
        std::string                         name;
        elements::elements_type::type       elements_type;
        utl::vector<u8>                     position_buffer;
        utl::vector<u8>                     element_buffer;

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
