#pragma once

#include "CommonHeaders.h"
#include "Platform/Window.h"
#include "EngineAPI/Camera.h"


namespace nidhog::graphics
{
    //类似window的构建
    DEFINE_TYPED_ID(surface_id);
    class surface
    {
    public:
        constexpr explicit surface(surface_id id) : _id{ id } {}
        constexpr surface() = default;
        constexpr surface_id get_id() const { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        void resize(u32 width, u32 height) const;
        u32 width() const;
        u32 height() const;
        void render() const;
    private:
        surface_id _id{ id::invalid_id };
    };

    struct render_surface
    {
        platform::window window{};
        surface surface{};
    };

    struct camera_parameter 
    {
        enum parameter : u32 
        {
            up_vector,
            field_of_view,
            aspect_ratio,
            view_width,
            view_height,
            near_z,
            far_z,
            view,
            projection,
            inverse_projection,
            view_projection,
            inverse_view_projection,
            type,
            entity_id,

            count
        };
    };

    struct camera_init_info
    {
        id::id_type     entity_id{ id::invalid_id };
        camera::type    type{};
        math::v3        up;
        union 
        {
            f32 field_of_view;
            f32 view_width;
        };
        union 
        {
            f32 aspect_ratio;
            f32 view_height;
        };
        f32 near_z;
        f32 far_z;
    };

    struct perspective_camera_init_info : public camera_init_info
    {
        explicit perspective_camera_init_info(id::id_type id)
        {
            assert(id::is_valid(id));
            entity_id = id;
            type = camera::perspective;
            up = { 0.f, 1.f, 0.f };
            field_of_view = 0.25f;
            aspect_ratio = 16.f / 10.f;
            near_z = 0.001f;
            far_z = 10000.f;
        }
    };

    struct orthographic_camera_init_info : public camera_init_info
    {
        explicit orthographic_camera_init_info(id::id_type id)
        {
            assert(id::is_valid(id));
            entity_id = id;
            type = camera::orthographic;
            up = { 0.f, 1.f, 0.f };
            view_width = 1920;
            view_height = 1080;
            near_z = 0.001f;
            far_z = 10000.f;
        }
    };

    struct primitve_topology {
        enum type : u32 {
            point_list = 1,
            line_list,
            line_strip,
            triangle_list,
            triangle_strip,

            count
        };
    };

#ifndef NIDHOG_PLUS
    //包括所有可用api
    enum class graphics_platform :u32
    {
        direct3d12 = 0,
    };
#else
#include "Graphics/GraphicsPlatform.h"
#endif

    bool initialize(graphics_platform platform);
    void shutdown();



    surface create_surface(platform::window window);
    void remove_surface(surface_id id);

    camera create_camera(camera_init_info info);
    void remove_camera(camera_id id);

    id::id_type add_submesh(const u8*& data);
    void remove_submesh(id::id_type id);

    // 获取已编译引擎着色器相对于可执行文件路径的位置。
    // 该路径适用于当前使用的图形 API
    const char* get_engine_shaders_path();

    // 获取指定平台的已编译引擎着色器相对于可执行文件路径的位置
    // 该路径适用于当前使用的图形 API
    const char* get_engine_shaders_path(graphics_platform platform);
}