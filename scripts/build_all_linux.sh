#!/bin/bash

readonly TIME="/usr/bin/time"

# Vulkan SDKs are assumed to be installed next to the project
readonly VULKAN_SDK_DIR=../VulkanSDK
readonly VULKAN_PREFERRED_VERSION="1.3.268.0"

echo "################################################################################"
echo "### Vulkan SDK setup"
echo "################################################################################"
echo "Searching for Vulkan SDK in $VULKAN_SDK_DIR"
if [ -f $VULKAN_SDK_DIR/$VULKAN_PREFERRED_VERSION/setup-env.sh ]; then
    readonly VULKAN_VERSION=$VULKAN_PREFERRED_VERSION
else
    SDK_VERSIONS=$(ls -1 $VULKAN_SDK_DIR | grep -E "[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+" | sort -rV)
    for SDK_VERSION in $SDK_VERSIONS; do
        if [ -f $VULKAN_SDK_DIR/$SDK_VERSION/setup-env.sh ]; then
            readonly VULKAN_VERSION=$SDK_VERSION
            break
        else
            echo "Skipping unusable SDK version $SDK_VERSION (missing: setup-env.sh)"
        fi
    done
fi

if [ -z $VULKAN_VERSION ]; then
    echo "No suitable Vulkan SDK found"
    exit 1
fi

echo "Using Vulkan SDK $VULKAN_VERSION"
source $VULKAN_SDK_DIR/$VULKAN_VERSION/setup-env.sh || exit 1
echo ""

# Compiler object interface
function compiler_id {
    local -r self=$1
    local -r self_id="$self[id]"
    echo "${!self_id}"
}
function compiler_name {
    local -r self=$1
    local -r self_name="$self[name]"
    echo "${!self_name}"
}
function compiler_c {
    local -r self=$1
    local -r self_c_compiler="$self[c_compiler]"
    echo "${!self_c_compiler}"
}
function compiler_cxx {
    local -r self=$1
    local -r self_cxx_compiler="$self[cxx_compiler]"
    echo "${!self_cxx_compiler}"
}
function compiler_build_configs {
    local -r self=$1
    local -r self_build_configs="$self[build_configs]"
    echo "${!self_build_configs}"
}
function compiler_config_status {
    local -r self=$1
    local -r self_config_status="$self[config_status]"
    echo "${!self_config_status}"
}
function compiler_set_config_status {
    local -r self=$1
    local -r config_status=$2
    local -r self_config_status="$self[config_status]"
    eval "$self[config_status]=$config_status"
}
function compiler_build_status {
    local -r self=$1
    local -r build_config=$2
    local -r self_build_status="$self[build_status_$build_config]"
    echo "${!self_build_status}"
}
function compiler_set_build_status {
    local -r self=$1
    local -r build_config=$2
    local -r build_status=$3
    eval "$self[build_status_$build_config]=$build_status"
}

# Declare each compiler
declare -A GCC_COMPILER=(
    [id]="gcc"
    [name]="GCC"
    [c_compiler]="gcc"
    [cxx_compiler]="g++"
    [build_configs]="Debug Release RelWithDebInfo"
)
declare -A CLANG_COMPILER=(
    [id]="clang"
    [name]="Clang"
    [c_compiler]="clang"
    [cxx_compiler]="clang++"
    [build_configs]="Debug Release RelWithDebInfo"
)

# Declare the list of compilers
declare -ra COMPILERS=(
    GCC_COMPILER
    CLANG_COMPILER
)

function print_configuration_header {
    local -r compiler=$1
    echo "################################################################################"
    echo "### Configuring project for $(compiler_name $compiler)"
    echo "################################################################################"
}
function print_configuration_footer {
    local -r compiler=$1
    echo "################################################################################"
    echo ""
    echo ""
}
function print_build_header {
    local -r compiler=$1
    local -r build_config=$2
    echo "################################################################################"
    echo "### Compiling with $(compiler_name $compiler), configuration $build_config"
    echo "################################################################################"
}
function print_build_footer {
    local -r compiler=$1
    local -r build_config=$2
    echo "################################################################################"
    echo ""
    echo ""
}

for compiler in "${COMPILERS[@]}"; do
    print_configuration_header $compiler
    if ! CC=$(compiler_c $compiler) CXX=$(compiler_cxx $compiler) $TIME cmake -S . -B build-$(compiler_id $compiler) -G "Ninja Multi-Config"; then
        compiler_set_config_status $compiler failed
        SOME_CONFIG_ERROR=1
    else
        compiler_set_config_status $compiler ok
    fi
    print_configuration_footer $compiler
done

for compiler in "${COMPILERS[@]}"; do
    if [ "$(compiler_config_status $compiler)" != "ok" ]; then
        for build_config in $(compiler_build_configs $compiler); do
            compiler_set_build_status $compiler $build_config skipped
            SOME_BUILD_ERROR=1
        done
        continue
    fi
    for build_config in $(compiler_build_configs $compiler); do
        print_build_header $compiler $build_config
        if ! $TIME cmake --build "build-$(compiler_id $compiler)" --config "$build_config"; then
            compiler_set_build_status $compiler $build_config failed
            SOME_BUILD_ERROR=1
        else
            compiler_set_build_status $compiler $build_config ok
        fi
        print_build_footer $compiler $build_config
    done
done

if [ -n "${SOME_CONFIG_ERROR}" ]; then
    echo "At least one of the configuration failed:"
    for compiler in "${COMPILERS[@]}"; do
        echo "- $(compiler_name $compiler) ($(compiler_config_status $compiler))"
    done
    echo ""
fi

if [ -n "${SOME_BUILD_ERROR}" ]; then
    echo "At least one of the build failed:"
    for compiler in "${COMPILERS[@]}"; do
        for build_config in $(compiler_build_configs $compiler); do
            build_result="$(compiler_build_status $compiler $build_config)"
            if [ "$build_result" != "ok" ]; then
                echo "- $(compiler_name $compiler) $build_config ($build_result)"
            fi
        done
    done
    exit 1
else
    echo "All builds succeeded!"
fi
