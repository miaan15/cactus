#!/bin/bash

set -e

mkdir -p ./vendor

if [ ! -d "./vendor/raylib" ]; then
    git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git ./vendor/raylib
fi

if [ ! -d "./vendor/glm" ]; then
    git clone --depth 1 --branch 1.0.3 https://github.com/g-truc/glm.git ./vendor/glm
fi

if [ ! -d "./vendor/boost" ]; then
    git clone --depth 1 --branch boost-1.90.0 https://github.com/boostorg/boost.git ./vendor/boost
    cd ./vendor/boost
    git submodule update --init tools/cmake
    git submodule update --init libs/core
    git submodule update --init libs/config
    git submodule update --init libs/assert
    git submodule update --init libs/static_assert
    git submodule update --init libs/type_traits
    git submodule update --init libs/throw_exception
    git submodule update --init libs/move
    git submodule update --init libs/intrusive
    git submodule update --init libs/container
    cd ../..
fi

if [ ! -d "./vendor/gtest" ]; then
    git clone --depth 1 --branch v1.17.0 https://github.com/google/googletest.git ./vendor/gtest
fi

