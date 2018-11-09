#!/bin/bash

X264_VERSION=""
SOURCE="x264"
SHELL_PATH=`pwd`
X264_PATH=$SHELL_PATH/$SOURCE
#输出路径
PREFIX=$SHELL_PATH/x264_android
LAST_VERSION=$1
ANDROID_API=$2
NDK=$3

#需要编译的Android API版本
if [ ! "$ANDROID_API" ]
then
ANDROID_API=28
fi
#需要编译的NDK路径，NDK版本需大等于r15c
if [ ! "$NDK" ]
then
NDK=/Users/lzj/Library/Android/sdk/ndk-bundle
fi
echo ANDROID_API=$ANDROID_API
echo NDK=$NDK

#需要编译的平台:arm arm64 x86 x86_64，可传入平台单独编译对应的库
ARCHS=(arm arm64 x86 x86_64)
TRIPLES=(arm-linux-androideabi aarch64-linux-android i686-linux-android x86_64-linux-android)
TRIPLES_PATH=(arm-linux-androideabi-4.9 aarch64-linux-android-4.9 x86-4.9 x86_64-4.9)

FF_CONFIGURE_FLAGS="--enable-static --enable-pic --disable-cli"

rm -rf "$SOURCE"
if [ ! -r $SOURCE ]
then
    echo "$SOURCE source not found, Trying to download..."
    if [ "$LAST_VERSION" ]
    then
        X264_TAR_NAME="last_x264.tar.bz2"
    else
        X264_TAR_NAME="x264-snapshot-20160114-2245.tar.bz2"
    fi
    curl -O http://download.videolan.org/pub/videolan/x264/snapshots/$X264_TAR_NAME
    mkdir $X264_PATH
    tar zxvf $SHELL_PATH/$X264_TAR_NAME --strip-components 1 -C $X264_PATH || exit 1
fi

cd $X264_PATH
for i in "${!ARCHS[@]}";
do
    ARCH=${ARCHS[$i]}
    TOOLCHAIN=$NDK/toolchains/${TRIPLES_PATH[$i]}/prebuilt/darwin-x86_64
    SYSROOT=$NDK/platforms/android-$ANDROID_API/arch-$ARCH/
    ISYSROOT=$NDK/sysroot
    ASM=$ISYSROOT/usr/include/${TRIPLES[$i]}
    CROSS_PREFIX=$TOOLCHAIN/bin/${TRIPLES[$i]}-
    PREFIX_ARCH=$PREFIX/$ARCH

    FF_CFLAGS="-I$ASM -isysroot $ISYSROOT -D__ANDROID_API__=$ANDROID_API -U_FILE_OFFSET_BITS -DANDROID"

    ./configure \
    --prefix=$PREFIX_ARCH \
    --sysroot=$SYSROOT \
    --host=${TRIPLES[$i]} \
    --cross-prefix=$CROSS_PREFIX \
    $FF_CONFIGURE_FLAGS \
    --extra-cflags="$FF_CFLAGS" \
    --extra-ldflags="" \
    $ADDITIONAL_CONFIGURE_FLAG || exit 1
    make -j3 install || exit 1
    make distclean
    rm -rf "$PREFIX_ARCH/lib/pkgconfig"
    if [[ $FF_CONFIGURE_FLAGS == *--enable-shared* ]]
    then
        mv $PREFIX_ARCH/lib/libx264.so.* $PREFIX_ARCH/lib/libx264.so
    fi
done

echo "Android x264 bulid success!"


