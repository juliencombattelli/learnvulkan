#pragma once

// C++20 designated initializers require to not have constructors.
#define VULKAN_HPP_NO_CONSTRUCTORS

// Do not use C++20 spaceship <=> operator. Refer to
// https://github.com/KhronosGroup/Vulkan-Hpp/tree/v1.3.268?tab=readme-ov-file#vulkan_hpp_no_spaceship_operator
// for explanations as of Vulkan 1.3.268.
#define VULKAN_HPP_NO_SPACESHIP_OPERATOR

// Use vk::DispatchLoaderDynamic as the default dispatcher to avoid passing it
// to every Vulkan function call. Refer to
// https://github.com/KhronosGroup/Vulkan-Hpp/tree/v1.3.268?tab=readme-ov-file#vulkan_hpp_default_dispatcher
// for more details as of Vulkan 1.3.268.
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp> // IWYU pragma: export

// Include all others Vulkan headers that might be used transitivelly by
// vulkan/vulkan.hpp
// This avoids issues with IWYU
#include <vulkan/vulkan_enums.hpp> // IWYU pragma: export
#include <vulkan/vulkan_funcs.hpp> // IWYU pragma: export
#include <vulkan/vulkan_structs.hpp> // IWYU pragma: export
