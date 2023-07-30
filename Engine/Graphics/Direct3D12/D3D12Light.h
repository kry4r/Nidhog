#pragma once
#include "D3D12CommonHeaders.h"

namespace nidhog::graphics::d3d12 
{
	struct d3d12_frame_info;
}
namespace nidhog::graphics::d3d12::light {

	bool initialize();
	void shutdown();

	graphics::light create(light_init_info info);
	void remove(light_id id, u64 light_set_key);
	void set_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, const void* const data, u32 data_size);
	void get_parameter(light_id id, u64 light_set_key, light_parameter::parameter parameter, void* const data, u32 data_size);

	void update_light_buffers(const d3d12_frame_info& d3d12_info);
	// get light buffer gpu virtual address
	D3D12_GPU_VIRTUAL_ADDRESS non_cullable_light_buffer(u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS cullable_light_buffer(u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS culling_info_buffer(u32 frame_index);
	D3D12_GPU_VIRTUAL_ADDRESS bounding_spheres_buffer(u32 frame_index);

	u32 non_cullable_light_count(u64 light_set_key);
	u32 cullable_light_count(u64 light_set_key);
}