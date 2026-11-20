# Experimental external dma-buf import candidate

## Status — independent functional validation, not production

This branch contains the candidate `src/vkgl.cpp` with SHA256
`12f5275a95098f1017651f23b40bd4fc9df8f1031a0892cec99633d1097d6724`
and public client source `tests/hardware/external-memory-client.cpp` with
SHA256 `94e5a36c17c4d23746c82ce78f3ba9b0721903a9cff4ee31c9e62d998e70a8ca`.

**Functionality has been independently verified for the narrow scope described
below. It has not been released to production.** The standalone repository
export separately compiles the candidate and runs host-unit tests; it does not
turn prior hardware evidence into a new hardware run or a production approval.

For the measured workload used by the candidate evaluation, the candidate
median was approximately **15.35% slower** than the baseline. This is a
workload-specific median regression, not a universal performance constant.
The regression remains disclosed and must be root-caused and reviewed before
any production decision. The standalone export did not rerun that benchmark.

## Independently verified functional scope

The prior candidate-6848 hardware verification exercised real LG110 dma-buf
import with exact CPU-source and Vulkan readback comparisons:

- single-plane linear (`drmFormatModifier == 0`) BGRA and RGBA images;
- sampled and transfer-source usage, explicit row pitch, and plane offsets 0,
  64, and 256 bytes;
- dedicated, device-local-only external memory requirements and allocation;
- five positive imports with exact byte comparisons (400 compared bytes total);
- successful-import FD consumption and failure-path caller FD ownership;
- rejection of non-dedicated, wrong-size, wrong-memory-type, and invalid-FD
  allocations;
- rejection of unsupported formats, usages, modifiers, plane counts, strides,
  offsets, layouts, and opaque-FD handle queries;
- device-local external-memory map failure, DRM modifier/layout queries,
  `vkTrimCommandPoolKHR`, and complete teardown.

The public client preserves positive and intentional-negative checks so an
independent reviewer can rerun the same class of evidence. A result is valid
only against the explicitly selected candidate manifest and a real non-llvmpipe
LoongGPU device.

## Explicit non-goals and unsupported boundaries

This candidate does **not** implement or claim:

- VPU decode/encode, YUV or multi-planar support, or Vulkan Video;
- dma-buf/external-memory export (`vkGetMemoryFdKHR` is tested as unsupported);
- external synchronization or external semaphore/fence import/export;
- tiled layouts or non-linear DRM modifiers;
- full zero-copy, universal zero-copy, or guaranteed direct scanout;
- arbitrary formats, usages, plane layouts, memory types, or cross-device
  interoperability;
- complete Vulkan conformance or production readiness.

Successful import does not prove zero copies, displayability, video support, or
synchronization interoperability. Those are separate roadmap items requiring
their own implementation and evidence.

## Build and host-unit verification

From the branch root, use a clean out-of-tree directory:

```sh
cmake -S . -B build/external -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DVKGL_FETCH_SPIRV_CROSS=ON \
  -DVKGL_BUILD_EXTERNAL_MEMORY_TEST=ON
cmake --build build/external --parallel
ctest --test-dir build/external --output-on-failure -L host-unit
```

This compiles the hardware client but does not run it. The source hash test now
pins the candidate hash rather than the `main` production hash. Do not copy the
candidate library or manifest into a production ICD directory.

## Opt-in LG110 hardware run

Only on an explicitly selected LG110 test system:

```sh
scripts/test-external-dmabuf.sh build/external \
  "$PWD/build/external/vkgl_icd_build.json"
```

For only the five positive cases without the intentional-negative matrix:

```sh
scripts/test-external-dmabuf.sh build/external \
  "$PWD/build/external/vkgl_icd_build.json" --positive-only
```

The script selects the build-tree manifest only in the client process. Record
the full commit, both source hashes, environment, command, positive/negative
counts, exact-byte result, teardown, and any failure. Do not report a skipped
hardware run as a pass.

## Development and verification route to a decision

1. **P0 correctness review:** independently audit API invariants, plane-span
   arithmetic, dedicated allocation, map behavior, FD ownership, error cleanup,
   and exact-pixel tests. Acceptance: all positive and intentional-negative
   cases pass on the declared LG110 environment with no resource leaks.
2. **P0 regression analysis:** reproduce the measured workload and disclose
   baseline/candidate medians, sample count, variance, and profiles. Acceptance:
   the approximately 15.35% median regression is explained, and correctness
   does not depend on hiding or weakening performance checks.
3. **P0 security review:** validate untrusted layouts/FDs, overflow boundaries,
   process ownership, and dependency/source provenance. Acceptance: malformed
   inputs fail safely and public evidence contains no private data or secrets.
4. **P1 scope expansion:** design YUV, export, external synchronization, tiled
   modifiers, and zero-copy separately. Acceptance: each advertised capability
   has its own query, positive, negative, interoperability, and teardown tests;
   none is inferred from this import candidate.
5. **P1 release gate:** stage a package without changing the running ICD,
   repeat independent hardware and performance review, and rehearse restoring
   the previous manifest/package. Acceptance: exact commit approval and a
   successful rollback drill. Until then, the candidate remains unreleased and
   off `main`.
