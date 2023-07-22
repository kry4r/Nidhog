#pragma once

#include "CommonHeaders.h"

namespace nidhog::graphics {

    DEFINE_TYPED_ID(light_id);

    class light
    {
    public:
        //type of light
        enum type : u32
        {
            directional,
            point,
            spot,

            count
        };

        // Set light to set(collection) depend key (Haha that's a bit of a mouthful)
        constexpr explicit light(light_id id, u64 light_set_key) : _light_set_key{ light_set_key }, _id{ id } {}
        constexpr light() = default;
        constexpr light_id get_id() const { return _id; }
        constexpr u64 ligh_set_key() const { return _light_set_key; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

        void is_enabled(bool is_enabled) const;
        void intensity(f32 intensity) const;
        void color(math::v3 color) const;
        void attenuation(math::v3 attenuation) const;
        void range(f32 range) const;
        void cone_angles(f32 umbra, f32 penumbra) const;

        bool is_enabled() const;
        f32 intensity() const;
        math::v3 color() const;
        math::v3 attenuation() const;
        f32 range() const;
        f32 umbra() const;
        f32 penumbra() const;
        type light_type() const;
        id::id_type entity_id() const;

    private:
        u64         _light_set_key{ 0 };
        light_id    _id{ id::invalid_id };
    };
}