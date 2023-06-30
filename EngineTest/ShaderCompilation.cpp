

#include "..\packages\DirectXShaderCompiler\inc\d3d12shader.h"
#include "..\packages\DirectXShaderCompiler\inc\dxcapi.h"


#include "Graphics\Direct3D12\D3D12Core.h"
#include "Graphics\Direct3D12\D3D12Shaders.h"

#include <fstream>
#include <filesystem>

// NOTE: we wouldn't need to do this if DXC had a NuGet package.
#pragma comment(lib, "../packages/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

using namespace nidhog;
using namespace Microsoft::WRL;
using namespace nidhog::graphics::d3d12::shaders;
namespace
{
	struct shader_file_info
	{
		const char* file;
		const char* function;
		engine_shader::id   id;
		shader_types::type   type;
	};

	constexpr shader_file_info shader_files[]
	{
		{"FullScreenTriangle.hlsl", "FullScreenTriangleVS", engine_shader::fullscreen_triangle_vs, shader_types::vertex},
		{"FillColor.hlsl", "FillColorPS", engine_shader::fill_color_ps, shader_types::pixel},
	};

	static_assert(_countof(shader_files) == engine_shader::count);

	constexpr const char* shaders_source_path{ "../../../Engine/Graphics/Direct3D12/Shaders/" };

	//字符串转换
	std::wstring to_wstring(const char* c)
	{
		std::string s{ c };
		return { s.begin(), s.end() };
	}

