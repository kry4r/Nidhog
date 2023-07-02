#pragma once

#include "D3D12CommonHeaders.h"

namespace nidhog::graphics::d3d12::shaders
{
	struct shader_types
	{
        enum type : u32 
        {
            vertex = 0,
            hull,
            domain,
            geometry,
            pixel,
            compute,
            amplification,
            mesh,

            count
        };
	};

    struct engine_shader 
    {
        enum id : u32 
        {
            fullscreen_triangle_vs = 0,
            fill_color_ps = 1,
            post_process = 2,

            count
        };
    };

    bool initialize();
    void shutdown();

    D3D12_SHADER_BYTECODE get_engine_shader(engine_shader::id id);
}