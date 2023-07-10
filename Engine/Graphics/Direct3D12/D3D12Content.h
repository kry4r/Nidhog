#pragma once

#include "D3D12CommonHeaders.h"

namespace nidhog::graphics::d3d12::content 
{
	namespace submesh 
	{

		id::id_type add(const u8*& data);
		void remove(id::id_type id);

	} // namespace submesh
}