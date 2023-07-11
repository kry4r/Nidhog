#pragma once

#include "CommonHeaders.h"
#include "..\Platform\Window.h"


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

    //包括所有可用api
    enum class graphics_platform :u32
    {
        direct3d12 = 0,
    };

    bool initialize(graphics_platform platform);
    void shutdown();



    surface create_surface(platform::window window);
    void remove_surface(surface_id id);

    id::id_type add_submesh(const u8*& data);
    void remove_submesh(id::id_type id);

    // 获取已编译引擎着色器相对于可执行文件路径的位置。
    // 该路径适用于当前使用的图形 API
    const char* get_engine_shaders_path();

    // 获取指定平台的已编译引擎着色器相对于可执行文件路径的位置
    // 该路径适用于当前使用的图形 API
    const char* get_engine_shaders_path(graphics_platform platform);
}