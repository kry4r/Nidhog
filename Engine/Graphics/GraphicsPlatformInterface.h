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
	};
}