#pragma once

//ʵ��API�ӿ�

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