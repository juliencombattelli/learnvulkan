#pragma once

#include <strong_type/strong_type.hpp>

namespace vki {

// Cannot use strong::type for those as they are given to Vulkan through arrays
using ExtensionName = const char*;
using LayerName = const char*;
using QueueFamilyIndex = uint32_t;

// Vulkan uses uint32_t to define version numbers
using VersionValueType = uint32_t;

} // namespace vki
