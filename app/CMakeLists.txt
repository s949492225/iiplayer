cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

include_directories(src/main/cpp/include/ffmpeg)

add_library( # Sets the name of the library.
        player
        SHARED
        src/main/cpp/android/iiplayer_jni.cpp
        src/main/cpp/android/CallJava.cpp
        src/main/cpp/media/MediaPlayer.cpp
        src/main/cpp/media/queue/PacketQueue.cpp
        src/main/cpp/media/global/LifeSequenceHolder.cpp
        src/main/cpp/media/other/util.cpp
        src/main/cpp/media/queue/FrameQueue.cpp
        src/main/cpp/media/reader/PacketReader.cpp
        src/main/cpp/media/decode/AudioDecoder.cpp
        src/main/cpp/media/decode/VideoDecoder.cpp
        src/main/cpp/media/decode/BaseDecoder.cpp
        src/main/cpp/media/decode/HardVideoDecoder.cpp
        src/main/cpp/media/render/AudioRender.cpp
        src/main/cpp/media/render/VideoRender.cpp
        src/main/cpp/media/sdl/SDLVideo.cpp
        src/main/cpp/media/sdl/shaderUtils.cpp
        src/main/cpp/media/sdl/sdl_jni.cpp
        src/main/cpp/android/util.cpp
        )

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavcodec.so)


add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavdevice.so)


add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavfilter.so)


add_library(avformat SHARED IMPORTED)
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavformat.so)


add_library(avutil SHARED IMPORTED)
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavutil.so)

add_library(postproc SHARED IMPORTED)
set_target_properties(postproc
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libpostproc.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libswresample.so)


add_library(swscale SHARED IMPORTED)
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libswscale.so)


target_link_libraries( # Specifies the target library.
        player
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        postproc
        swresample
        swscale
        android
        -landroid
        OpenSLES
        EGL
        GLESv2
        log)

# Links the target library to the log library
# included in the NDK.