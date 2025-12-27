#!/bin/bash
set -eu
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done
if [ ! -v gxx ];     then clangxx=1; fi
if [ ! -v release ]; then debug=1; fi
if [ -v debug ];     then echo "[debug mode]"; fi
if [ -v release ];   then echo "[release mode]"; fi
if [ -v clangxx ];     then compiler="${CC:-clang++}"; echo "[clang++ compile]"; fi
if [ -v gxx ];       then compiler="${CC:-g++}"; echo "[g++ compile]"; fi

# --- Unpack Command Line Build Arguments -------------------------------------
auto_compile_flags=''

# --- Get Current Git Commit Id -----------------------------------------------
git_hash=$(git describe --always --dirty)
git_hash_full=$(git rev-parse HEAD)

# --- Compile/Link Line Definitions -------------------------------------------
clangxx_common="-I../src/ -I/usr/include/freetype2/ -I../local/ -D_GNU_SOURCE -g -DBUILD_GIT_HASH=\"$git_hash\" -DBUILD_GIT_HASH_FULL=\"$git_hash_full\" -Wno-unknown-warning-option -fdiagnostics-absolute-paths -Wall -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Wno-for-loop-analysis -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
clangxx_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${clangxx_common} ${auto_compile_flags}"
clangxx_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${clangxx_common} ${auto_compile_flags}"
clangxx_link="-lpthread -lm -lrt -ldl"
clangxx_out="-o"
gxx_common="-I../src/ -I../local/ -g -D_GNU_SOURCE -DBUILD_GIT_HASH=\"$git_hash\" -DBUILD_GIT_HASH_FULL=\"$git_hash_full\" -Wno-unknown-warning-option -Wall -Wno-missing-braces -Wno-unused-function -Wno-attributes -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-compare-distinct-pointer-types -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
gxx_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${gxx_common} ${auto_compile_flags}"
gxx_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${gxx_common} ${auto_compile_flags}"
gxx_link="-lpthread -lm -lrt -ldl"
gxx_out="-o"

# --- Per-Build Settings ------------------------------------------------------
link_dll="-fPIC"
link_os_gfx="-lX11 -lXext"
link_render="-lvulkan"
link_font_provider="-lfreetype"

# --- Choose Compile/Link Lines -----------------------------------------------
if [ -v gxx ];     then compile_debug="$gxx_debug"; fi
if [ -v gxx ];     then compile_release="$gxx_release"; fi
if [ -v gxx ];     then compile_link="$gxx_link"; fi
if [ -v gxx ];     then out="$gxx_out"; fi
if [ -v clangxx ];   then compile_debug="$clangxx_debug"; fi
if [ -v clangxx ];   then compile_release="$clangxx_release"; fi
if [ -v clangxx ];   then compile_link="$clangxx_link"; fi
if [ -v clangxx ];   then out="$clangxx_out"; fi
if [ -v debug ];   then compile="$compile_debug"; fi
if [ -v release ]; then compile="$compile_release"; fi

# --- Prep Directories --------------------------------------------------------
mkdir -p build
mkdir -p local

# --- Build GLFW Submodule ----------------------------------------------------
if [ ! -f local/libglfw3.a ]
then
  echo "[building glfw]"
  mkdir -p build/glfw
  cd build/glfw
  
  # Configure and build GLFW with cmake
  cmake ../../external/glfw \
    -DCMAKE_BUILD_TYPE=Release \
    -DGLFW_BUILD_DOCS=OFF \
    -DGLFW_BUILD_TESTS=OFF \
    -DGLFW_BUILD_EXAMPLES=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DGLFW_BUILD_WAYLAND=OFF \
    -DGLFW_BUILD_X11=ON
  
  make -j$(nproc)
  
  # Copy the library to local
  cp src/libglfw3.a ../../local/
  cd ../..
fi

# --- Build Everything (@build_targets) ---------------------------------------
cd build
if [ -v tinyrt ];                then didbuild=1 && $compile ../src/tinyrt/tinyrt_main.cpp                                   $compile_link $link_os_gfx $link_render $out tinyrt; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [ ! -v didbuild ]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh raddbg\` or \`./build.sh rdi_from_pdb\`."
  exit 1
fi
