#pragma once
#include "ToolsCommon.h"

namespace nidhog::tools {

    //ö����Щ��������������
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

    //�������struct��������ʼ��ͼԪ������
    struct primitive_init_info
    {
        primitive_mesh_type type;
        //���ϸ��
        u32                 segments[3]{ 1, 1, 1 };
        math::v3            size{ 1, 1, 1 };
        //����ϸ������lod
        u32                 lod{ 0 };
    };

}
