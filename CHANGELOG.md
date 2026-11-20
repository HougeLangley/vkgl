# Changelog

All notable changes to this standalone source tree are documented here. The
format follows Keep a Changelog and versions describe source releases, not
unverified capability claims.

## [0.1.36-external-dmabuf.0] - Unreleased

### Added

- Experimental single-plane linear dma-buf import candidate and its public
  positive/negative LG110 client, source hashes, and hardware entry point.
- Independent functional-validation scope and explicit unsupported boundaries
  in `EXTERNAL_DMABUF.md`.

### Performance and release boundary

- The measured candidate workload median was approximately 15.35% slower than
  baseline. The regression is disclosed and remains a release gate.
- Functionality was independently verified only for the documented narrow
  scope. This candidate has not been released to production and does not add
  VPU/YUV/Vulkan Video, export, external synchronization, tiled layouts, or
  full zero-copy.

## [0.1.36] - 2026-11-20

### Added

- Standalone `src`, `protocol`, `icd`, `scripts`, and `tests` layout.
- Portable CMake build with system-first SPIRV-Cross discovery and a
  hash-pinned public source fallback.
- Native loongarch64 `-O3 -march=loongarch64 -mtune=la664 -flto` flags scoped
  away from other architectures.
- Public C/C++/Python host tests, GLSL source fixtures, reproducibility check,
  offline secret scan, and opt-in LG110 smoke client.
- Safe staged installation guidance, CI templates, and complete community,
  security, support, hardware, testing, contribution, and roadmap documents.

### Changed

- The original monorepo build helper was replaced by an out-of-tree CMake build
  without changing the authoritative `src/vkgl.cpp` bytes or its pinned SHA256.
- The ICD manifest is generated for the selected install prefix or build tree;
  no production manifest or library is overwritten by the source build.

### Boundaries

- This is not a complete Vulkan implementation, VPU/Vulkan Video driver, or a
  hardware certification. Experimental external dma-buf work is kept off this
  production-source branch.
