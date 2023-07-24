#pragma once

#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace nidhog::graphics::d3d12 {
    namespace camera { class d3d12_camera; }

    struct d3d12_frame_info
    {
        const frame_info*           info{ nullptr };
        camera::d3d12_camera*       camera{ nullptr };
        D3D12_GPU_VIRTUAL_ADDRESS   global_shader_data{ 0 };
        u32                         surface_width{ 0 };
        u32                         surface_height{ 0 };
        id::id_type                 light_culling_id{ id::invalid_id };
        u32                         frame_index{ 0 };
        f32                         delta_time{ 16.7f };
    };
}


namespace nidhog::graphics::d3d12::core 
{

    bool initialize();
    void shutdown();

    template<typename T>
    constexpr void release(T*& resource)
    {
        if (resource)
        {
            resource->Release();
            resource = nullptr;
        }
    }
    namespace detail {
        void deferred_release(IUnknown* resource);
    }

    template<typename T>
    constexpr void deferred_release(T*& resource)
    {
        if (resource)
        {
            //将指针添加到数组，之后再进行release
            detail::deferred_release(resource);
            resource = nullptr;
        }
    }

    //获取Main device
    id3d12_device *const device();
    
    [[nodiscard]] descriptor_heap& rtv_heap();    //Render Target View
    [[nodiscard]] descriptor_heap& dsv_heap();    //deapth VIew
    [[nodiscard]] descriptor_heap& srv_heap();    //shader resource View
    [[nodiscard]] descriptor_heap& uav_heap();    //unordered access View
    [[nodiscard]] constant_buffer& cbuffer();
    [[nodiscard]] u32 current_frame_index();
    void set_deferred_releases_flag();

    //一些surface相关函数
    [[nodiscard]] surface create_surface(platform::window window);
    void remove_surface(surface_id id);
    void resize_surface(surface_id id, u32, u32);
    [[nodiscard]] u32 surface_width(surface_id id);
    [[nodiscard]] u32 surface_height(surface_id id);
    void render_surface(surface_id id, frame_info info);

}