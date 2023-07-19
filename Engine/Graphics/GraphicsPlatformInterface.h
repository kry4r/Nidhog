#pragma once

//实现API接口

#include "CommonHeaders.h"
#include "Renderer.h"

namespace nidhog::graphics
{
	struct platform_interface
	{
		bool(*initialize)(void);
		void(*shutdown)(void);
		void(*render)(void);

		//调用low-level renderer
		struct 
		{
			surface(*create)(platform::window);
			void(*remove)(surface_id);
			void(*resize)(surface_id, u32, u32);
			u32(*width)(surface_id);
			u32(*height)(surface_id);
			void(*render)(surface_id, frame_info);
		} surface;

		struct 
		{
			light(*create)(light_init_info);
			void(*remove)(light_id, u64);
			void(*set_parameter)(light_id, u64, light_parameter::parameter, const void* const, u32);
			void(*get_parameter)(light_id, u64, light_parameter::parameter, void* const, u32);
		} light;

		struct 
		{
			camera(*create)(camera_init_info);
			void(*remove)(camera_id);
			//根据我们定义的参数不同，实现set和get相应Parameter
			void(*set_parameter)(camera_id, camera_parameter::parameter, const void* const, u32);
			void(*get_parameter)(camera_id, camera_parameter::parameter, void* const, u32);
		} camera;


		struct 
		{
			id::id_type(*add_submesh)(const u8*&);
			void (*remove_submesh)(id::id_type);
			id::id_type(*add_material)(material_init_info);
			void (*remove_material)(id::id_type);
			id::id_type(*add_render_item)(id::id_type, id::id_type, u32, const id::id_type* const);
			void (*remove_render_item)(id::id_type);
		} resources;

		graphics_platform platform = (graphics_platform)-1;

	};
}