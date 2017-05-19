#!/bin/bash

#
###########################################################################
#
# Don't change anything here
WORKING_DIR="$PWD";

ARCHS="x86 x86_64 armeabi armeabi-v7a arm64-v8a";
NDK_ROOT=$NDK_ROOT;
SOURCE_DIR="$PWD";
ANDROID_NATIVE_API_LEVEL=16 ;
ANDROID_TOOLCHAIN=clang ;
ANDROID_STL= ; #
LUAINCDIR= ;
LUALIBDIR= ;
LUALIBNAME=("libluajit-*.so" "liblua.a" "liblua.so" "liblua.so.*" "libtolua.so" "libtolua.so.*");
BUILD_TYPE="RelWithDebInfo" ;

# ======================= options ======================= 
while getopts "a:b:c:f:hi:l:n:r:t:u:-" OPTION; do
    case $OPTION in
        a)
            ARCHS="$OPTARG";
        ;;
        b)
            BUILD_TYPE="$OPTARG";
        ;;
        c)
            ANDROID_STL="$OPTARG";
        ;;
        n)
            NDK_ROOT="$OPTARG";
        ;;
        h)
            echo "usage: $0 [options] -n NDK_ROOT -r SOURCE_DIR [-- [cmake options]]";
            echo "options:";
            echo "-a [archs]                    which arch need to built, multiple values must be split by space(default: $ARCHS)";
            echo "-b [build type]               build type(default: $BUILD_TYPE, available: Debug, Release, RelWithDebInfo, MinSizeRel)";
            echo "-c [android stl]              stl used by ndk(default: $ANDROID_STL, available: system, stlport_static, stlport_shared, gnustl_static, gnustl_shared, c++_static, c++_shared, none)";
            echo "-n [ndk root directory]       ndk root directory.(default: $DEVELOPER_ROOT)";
            echo "-l [api level]                API level, see $NDK_ROOT/platforms for detail.(default: $ANDROID_NATIVE_API_LEVEL)";
            echo "-r [source dir]               root directory of this library";
            echo "-t [toolchain]                ANDROID_TOOLCHAIN.(gcc version/clang, default: $ANDROID_TOOLCHAIN, @see CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION in cmake)";
            echo "-i [lua include dir]          Lua include dir, which should has [$ARCHS/]include directory in it.";
            echo "-f [lua library file pattern] Lua library search pattern for lua library file, which will be linked into protobuf.so.";
            echo "-u [lua library dir]          Lua library dir, which should has [$ARCHS] which we can find a lua lib in it.";
            echo "-h                            help message.";
            exit 0;
        ;;
        l)
            ANDROID_NATIVE_API_LEVEL=$OPTARG;
        ;;
        r)
            SOURCE_DIR="$OPTARG";
        ;;
        t)
            ANDROID_TOOLCHAIN="$OPTARG";
        ;;
        i)
            LUAINCDIR="$OPTARG";
        ;;
        u)
            LUALIBDIR="$OPTARG";
        ;;
        f)
            LUALIBNAME=($OPTARG);
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

##########
if [ ! -e "$SOURCE_DIR/CMakeLists.txt" ]; then
    echo "$SOURCE_DIR/CMakeLists.txt not found";
    exit -2;
fi
SOURCE_DIR="$(cd "$SOURCE_DIR" && pwd)";
if [ -z "$NDK_ROOT" ]; then
    echo "ndk root must be assigned.";
    exit 0;
fi

NDK_ROOT="$(cd "$NDK_ROOT" && pwd)";
LUAINCDIR="$(cd "$LUAINCDIR" && pwd)";
LUALIBDIR="$(cd "$LUALIBDIR" && pwd)";

CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=$ANDROID_TOOLCHAIN;
if [ "${ANDROID_TOOLCHAIN:0:5}" != "clang" ]; then
    ANDROID_TOOLCHAIN="gcc";
fi

