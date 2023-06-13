#pragma once
#pragma warning(disable: 4530) //disable exception warning
//C/C++ header(C/C++Í·ÎÄ¼þ£©

#include<stdint.h>
#include<assert.h>
#include<typeinfo>
#include <memory>
#include <unordered_map>

#if defined(_WIN64)
#include<DirectXMath.h>
#endif
//Common header
#include"PrimitiveTypes.h"
#include "..\Utilities\Math.h"
#include"..\Utilities\Utilities.h"
#include"..\Utilities\MathType.h"
#include "id.h"


#ifdef _DEBUG
#define DEBUG_OP(x) x
#else
#define DEBUG_OP(x) (void(0))
#endif