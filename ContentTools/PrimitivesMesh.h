#pragma once
#include "ToolsCommon.h"

namespace nidhog::tools {

    //枚举这些基本网格体类型
    enum primitive_mesh_type : u32
    {
        plane,
        cube,
        uv_sphere,
        ico_sphere,
        cylinder,
        capsule,

        count
    };

    //定义这个struct，用来初始化图元网格体
    struct primitive_init_info
    {
        primitive_mesh_type type;
        //如何细分
        u32                 segments[3]{ 1, 1, 1 };
        math::v3            size{ 1, 1, 1 };
        //网格细分类型lod
        u32                 lod{ 0 };
    };

}
