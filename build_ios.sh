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
WORKING_DIR="$(cd $(dirname $0))";
cd "$WORKING_DIR";

ARCHS="i386 x86_64 armv7 armv7s arm64";
DEVELOPER_ROOT=$(xcode-select -print-path);

# ======================= options ======================= 
while getopts "a:d:hs:-" OPTION; do
    case $OPTION in
        a)
            ARCHS="$OPTARG";
        ;;
        d)
            DEVELOPER_ROOT="$OPTARG";
        ;;
        h)
            echo "usage: $0 [options] [-- [make options]]";
            echo "options:";
            echo "-a [archs]                    which arch need to built, multiple values must be split by space(default: $ARCHS)";
            echo "-d [developer root directory] developer root directory, we use xcode-select -print-path to find default value.(default: $DEVELOPER_ROOT)";
            echo "-s [sdk version]              sdk version, we use xcrun -sdk iphoneos --show-sdk-version to find default value.(default: $SDKVERSION)";
            echo "-h                            help message.";
            exit 0;
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
    #export CC="${BUILD_TOOLS}/usr/bin/gcc -arch ${ARCH}";
    export CC=${BUILD_TOOLS}/usr/bin/gcc;
    #export LD=${BUILD_TOOLS}/usr/bin/ld;
    #export CPP=${BUILD_TOOLS}/usr/bin/cpp;
    #export CXX=${BUILD_TOOLS}/usr/bin/g++;
    export AR=${DEVELOPER_ROOT}/Toolchains/XcodeDefault.xctoolchain/usr/bin/ar;
    #export AS=${DEVROOT}/usr/bin/as;
    #export NM=${DEVROOT}/usr/bin/nm;
    #export CXXCPP=${BUILD_TOOLS}/usr/bin/cpp;
    #export RANLIB=${BUILD_TOOLS}/usr/bin/ranlib;
    #export LDFLAGS="-arch ${ARCH} -isysroot ${SDKROOT} ";
    export CFLAGS="-arch ${ARCH} -isysroot ${SDKROOT} -O2 -fPIC -Wall";
   
    make libpbc.a CC="$CC" AR="$AR rc" CFLAGS="$CFLAGS" $@;
    mv -f build/libpbc.a "build/libpbc-$ARCH.a";
done

cd "$WORKING_DIR";
echo "Linking and packaging library...";

for LIB_NAME in "libpbc"; do
    lipo -create build/$LIB_NAME-*.a -output "build/$LIB_NAME.a";
    echo "build/$LIB_NAME.a built.";
done
