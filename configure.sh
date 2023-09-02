# remember to actually set these..
export TOOLCHAIN_BASE=/home/mint/geode/toolchain_linux
export SPLAT_DIR=$TOOLCHAIN_BASE/splat
export TOOLCHAIN_REPO=$TOOLCHAIN_BASE/toolchain

export HOST_ARCH=x86
export MSVC_BASE=$SPLAT_DIR/crt
export WINSDK_BASE=$SPLAT_DIR/sdk
# TODO: get this from the thing..
export WINSDK_VER=10.0.22000
# change this to your llvm version!!!
export LLVM_VER=14
export CLANG_VER=$LLVM_VER

# # you can also use `-G Ninja` here
# cmake \
#   -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_REPO/clang-cl-msvc.cmake \
#   -DCMAKE_BUILD_TYPE=Release -DGEODE_DISABLE_FMT_CONSTEVAL=1 -B build -G Ninja
  
# # cmake --build build --config Release
