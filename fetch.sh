#!/bin/bash

set -e

mkdir -p ./vendor

if [ ! -d "./vendor/sdl" ]; then
    git clone --depth 1 --branch release-3.4.2 https://github.com/libsdl-org/SDL.git ./vendor/sdl
fi

if [ ! -d "./vendor/sdl_image" ]; then
    git clone --depth 1 --branch release-3.4.0 https://github.com/libsdl-org/SDL_image.git ./vendor/sdl_image
fi

if [ ! -d "./vendor/sdl_shadercross" ]; then
    git clone --depth 1 https://github.com/libsdl-org/SDL_shadercross.git ./vendor/sdl_shadercross
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
    git submodule update --init libs/detail
    git submodule update --init libs/io
    git submodule update --init libs/utility
    git submodule update --init libs/preprocessor
    git submodule update --init libs/predef
    git submodule update --init libs/typeof
    git submodule update --init libs/describe
    git submodule update --init libs/assert
    git submodule update --init libs/static_assert
    git submodule update --init libs/type_traits
    git submodule update --init libs/throw_exception
    git submodule update --init libs/move
    git submodule update --init libs/intrusive
    git submodule update --init libs/mpl
    git submodule update --init libs/mp11
    git submodule update --init libs/bind
    git submodule update --init libs/function_types
    git submodule update --init libs/function
    git submodule update --init libs/container_hash
    git submodule update --init libs/container
    git submodule update --init libs/functional
    git submodule update --init libs/unordered
    cd ../..
fi

if [ ! -d "./vendor/cpptoml" ]; then
    git clone --depth 1 --branch v0.1.1 https://github.com/skystrife/cpptoml.git ./vendor/cpptoml
fi

if [ ! -d "./vendor/spdlog" ]; then
    git clone --depth 1 --branch v1.17.0 https://github.com/gabime/spdlog.git ./vendor/spdlog
fi

if [ ! -d "./vendor/tuplet" ]; then
    git clone --depth 1 https://github.com/codeinred/tuplet.git ./vendor/tuplet
fi

if [ ! -d "./vendor/gtest" ]; then
    git clone --depth 1 --branch v1.17.0 https://github.com/google/googletest.git ./vendor/gtest
fi

