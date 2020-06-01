#!/bin/bash
# gomobile bind -o csocks.a -target=android -buildmode=c-archive ../
# gomobile bind -target=ios/arm64 -o testlwipcs/csocks.framework ../


# set NDK_ROOT=/Users/vty/Library/Android/sdk/ndk-bundle/
# set GOARCH=armeabi-v7a
# set GOOS=android
# set CC=arm-linux-androideabi-gcc
# set CGO_ENABLED=1
# set CGO_LDFLAGS=--sysroot=%NDK_ROOT%/platforms/android-21/arch-arm
# set CGO_CFLAGS=--sysroot=%NDK_ROOT%/platforms/android-21/arch-arm
# set CC=arm-linux-androideabi-gcc
# export GOARM=7
# export GOPATH=/Users/vty/vgo
# export CC=/Users/vty/Library/Android/sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/darwin-x86_64/bin/clang
# export ANDROID_SDK=/Users/vty/Library/Android/sdk
# export GOARCH=arm
# export ANDROID_NDK=/Users/vty/Library/Android/sdk/ndk/21.1.6352462/
# export CXX=/Users/vty/Library/Android/sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/darwin-x86_64/bin/clang++
# export ANDROID_HOME=/Users/vty/Library/Android/sdk
# export GOOS=android
# export CGO_ENABLED=1

export ANDROID_HOME=/Users/vty/Library/Android/sdk
export ANDROID_NDK=/Users/vty/Library/Android/sdk/ndk/21.1.6352462/
export GOOS=android
export CC=/Users/vty/Library/Android/sdk/ndk-bundle/toolchains/llvm/prebuilt/darwin-x86_64/bin/armv7a-linux-androideabi16-clang
export CXX=/Users/vty/Library/Android/sdk/ndk-bundle/toolchains/llvm/prebuilt/darwin-x86_64/bin/armv7a-linux-androideabi16-clang++
export CGO_ENABLED=1
export ANDROID_SDK=/Users/vty/Library/Android/sdk
export GOPATH=/Users/vty/go
export GOARM=7
export GOARCH=arm
# go build -buildmode=c-archive -o libmobile.a .
go build -buildmode=c-shared -o libmobile.so ../mobile
