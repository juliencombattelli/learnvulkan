#/bin/bash

readonly TIME="/usr/bin/time"

echo "################################################################################"
echo "### Configuring project for gcc"
echo "################################################################################"
CC=gcc CXX=g++ $TIME cmake -S . -B build-gcc -G "Ninja Multi-Config"
echo "################################################################################"
echo ""
echo ""
echo "################################################################################"
echo "### Configuring project for clang"
echo "################################################################################"
CC=clang CXX=clang++ $TIME cmake -S . -B build-clang -G "Ninja Multi-Config"
echo "################################################################################"
echo ""
echo ""

for compiler in gcc clang; do
    for config in Debug Release RelWithDebInfo; do
        echo "################################################################################"
        echo "### Compiling with $compiler, config $config"
        echo "################################################################################"
        $TIME cmake --build "build-$compiler" --config "$config"
        echo "################################################################################"
        echo ""
        echo ""
    done
done
