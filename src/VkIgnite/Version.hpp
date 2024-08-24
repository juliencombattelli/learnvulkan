#pragma once

#include "MinVkVersion.hpp"

#include "Pch/Vulkan.hpp"

#include <strong_type/strong_type.hpp>

namespace vki {

using Version = strong::type<uint32_t, struct VersionTag, strong::default_constructible>;

// TODO move to defaults
[[nodiscard]] static inline constexpr Version makeVersion(
    uint32_t major,
    uint32_t minor,
    uint32_t patch) noexcept
{
    // Reuse the Vulkan version encoding for now, with a variant set to 0 to be
    // conform with semantic versionning.
    // The user can still use whatever encoding he wants for application and
    // engine versions.
    return Version { vk::makeApiVersion(0u, major, minor, patch) };
}

class ApiVersion {
public:
    [[nodiscard]] constexpr ApiVersion(
        uint32_t variant,
        uint32_t major,
        uint32_t minor,
        uint32_t patch)
        : value_ { vk::makeApiVersion(variant, major, minor, patch) } {};

    [[nodiscard]] constexpr uint32_t variant() const
    {
        return vk::apiVersionVariant(value_);
    }

    [[nodiscard]] constexpr uint32_t major() const
    {
        return vk::apiVersionMajor(value_);
    }

    [[nodiscard]] constexpr uint32_t minor() const
    {
        return vk::apiVersionMinor(value_);
    }

    [[nodiscard]] constexpr uint32_t patch() const
    {
        return vk::apiVersionPatch(value_);
    }

    [[nodiscard]] constexpr uint32_t value() const
    {
        return value_;
    }

    [[nodiscard]] static constexpr ApiVersion minimumRequired()
    {
        return {
            0,
            VULKAN_MIN_VERSION_MAJOR,
            VULKAN_MIN_VERSION_MINOR,
            VULKAN_MIN_VERSION_PATCH,
        };
    }

private:
    uint32_t value_;
};

} // namespace vki