for ARCH in ${ARCHS}; do
    echo "================== Compling $ARCH ==================";
    echo "Building pbc for android-$ANDROID_NATIVE_API_LEVEL ${ARCH}"
    
    # sed -i.bak '4d' Makefile;
    echo "Please stand by..."
    if [ -e "$WORKING_DIR/build-$ARCH" ]; then
        rm -rf "$WORKING_DIR/build-$ARCH";
    fi
    mkdir -p "$WORKING_DIR/build-$ARCH";
    cd "$WORKING_DIR/build-$ARCH";
    
    mkdir -p "$WORKING_DIR/prebuilt/$ARCH";

    ARCH_LUAINCDIR="";
    ARCH_LUALIBPATH="";
    ARCH_LUAVER="5.1";
    if [ ! -z "$LUAINCDIR" ] && [ ! -z "$LUALIBDIR" ]; then
        if [ -e "$LUAINCDIR/$ARCH/lua.h" ]; then
            ARCH_LUAINCDIR="$LUAINCDIR/$ARCH";
        elif [ -e "$LUAINCDIR/lua.h" ]; then
            ARCH_LUAINCDIR="$LUAINCDIR";
        else
            echo -e "\033[34mLua header not found in $LUAINCDIR or $LUAINCDIR/$ARCH, we will skip build lua binding for $ARCH \033[0m";
            ARCH_LUAINCDIR="";
        fi
        
        for SEARCH_PATTERN in ${LUALIBNAME[@]}; do
            ARCH_LUALIBPATH=($(find "$LUALIBDIR/$ARCH" -name "$SEARCH_PATTERN"));
            if [ ${#ARCH_LUALIBPATH} -gt 0 ]; then
                ARCH_LUALIBPATH="${ARCH_LUALIBPATH[0]}";
                break
            else
                ARCH_LUALIBPATH="";
            fi
        done

        if [ -z "$ARCH_LUALIBPATH" ]; then
            echo -e "\033[34mLua library not found in $LUALIBDIR/$ARCH with pattern=${LUALIBNAME[@]}, we will skip build lua binding for $ARCH \033[0m";
            ARCH_LUALIBPATH=""
        fi
    fi

    if [ ! -z "$ARCH_LUAINCDIR" ] && [ ! -z "$ARCH_LUALIBPATH" ]; then
        ARCH_LUAVER=$(grep LUA_VERSION_NUM $ARCH_LUAINCDIR/lua.h | awk '{print $3}');
        if [ $ARCH_LUAVER -ge 503 ]; then
            ARCH_LUAVER="5.3";
        else
            ARCH_LUAVER="5.1";
        fi
    fi

    # 64 bits must at least using android-21
    # @see $NDK_ROOT/build/cmake/android.toolchain.cmake
    echo $ARCH | grep -E '64(-v8a)?$' ;
    if [ $? -eq 0 ] && [ $ANDROID_NATIVE_API_LEVEL -lt 21 ]; then
        ANDROID_NATIVE_API_LEVEL=21 ;
    fi

    if [ ! -z "$ARCH_LUAINCDIR" ] && [ ! -z "$ARCH_LUALIBPATH" ]; then
        echo -e "\033[32m Build pbc and protobuf binding($ARCH_LUAINCDIR, $ARCH_LUALIBPATH, version=$ARCH_LUAVER) \033[0m";
        cmake "$SOURCE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX="$WORKING_DIR/prebuilt/$ARCH" \
            -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$WORKING_DIR/lib/$ARCH" -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$WORKING_DIR/lib/$ARCH" \
            -DCMAKE_TOOLCHAIN_FILE="$NDK_ROOT/build/cmake/android.toolchain.cmake" \
            -DANDROID_NDK="$NDK_ROOT" -DCMAKE_ANDROID_NDK="$NDK_ROOT" \
            -DANDROID_NATIVE_API_LEVEL=$ANDROID_NATIVE_API_LEVEL -DCMAKE_ANDROID_API=$ANDROID_NATIVE_API_LEVEL \
            -DANDROID_TOOLCHAIN=$ANDROID_TOOLCHAIN -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=$CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION \
            -DANDROID_ABI=$ARCH -DCMAKE_ANDROID_ARCH_ABI=$ARCH \
            -DANDROID_STL=$ANDROID_STL -DCMAKE_ANDROID_STL_TYPE=$ANDROID_STL \
            -DANDROID_PIE=YES -DLUA_INCLUDE_DIR="$ARCH_LUAINCDIR" -DLUA_VERSION_STRING=$ARCH_LUAVER -DLUA_LIBRARIES="$ARCH_LUALIBPATH" "$@";
    else
        echo -e "\033[32m Build pbc \033[0m";
        cmake "$SOURCE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX="$WORKING_DIR/prebuilt/$ARCH" \
            -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$WORKING_DIR/lib/$ARCH" -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$WORKING_DIR/lib/$ARCH" \
            -DCMAKE_TOOLCHAIN_FILE="$NDK_ROOT/build/cmake/android.toolchain.cmake" \
            -DANDROID_NDK="$NDK_ROOT" -DCMAKE_ANDROID_NDK="$NDK_ROOT" \
            -DANDROID_NATIVE_API_LEVEL=$ANDROID_NATIVE_API_LEVEL -DCMAKE_ANDROID_API=$ANDROID_NATIVE_API_LEVEL \
            -DANDROID_TOOLCHAIN=$ANDROID_TOOLCHAIN -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=$CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION \
            -DANDROID_ABI=$ARCH -DCMAKE_ANDROID_ARCH_ABI=$ARCH \
            -DANDROID_STL=$ANDROID_STL -DCMAKE_ANDROID_STL_TYPE=$ANDROID_STL \
            -DANDROID_PIE=YES "$@";
    fi
    cmake --build . --target install
done

echo "Building done.";
