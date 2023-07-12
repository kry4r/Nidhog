#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "..\Graphics\GraphicsPlatformInterface.h"
#include "D3D12Core.h"
#include "D3D12Content.h"
#include "D3D12Camera.h"

namespace nidhog::graphics::d3d12
{
	void get_platform_interface(platform_interface& pi)
	{
		pi.initialize = core::initialize;
		pi.shutdown = core::shutdown;

		//设置surface接口指针
		pi.surface.create = core::create_surface;
		pi.surface.remove = core::remove_surface;
		pi.surface.resize = core::resize_surface;
		pi.surface.width = core::surface_width;
		pi.surface.height = core::surface_height;
		pi.surface.render = core::render_surface;

		pi.camera.create = camera::create;
		pi.camera.remove = camera::remove;
		pi.camera.set_parameter = camera::set_parameter;
		pi.camera.get_parameter = camera::get_parameter;

		pi.resources.add_submesh = content::submesh::add;
		pi.resources.remove_submesh = content::submesh::remove;

		pi.platform = graphics_platform::direct3d12;
	}
}