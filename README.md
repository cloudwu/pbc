## PBC

[![travis-ci status](https://travis-ci.org/cloudwu/pbc.svg?branch=master)](https://travis-ci.org/cloudwu/pbc)

PBC is a google protocol buffers library for C without code generation.

## Quick Example

    package tutorial;
    
    message Person {
      required string name = 1;
      required int32 id = 2;        // Unique ID number for this person.
      optional string email = 3;
    
      enum PhoneType {
        MOBILE = 0;
        HOME = 1;
        WORK = 2;
      }
    
      message PhoneNumber {
        required string number = 1;
        optional PhoneType type = 2 [default = HOME];
      }
    
      repeated PhoneNumber phone = 4;
    }

```C
struct pbc_rmessage * m = pbc_rmessage_new(env, "tutorial.Person", slice);
printf("name = %s\n", pbc_rmessage_string(m , "name" , 0 , NULL));
printf("id = %d\n", pbc_rmessage_integer(m , "id" , 0 , NULL));
printf("email = %s\n", pbc_rmessage_string(m , "email" , 0 , NULL));

int phone_n = pbc_rmessage_size(m, "phone");
int i;

for (i=0;i<phone_n;i++) {
	struct pbc_rmessage * p = pbc_rmessage_message(m , "phone", i);
	printf("\tnumber[%d] = %s\n",i,pbc_rmessage_string(p , "number", i ,NULL));
	printf("\ttype[%d] = %s\n",i,pbc_rmessage_string(p, "type", i, NULL));
}

pbc_rmessage_delete(m);
```

## Message API

You can use *wmessage* for encoding , and *rmessage* for decoding.

See test/addressbook.c for details.

## Pattern API

If you need better performance , you can use pbc_pattern_xxx api .

See test/pattern.c for details.

Pattern api is faster and less memory used because it can access data in native C struct.

## Extension

PBC support extension in a very simple way . PBC add a specific prefix to every extension field name. 

## Service

Not supported

## Enum

With message API , you can use both string and integer as enum type . They must be integer in Pattern API. 

## Lua bindings

cd bindings/lua && make

See https://github.com/cloudwu/pbc/tree/master/binding/lua/README.md

## Build with cmake
### Build for native library
```bash
cd REPO
mkdir build
cd build
cmake .. 
# You may also add -G "Visual Studio 15 2017 Win64" for Visual Studio 2017 and win64 or -G "MSYS Makefils" for MinGW
# Or add -DCMAKE_INSTALL_PREFIX=<install dir> to specify where to install when run make install

# Then build
make -j4 # Using gcc/clang and make
MSBuild pbc.sln /verbosity:minimal /target:ALL_BUILD /p:Configuration=RelWithDebInfo /p:Platform=x64 # Using Visual Studio, usually in C:\Program Files (x86)\MSBuild\15.0 or C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\

# At last, install it
make install # Using gcc/clang and make
MSBuild pbc.sln /verbosity:minimal /target:INSTALL /p:Configuration=RelWithDebInfo /p:Platform=x64 # Using Visual Studio
```

### Build lua-binding
Just add lua include directory and library directory to cmake standard search directory. If we can find available lua, we will build lua binding for the right version.

### Cross-Compile for iOS
Just run build_ios.sh in macOS

**CMAKE_INSTALL_PREFIX** can not be set here

```bash
mkdir -p build && cd build && ./build_ios.sh -r ..

# add LUA_INCLUDE_DIR and LUA_VERSION_STRING for build lua binding
mkdir -p build && cd build && ./build_ios.sh -r .. -- -DLUA_INCLUDE_DIR=<where has lua.h> -DLUA_VERSION_STRING=<5.1 or 5.3>
```

All prebuilt static library files and lua file will be generated at $PWD/prebuilt

### Cross-Compile for Android
Just run build_android.sh in Unix like system. We do not support MSYS or MinGW shell now.

**CMAKE_INSTALL_PREFIX** can not be set here

You must at least specify NDK_ROOT
```bash
mkdir -p build && cd build && ./build_android.sh -r .. -n <ndk_root>

# add -i and -u options to build lua binding
mkdir -p build && cd build && ./build_android.sh -r .. -n <ndk_root> -i <where has lua.h or ARCHITECTURE/lua.h> -u <where ARCHITECTURE/[library pattern] in it>

# for example, if we install NDK in /home/prebuilt/android/ndk/android-ndk-r13b, and lua.h in /home/prebuilt/lua/luajit/include/luajit-2.0/ and armeabi-v7a/libluajit-5.1.[a|so] x86/libluajit-5.1.[a|so] x86_64/libluajit-5.1.[a|so] arm64-v8a/libluajit-5.1.[a|so] .. in /home/prebuilt/lua/luajit/include/lib, we can use the command below
mkdir -p build && cd build && ./build_android.sh -r .. -n /home/prebuilt/android/ndk/android-ndk-r13b -i /home/prebuilt/lua/luajit/include/luajit-2.0 -u /home/prebuilt/lua/luajit/include/lib
```

All prebuilt library files and lua file will be generated at $PWD/prebuilt. And library in difference architecture will be placed in $PWD/prebuilt/ARCHITECTURE


## Question ?

* Send me email : http://www.codingnow.com/2000/gmail.gif
* My Blog : http://blog.codingnow.com
* Design : http://blog.codingnow.com/2011/12/protocol_buffers_for_c.html (in Chinese)
* Build for Visual Studio 2012 : https://github.com/miaodadao/pbc


