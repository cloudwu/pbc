#!/bin/bash

#  Automatic build script for pbc
###########################################################################
#  Change values here
#
SDKVERSION=$(xcrun -sdk iphoneos --show-sdk-version);
#
###########################################################################
#
# Don't change anything here
WORKING_DIR="$PWD";
ARCHS="i386 x86_64 armv7 armv7s arm64";
DEVELOPER_ROOT=$(xcode-select -print-path);
SOURCE_DIR="$PWD";
BUILD_TYPE="RelWithDebInfo" ;
OTHER_CFLAGS="-fPIC" ;

# ======================= options ======================= 
while getopts "a:b:d:hi:r:s:-" OPTION; do
    case $OPTION in
        a)
            ARCHS="$OPTARG";
        ;;
        b)
            BUILD_TYPE="$OPTARG";
        ;;
        d)
            DEVELOPER_ROOT="$OPTARG";
        ;;
        h)
            echo "usage: $0 [options] [-- [make options]]";
            echo "options:";
            echo "-a [archs]                    which arch need to built, multiple values must be split by space(default: $ARCHS)";
            echo "-b [build type]               build type(default: $BUILD_TYPE, available: Debug, Release, RelWithDebInfo, MinSizeRel)";
            echo "-d [developer root directory] developer root directory, we use xcode-select -print-path to find default value.(default: $DEVELOPER_ROOT)";
            echo "-h                            help message.";
            echo "-i [option]                   enable bitcode support(available: off, all, bitcode, marker)";
            echo "-s [sdk version]              sdk version, we use xcrun -sdk iphoneos --show-sdk-version to find default value.(default: $SDKVERSION)";
            echo "-r [source dir]               root directory of this library";
            exit 0;
        ;;
        i)
            if [ ! -z "$OPTARG" ]; then
                OTHER_CFLAGS="$OTHER_CFLAGS -fembed-bitcode=$OPTARG";
            else
                OTHER_CFLAGS="$OTHER_CFLAGS -fembed-bitcode";
            fi
        ;;
        r)
            SOURCE_DIR="$OPTARG";
        ;;
        s)
            SDKVERSION="$SDKVERSION";
        ;;
        -) 
            break;
            break;
        ;;
        ?)  #当有不认识的选项的时候arg为?
            echo "unkonw argument detected";
            exit 1;
        ;;
    esac
done

shift $(($OPTIND-1));

if [ ! -e "$SOURCE_DIR/CMakeLists.txt" ]; then
    echo "$SOURCE_DIR/CMakeLists.txt not found";
    exit -2;
fi
SOURCE_DIR="$(cd "$SOURCE_DIR" && pwd)";

echo "Ready to build for ios";
echo "WORKING_DIR=${WORKING_DIR}";
echo "ARCHS=${ARCHS}";
echo "DEVELOPER_ROOT=${DEVELOPER_ROOT}";
echo "SDKVERSION=${SDKVERSION}";
echo "make options=$@";

##########
for ARCH in ${ARCHS}; do
    echo "================== Compling $ARCH ==================";
    if [[ "${ARCH}" == "i386" || "${ARCH}" == "x86_64" ]]; then
        PLATFORM="iPhoneSimulator";
    else
        PLATFORM="iPhoneOS";
    fi

    if [ -e build/o ]; then
        rm -rf build/o;
    fi
    echo "Building pbc for ${PLATFORM} ${SDKVERSION} ${ARCH}";
    
    echo "Please stand by...";
    
    export DEVROOT="${DEVELOPER_ROOT}/Platforms/${PLATFORM}.platform/Developer";
    export SDKROOT="${DEVROOT}/SDKs/${PLATFORM}${SDKVERSION}.sdk";
    export BUILD_TOOLS="${DEVELOPER_ROOT}";
    export CFLAGS="-arch ${ARCH} -isysroot ${SDKROOT} -O2 -fPIC -Wall";
   
    if [ -e "$WORKING_DIR/build-$ARCH" ]; then
        rm -rf "$WORKING_DIR/build-$ARCH";
    fi
    mkdir -p "$WORKING_DIR/build-$ARCH";
    cd "$WORKING_DIR/build-$ARCH";

    cmake "$SOURCE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX="$WORKING_DIR/build-$ARCH" -DCMAKE_OSX_SYSROOT=$SDKROOT -DCMAKE_SYSROOT=$SDKROOT -DCMAKE_OSX_ARCHITECTURES=$ARCH -DCMAKE_C_FLAGS="$OTHER_CFLAGS" "$@";
    cmake --build . --target lib
    cmake --build . --target install
done

cd "$WORKING_DIR";
echo "Linking and packaging library...";

mkdir -p "prebuilt/lib";
if [ -e "prebuilt/include" ]; then
    rm -rf "prebuilt/include";
fi

if [ -e "prebuilt/lib/lua" ]; then
    rm -rf "prebuilt/lib/lua";
fi

for LIB_NAME in "libpbc.a" "libprotobuf.a"; do
    LIB_FOUND=($(find build-* -name $LIB_NAME));
    if [ ${#LIB_FOUND} -gt 0 ]; then
        if [ -e "prebuilt/lib/$LIB_NAME" ]; then
            rm -rf "prebuilt/lib/$LIB_NAME";
        fi
        echo "Run: lipo -create ${LIB_FOUND[@]} -output \"prebuilt/lib/$LIB_NAME\"";
        lipo -create ${LIB_FOUND[@]} -output "prebuilt/lib/$LIB_NAME";
        echo "lib/$LIB_NAME built.";
    fi
done

for ARCH_DIR in build-*; do
    if [ ! -e "prebuilt/include" ] && [ -e "$ARCH_DIR/include" ]; then
        echo "Copy include files from $ARCH_DIR/include";
        cp -rf "$ARCH_DIR/include" "prebuilt/include";
    fi

    if [ ! -e "prebuilt/lib/lua" ] && [ -e "$ARCH_DIR/lib/lua" ]; then
        echo "Copy lua files from $ARCH_DIR/lib/lua";
        cp -rf "$ARCH_DIR/lib/lua" "prebuilt/lib/lua";
    fi
done