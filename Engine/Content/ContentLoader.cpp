#include "ContentLoader.h"
#include "..\Components\Entity.h"
#include "..\Components\Transform.h"
#include "..\Components\Script.h"


#if !defined(SHIPPING)
#include <fstream>
#include <filesystem>
#include <Windows.h>
namespace nidhog::content 
{
	namespace 
	{
		enum component_type
		{
			transform,
			script,

			count
		};

		utl::vector<game_entity::entity> entities;
		transform::init_info transform_info{};
		script::init_info script_info{};

		bool read_transform(const u8*& data, game_entity::entity_info& info)
		{
			using namespace DirectX;
			//使用info指针时，需要先创建一个东西来储存，之后判断指针是否尚未设置
			f32 rotation[3];

			assert(!info.transform);
			memcpy(&transform_info.position[0], data, sizeof(transform_info.position)); data += sizeof(transform_info.position);
			memcpy(&rotation[0], data, sizeof(rotation)); data += sizeof(rotation);
			memcpy(&transform_info.scale[0], data, sizeof(transform_info.scale)); data += sizeof(transform_info.scale);

			XMFLOAT3A rot{ &rotation[0] };
			XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
			XMFLOAT4A rot_quat{};
			XMStoreFloat4A(&rot_quat, quat);
			memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));

			info.transform = &transform_info;

			return true;
		}

		bool read_script(const u8*& data, game_entity::entity_info& info)
		{
			assert(!info.script);
			const u32 name_length{ *data }; data += sizeof(u32);
			if (!name_length) return false;
			// 如果脚本名称超过255个字符
			// 那么可能是二进制编写器或游戏程序员出了问题。
			assert(name_length < 256);
			char script_name[256];
			memcpy(&script_name[0], data, name_length); data += name_length;
			// 使名称成为以0结尾的c-string.
			script_name[name_length] = 0;
			script_info.script_creator = script::detail::get_script_creator(script::detail::string_hash()(script_name));
			info.script = &script_info;
			return script_info.script_creator != nullptr;
		}

		using component_reader = bool(*)(const u8*&, game_entity::entity_info&);

		component_reader component_readers[]
		{
			read_transform,
			read_script,
		};
		static_assert(_countof(component_readers) == component_type::count);
	}
	bool load_game()
	{
		// 将工作目录设置为可执行文件路径
		wchar_t path[MAX_PATH];
		const u32 length{ GetModuleFileName(0, &path[0], MAX_PATH) };
		if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return false;
		std::filesystem::path p{ path };
		SetCurrentDirectory(p.parent_path().wstring().c_str());


		//读取game.bin文件并且创建entity
		std::ifstream game("game.bin", std::ios::in | std::ios::binary);
		//从buffer（缓冲区）中读取文件的一种比较慢的方法
		utl::vector<u8> buffer(std::istreambuf_iterator<char>(game), {});

		assert(buffer.size());
		//指向buffer的指针
		const u8* at{ buffer.data() };
		constexpr u32 su32{ sizeof(u32) };
		const u32 num_entities{ *at }; at += su32;

		if (!num_entities) return false;

		for (u32 entity_index{ 0 }; entity_index < num_entities; ++entity_index) 
		{
			game_entity::entity_info info{};
			const u32 entity_type{ *at }; at += su32;
			const u32 num_components{ *at }; at += su32;
			if (!num_components) return false;

			for (u32 component_index{ 0 }; component_index < num_components; ++component_index)
			{
				const u32 component_type{ *at }; at += su32;
				assert(component_type < component_type::count);
				//指针数组
				if (!component_readers[component_type](at, info)) return false;
			}
			//一切读取完成之后，创建entity
			assert(info.transform);
			game_entity::entity entity{ game_entity::create(info) };
			if (!entity.is_valid()) return false;
			entities.emplace_back(entity);
		}
		assert(at == buffer.data() + buffer.size());
		return true;
	}
	void unload_game() 
	{
		for (auto entity : entities)
		{
			game_entity::remove(entity.get_id());
		}
	}
}

#endif // !defined(SHIPPING)