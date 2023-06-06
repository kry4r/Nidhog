#ifndef EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif // !EDITOR_INTERFACE

#include "CommonHeaders.h"
#include "Id.h"
#include "..\Engine\Components\Entity.h"
#include "..\Engine\Components\Transform.h"

using namespace nidhog;

namespace {

    struct transform_component
    {
        f32 position[3];
        f32 rotation[3];
        f32 scale[3];

        transform::init_info to_init_info()
        {
            using namespace DirectX;
            transform::init_info info{};
            memcpy(&info.position[0], &position[0], sizeof(f32) * _countof(position));
            memcpy(&info.scale[0], &scale[0], sizeof(f32) * _countof(scale));
            //这里是欧拉角，我们要将其转化为四元数（详见原rotation定义）
            XMFLOAT3A rot{ &rotation[0] };
            XMVECTOR quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
            XMFLOAT4A rot_quat{};
            XMStoreFloat4A(&rot_quat, quat);
            memcpy(&info.rotation[0], &rot_quat.x, sizeof(f32) * _countof(info.rotation));
            return info;
        }
    };


    struct game_entity_descriptor
    {
        transform_component transform;
    };

    game_entity::entity entity_from_id(id::id_type id)
    {
        return game_entity::entity{ game_entity::entity_id{id} };
    }

} // 匿名namespace


//给editor的接口，使用对entity的描述来创建entity（这里暂时只有transform组件）

EDITOR_INTERFACE id::id_type
CreateGameEntity(game_entity_descriptor* e)
{
    assert(e);
    game_entity_descriptor& desc{ *e };
    transform::init_info transform_info{ desc.transform.to_init_info() };
    game_entity::entity_info entity_info
    {
        &transform_info,
    };
    return game_entity::create(entity_info).get_id();
}

EDITOR_INTERFACE void
RemoveGameEntity(id::id_type id)
{
    assert(id::is_valid(id));
    game_entity::remove(game_entity::entity_id{id});
}