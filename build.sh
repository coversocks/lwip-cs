cmake                                                           \
    -DCMAKE_TOOLCHAIN_FILE=${NDK_ROOT}/build/cmake/android.toolchain.cmake \
    -DANDROID_NDK=${NDK_ROOT}                               \
    -DANDROID_ABI=armeabi-v7a                               \
    -DANDROID_PLATFORM=android-19                           \
    -DANDROID_STL=c++_shared                                \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=~/deps/android/armv7/lib/       \
