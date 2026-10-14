#!/usr/bin/env bash
# Configure and build the findMUS tests against the libminizinc SDK and Gecode
# already in $ROOT (see fetch_minizinc.sh), then run them.
#
# Env: ROOT, CMAKE_GENERATOR, CMAKEDIR (as for ci_build.sh)
set -eux

: "${ROOT:?ROOT must be set}"
: "${CMAKEDIR:?CMAKEDIR must be set}"

# Debug, so the enumerators' assertions are checked too.
cmake -S "$ROOT" -B "$ROOT/build" -G "${CMAKE_GENERATOR:-Ninja}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_FINDMUS_TESTS=ON \
  -DUSE_SYSTEM_LIBMINIZINC=1 \
  -Dlibminizinc_DIR="$ROOT/minizinc/${CMAKEDIR}/libminizinc" \
  -DGecode_ROOT="$ROOT/vendor/gecode" \
  -DOsiCBC_ROOT="$ROOT/vendor/cbc"
cmake --build "$ROOT/build" --target tests

# The tests find their specs relative to the build directory, and option
# parsing needs a standard library even though the tests never use it.
cd "$ROOT/build"
MZN_STDLIB_DIR="$ROOT/minizinc/share/minizinc" ./tests
