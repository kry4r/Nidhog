#pragma once

//实现API接口

#include "CommonHeaders.h"
#include "Renderer.h"
#include "Platform\Window.h"

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
			void(*render)(surface_id);
		} surface;


		struct 
		{
			id::id_type(*add_submesh)(const u8*&);
			void (*remove_submesh)(id::id_type);
		} resources;

		graphics_platform platform = (graphics_platform)-1;

	};
}