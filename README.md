# vkgl

vkgl is a small, experimental Vulkan 1.0 ICD for the Loongson LG110 GPU. It
translates a deliberately limited Vulkan subset to the vendor OpenGL ES 2.0,
EGL, GBM, and Wayland stack. The project exists to make that work inspectable,
testable, and open to contributions from every developer and hardware platform.

**vkgl is not a complete Vulkan implementation, a Vulkan conformance claim, a
VPU driver, or a Vulkan Video implementation.** It is not RADV and does not use
AMD PM4. Do not infer unsupported features from the presence of Vulkan entry
points or extension names.

## Branch and release status

- `main` carries the standalone `0.1.36` source baseline. Its
  `src/vkgl.cpp` is byte-identical to the authoritative production source and
  is pinned by `tests/source-sha256.txt`.
- `experimental/external-dmabuf` is a separate candidate branch. It is not a
  production release; see its `EXTERNAL_DMABUF.md` before evaluating it.
> **Candidate warning on this branch:** narrow external dma-buf import
> functionality has been independently verified, but the measured workload
> median was approximately 15.35% slower and it has not been released to
> production. VPU/YUV/Vulkan Video, export, external synchronization, tiled
> layouts, and full zero-copy are not included. Read
> [EXTERNAL_DMABUF.md](EXTERNAL_DMABUF.md).

- A clean compile and host unit tests do not prove GPU correctness, Vulkan
  conformance, performance, or production readiness.

## Reference development environment

The source baseline was developed on Loongson 3A6000 with LG110, AOSC OS
14.0.1, GCC 16.2, and Wayland 1.26. During the standalone export on
2026-11-20, the loongarch64 host build rechecked GCC 16.2.0, Wayland client
1.26.0, EGL 1.5, GLES 3.2 development metadata, GBM 26.1.5, Vulkan headers
1.4.363, and CMake 4.4.5. These are environment observations, not a promise
that every version combination works.

See [HARDWARE.md](HARDWARE.md) for the support matrix and current device
assumptions.

## Repository layout

- `src/`: vkgl ICD source and fence helper.
- `protocol/`: generated Linux dma-buf Wayland protocol source and its notice.
- `icd/`: installed ICD manifest template; build-tree manifests are generated.
- `scripts/`: out-of-tree build, host test, reproducibility, secret scan, and
  opt-in hardware entry points.
- `tests/host/`: C, C++, and Python tests that need no GPU.
- `tests/shaders/`: public shader source fixtures, not compiled binaries.
- `tests/hardware/`: opt-in hardware clients; never included in host-unit runs.

## Dependencies

Required for the ICD build:

- a C11/C++17 toolchain (GCC is the validated loongarch64 toolchain);
- CMake 3.20 or newer and pkg-config;
- Wayland client, EGL, GLESv2, GBM, and Vulkan development headers;
- SPIRV-Cross C++ GLSL, util, and core headers/libraries;
- Python 3 for host policy and secret-scan tests.

The build first searches system paths and `VKGL_SPIRV_CROSS_ROOT`. If no usable
installation exists, `-DVKGL_FETCH_SPIRV_CROSS=ON` enables a public source
fallback pinned to SPIRV-Cross `vulkan-sdk-1.4.363.0` with SHA256
`f708982e88b763ef5b0394ed468fc2c3628f68cdab89f2e7a946cd628c04e721`.
The fallback fetches source over HTTPS; it does not use credentials, upload
anything, or create a remote repository. Keep the hash check enabled. System
SPIRV-Cross remains the preferred provider.

On a loongarch64 GCC build, vkgl retains `-O3 -march=loongarch64
-mtune=la664 -flto`. Other architectures use `-O3 -flto` without either
LoongArch-specific flag. The host repository-policy test checks the generated
compile commands for this boundary.

## Build

Use a fresh out-of-tree directory. From the repository root:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DVKGL_FETCH_SPIRV_CROSS=ON
cmake --build build/release --parallel
```

For a system or separately built SPIRV-Cross installation, omit the fetch
option and, when needed, provide its prefix:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DVKGL_SPIRV_CROSS_ROOT=/path/to/spirv-cross-prefix
cmake --build build/release --parallel
```

The convenience wrapper is:

```sh
scripts/build.sh build/release -DVKGL_FETCH_SPIRV_CROSS=ON
```

A build-tree `vkgl_icd_build.json` points to the just-built library. Use it only
for an explicitly selected test process.

## Host tests

```sh
scripts/test-host.sh build/host -DVKGL_FETCH_SPIRV_CROSS=ON
# Or, after configuring and building:
ctest --test-dir build/host --output-on-failure -L host-unit
```

Host-unit tests cover fence state transitions, generated protocol metadata,
source integrity, public-file policy, architecture flag scoping, shader source
fixtures, ICD manifest paths, and an offline secret scan. They do not open a
DRM device or contact a GPU. See [TESTING.md](TESTING.md).

To check that two clean build directories produce identical ICD bytes in the
same toolchain/dependency environment:

```sh
scripts/reproduce-build.sh "$PWD/build/repro-a" "$PWD/build/repro-b" \
  -DVKGL_FETCH_SPIRV_CROSS=ON
```

## Optional LG110 hardware smoke test

Only run this on an explicitly selected test machine. It does not replace or
install a system driver:

```sh
scripts/test-lg110.sh build/hardware \
  "$PWD/build/hardware/vkgl_icd_build.json"
```

The client sets `VK_ICD_FILENAMES` only in its own process, requires a
non-llvmpipe LoongGPU device, and creates then destroys a minimal Vulkan
instance/device. A missing GPU, display, permission, or vendor stack is a test
environment result to record—not evidence that host tests failed.

## Safe staged installation

The default install prefix is CMake's `/usr/local`, not the original production
`/usr/lib/vkgl` location. For a non-privileged staged test, choose the prefix at
**configure time** so the installed JSON matches the staged library:

```sh
cmake -S . -B build/stage -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/stage/prefix" \
  -DVKGL_FETCH_SPIRV_CROSS=ON
cmake --build build/stage --parallel
cmake --install build/stage
```

This installs below `build/stage/prefix`; no `sudo` is needed. Test the staged
manifest by setting `VK_ICD_FILENAMES` in one process only. Do not copy the
library or manifest over a running production ICD, do not replace distribution
files, and do not set a global loader override. Package integration and any
production switch require a separate tested release and rollback plan.

## Contributing and community

All developers and hardware efforts are welcome. Read
[CONTRIBUTING.md](CONTRIBUTING.md), [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md),
[ROADMAP.md](ROADMAP.md), and [SUPPORT.md](SUPPORT.md). Contributions require a
DCO `Signed-off-by` line and truthful test evidence. Never advertise a feature
that has not been implemented and independently tested.

Original vkgl work is licensed under Apache-2.0. The generated Wayland protocol
retains its Collabora MIT notice, and SPIRV-Cross remains an external
Apache-2.0 OR MIT dependency. See [LICENSE](LICENSE) and [NOTICE](NOTICE).
