#!/usr/bin/env bash

if [ ! -d "jni/SDL2" ]; then
    hg clone http://hg.libsdl.org/SDL jni/SDL2
    hg clone http://hg.libsdl.org/SDL_image jni/SDL2_image
    hg clone http://hg.libsdl.org/SDL_mixer jni/SDL2_mixer
fi

ndk-build -j$(nproc) && ant clean debug
