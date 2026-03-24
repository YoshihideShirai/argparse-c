#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "usage: $0 <build-dir> <install-prefix> <c-compiler>" >&2
  exit 2
fi

build_dir="$1"
install_prefix="$2"
c_compiler="$3"

cmake --install "$build_dir" --prefix "$install_prefix"

cmake -S test/consumer/cmake -B consumer-build/cmake \
  -G Ninja \
  -DCMAKE_C_COMPILER="$c_compiler" \
  -DCMAKE_PREFIX_PATH="$install_prefix"
cmake --build consumer-build/cmake --parallel
./consumer-build/cmake/argparse_c_consumer

libdir="$(cmake -LA -N "$build_dir" | awk -F= '/^CMAKE_INSTALL_LIBDIR:PATH=/{print $2}')"
if [ -z "$libdir" ]; then
  libdir="lib"
fi

export PKG_CONFIG_PATH="$install_prefix/$libdir/pkgconfig"
export LD_LIBRARY_PATH="$install_prefix/$libdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
"$c_compiler" test/consumer/pkg-config/main.c \
  -o consumer-build/pkg-config-consumer \
  $(pkg-config --cflags --libs argparse-c)
./consumer-build/pkg-config-consumer