	class shader_compiler
	{
	public:
		shader_compiler()
		{
			HRESULT hr{ S_OK };
			DXCall(hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&_compiler)));
			if (FAILED(hr)) return;
			DXCall(hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&_utils)));
			if (FAILED(hr)) return;
			DXCall(hr = _utils->CreateDefaultIncludeHandler(&_include_handler));
			if (FAILED(hr)) return;
		}

		DISABLE_COPY_AND_MOVE(shader_compiler);

		IDxcBlob* compile(shader_file_info info, std::filesystem::path full_path)
		{
			assert(_compiler && _utils && _include_handler);
			HRESULT hr{ S_OK };

			// Load the source file using Utils interface.
			ComPtr<IDxcBlobEncoding> source_blob{ nullptr };
			DXCall(hr = _utils->LoadFile(full_path.c_str(), nullptr, &source_blob));
			if (FAILED(hr)) return nullptr;
			assert(source_blob && source_blob->GetBufferSize());

			std::wstring file{ to_wstring(info.file) };
			std::wstring func{ to_wstring(info.function) };
			std::wstring prof{ to_wstring(_profile_strings[(u32)info.type]) };

			//为编译器提供编译器选项
			LPCWSTR args[]
			{
				file.c_str(),                       // Optional shader source file name for error reporting
				L"-E", func.c_str(),                // Entry function
				L"-T", prof.c_str(),                // Target profile
				DXC_ARG_ALL_RESOURCES_BOUND,
	#if _DEBUG
				DXC_ARG_DEBUG,
				DXC_ARG_SKIP_OPTIMIZATIONS,
	#else
				DXC_ARG_OPTIMIZATION_LEVEL3,
	#endif
				DXC_ARG_WARNINGS_ARE_ERRORS,
				L"-Qstrip_reflect",                 // 将反射信息剥离到单独的 blob 中
				L"-Qstrip_debug",                   // 将调试信息剥离到单独的 blob 中
			};

			OutputDebugStringA("Compiling ");
			OutputDebugStringA(info.file);

			return compile(source_blob.Get(), args, _countof(args));
		}

		IDxcBlob* compile(IDxcBlobEncoding* source_blob, LPCWSTR* args, u32 num_args)
		{
			//填充DXCBuffer
			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP; // 自动检测文本格式
			buffer.Ptr = source_blob->GetBufferPointer();
			buffer.Size = source_blob->GetBufferSize();

			HRESULT hr{ S_OK };
			ComPtr<IDxcResult> results{ nullptr };
			DXCall(hr = _compiler->Compile(&buffer, args, num_args, _include_handler.Get(), IID_PPV_ARGS(&results)));
			if (FAILED(hr)) return nullptr;

			ComPtr<IDxcBlobUtf8> errors{ nullptr };
			DXCall(hr = results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
			if (FAILED(hr)) return nullptr;

			if (errors && errors->GetStringLength())
			{
				OutputDebugStringA("\nShader compilation error: \n");
				OutputDebugStringA(errors->GetStringPointer());
			}
			else
			{
				OutputDebugStringA(" [ Succeeded ]");
			}
			OutputDebugStringA("\n");

			HRESULT status{ S_OK };
			DXCall(hr = results->GetStatus(&status));
			if (FAILED(hr) || FAILED(status)) return nullptr;

			ComPtr<IDxcBlob> shader{ nullptr };
			DXCall(hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));
			if (FAILED(hr)) return nullptr;

			return shader.Detach();
		} 

	private:
		const char* _profile_strings[shader_types::count]{ "vs_6_5", "hs_6_5", "ds_6_5", "gs_6_5", "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5" };
		static_assert(_countof(_profile_strings) == shader_types::count);

		//对一些接口进行实例化
		ComPtr<IDxcCompiler3>       _compiler{ nullptr };
		ComPtr<IDxcUtils>           _utils{ nullptr };
		ComPtr<IDxcIncludeHandler>  _include_handler{ nullptr };
	};

	//获取需要编译的shader位置
	decltype(auto) get_engine_shaders_path()
	{
		return std::filesystem::absolute(graphics::get_engine_shaders_path(graphics::graphics_platform::direct3d12));
	}

	bool compile_shaders_are_up_to_date()
	{
		auto engine_shaders_path = get_engine_shaders_path();
		if (!std::filesystem::exists(engine_shaders_path)) return false;
		auto shaders_compilation_time = std::filesystem::last_write_time(engine_shaders_path);

		std::filesystem::path path{};
		std::filesystem::path full_path{};

		// 检查是否是最新编译的
		// In that case, we need to recompile.
		for (u32 i{ 0 }; i < engine_shader::count; ++i)
		{
			auto& info = shader_files[i];

			path = shaders_source_path;
			path += info.file;
			full_path = std::filesystem::absolute(path);
			if (!std::filesystem::exists(full_path)) return false;

			auto shader_file_time = std::filesystem::last_write_time(full_path);
			if (shader_file_time > shaders_compilation_time)
			{
				return false;
			}
		}

		return true;
	}

	bool save_compile_shaders(utl::vector<ComPtr<IDxcBlob>>& shaders)
	{
		auto engine_shaders_path = get_engine_shaders_path();
		std::filesystem::create_directories(engine_shaders_path.parent_path());
		//创建二进制文件并写入shader大小及其字节码
		std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
		if (!file || !std::filesystem::exists(engine_shaders_path))
		{
			file.close();
			return false;
		}

		for (auto& shader : shaders)
		{
			const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };
			file.write((char*)&byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
			file.write((char*)byte_code.pShaderBytecode, byte_code.BytecodeLength);
		}

		file.close();
		return true;
	}
} //匿名namesapce


bool compile_shaders()
{
	if (compile_shaders_are_up_to_date()) return true;
	utl::vector<ComPtr<IDxcBlob>> shaders;
	std::filesystem::path path{};
	std::filesystem::path full_path{};

	shader_compiler compiler{};




	/// compile shaders and them together in a buffer in the same order of compilation.
	for (u32 i{ 0 }; i < engine_shader::count; ++i)
	{
		auto& info = shader_files[i];

		path = shaders_source_path;
		path += info.file;
		full_path = std::filesystem::absolute(path);
		if (!std::filesystem::exists(full_path)) return false;
		ComPtr<IDxcBlob> compiled_shader{ compiler.compile(info, full_path) };
		if (compiled_shader->GetBufferPointer() && compiled_shader->GetBufferSize())
		{
			shaders.emplace_back(std::move(compiled_shader));
		}
		else
		{
			return false;
		}
	}

	return save_compile_shaders(shaders);
}