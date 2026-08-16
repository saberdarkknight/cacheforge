#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_type="${1:-Debug}"
build_dir="${project_dir}/build"

cmake -S "${project_dir}" -B "${build_dir}" -DCMAKE_BUILD_TYPE="${build_type}"
cmake --build "${build_dir}"
ctest --test-dir "${build_dir}" --output-on-failure
