# vkgl roadmap

This roadmap is a development and verification plan, not a capability claim.
`main` is the 0.1.36 source baseline. Experimental branches do not become
production merely because they compile or pass a focused client.

Priorities: **P0** protects correctness/data and honest unsupported behavior;
**P1** enables useful interoperable workloads; **P2** expands coverage and
maintainability. Every item requires public source tests where practical and
hardware evidence for hardware-visible behavior.

## Current baseline

- Implemented: a limited Vulkan 1.0 subset over EGL/GLES2/GBM/Wayland,
  including the existing instance/device/memory/buffer/image/render/command,
  pipeline, clear, and selected WSI paths present in the baseline source.
- Verified during standalone export: clean loongarch64 compile and host-unit
  coverage of fence helpers, generated protocol metadata, source policy, and
  architecture flag scoping.
- Not established by those checks: full Vulkan conformance, all formats/WSI,
  VPU/YUV/Vulkan Video, external-memory capability on `main`, tiled layouts,
  external synchronization, or performance across arbitrary workloads.

## P0 — correctness, safe failure, synchronization, and security

| Work item | Required development work | Acceptance evidence |
| --- | --- | --- |
| API correctness inventory | Map implemented entry points to Vulkan 1.0 requirements; reject unsupported structures/enums instead of silently accepting them. | Public negative tests cover each reviewed path; no false success; unsupported surface documented. |
| Memory and lifetime safety | Audit ownership, dispatchable handles, mapping, destruction order, overflow, and error cleanup. | Sanitizer host tests where separable; hardware lifetime/negative suite; no leak/use-after-free/double-close in reviewed paths. |
| Fence/semaphore semantics | Test host events, EGL fence fallback, wait-any/all, zero/finite/infinite timeouts, overflow, and submission failures. | Deterministic host unit matrix plus LG110 tests; no manufactured signals or stranded earlier fence. |
| Format truthfulness | Publish a per-format usage/tiling/features table and reject unsupported YUV/multi-planar/modifier/layout requests. | Format query and creation tests match documentation and exact device results; no unsupported YUV advertisement. |
| Security boundaries | Threat-model FD ownership, untrusted shader/SPIR-V input, environment selection, manifest paths, and dependency fetching. | Offline secret scan; pinned dependency hash; malformed-input tests; private advisory process exercised; no secrets in public evidence. |
| Regression triage | Reduce every correctness report to a public minimal source reproducer where disclosure permits. | Reproducer fails before and passes after; adjacent paths checked; result and residual risks recorded. |

## P1 — formats, WSI, dma-buf, external memory, and performance

| Work item | Required development work | Acceptance evidence |
| --- | --- | --- |
| Baseline format coverage | Validate RGBA/BGRA channel order, strides, offsets, extents, usage flags, and copy/readback paths. | Exact-byte tests across boundary sizes, offsets, strides, and failure cases on declared hardware. |
| Wayland WSI | Cover shm/dma-buf feedback, acquire/present ordering, resize, minimization, compositor absence, and teardown. | Public client source; repeated present/resize tests; no stale buffer use; software/non-LG110 results separated. |
| Linux dma-buf integration | Validate protocol versions, modifiers, plane metadata, and compositor negotiation without assuming linear-only behavior forever. | Metadata and interoperability tests for each advertised format/modifier; unsupported tiled paths fail cleanly. |
| External dma-buf import candidate | Independently review `experimental/external-dmabuf`; test FD ownership, dedicated allocation, exact pixels, negative cases, and cleanup. | Full candidate client and independent review pass; measured workload median regression (about 15.35%) disclosed and root-caused; no production release until gates pass. |
| External-memory completion | Determine separately whether export, non-dedicated paths, additional memory types, and cross-process use can be supported. | No umbrella claim: each handle type/direction has query/allocation/ownership/interop tests; unsupported export remains explicit. |
| External synchronization | Research native fence/sync FD support and mapping to EGL/Vulkan synchronization. | Import/export wait and signal tests across processes and failure paths; no claim until supported end to end. |
| Performance discipline | Profile baseline and candidate with identical workload, environment, warm-up, sample count, and medians. | Publish raw summarized source data and variance; correctness unchanged; regressions have owner and acceptance threshold. |

The external-memory candidate is explicitly not a VPU, YUV, Vulkan Video,
export, external-sync, tiled, or full-zero-copy implementation. Those require
separate designs and tests; none can be inferred from dma-buf import success.

## P1 — release and rollback

| Work item | Required development work | Acceptance evidence |
| --- | --- | --- |
| Reproducible source release | Pin dependency source and hash, document system-provider preference, build twice from clean directories, and generate source-only archives. | Byte-identical ICD in the declared environment or explained differences; no binaries/private paths/secrets in source archive. |
| Staged package validation | Use a non-production prefix and per-process loader selection; verify installed manifest/library agreement. | Fresh install under staging passes host and hardware suites without changing the running driver. |
| Production gate | Require independent correctness, security, performance, and packaging review with signed-off commits. | Verifier records exact commit/hash/environment and explicit approval; experimental evidence alone is insufficient. |
| Rollback drill | Preserve the prior package and configuration; define a reversible switch and health check. | A staged rollback restores the prior ICD manifest and device behavior without deleting user data; recovery evidence attached. |

## P2 — CI, cross-hardware, and maintainability

| Work item | Required development work | Acceptance evidence |
| --- | --- | --- |
| Public CI | Build with the pinned dependency on x86_64, run host-unit and offline secret checks, and periodically verify reproducibility. | Actions logs are public and reproducible; LoongArch flags never leak into x86_64 commands. |
| Native LoongArch CI | Add an isolated runner only with explicit security controls and no production driver mutation. | Clean native build/tests use staging; runner contains no long-lived publishing credential. |
| Other hardware | Welcome backend-specific fixes and new capability matrices without copying LG110 assumptions. | Each device has its own tested matrix, exact client evidence, and honest unsupported paths; common host tests stay portable. |
| Shader/SPIR-V coverage | Add legal public shader source and generated test artifacts reproducibly; test reflection, locations, and malformed input. | Source shaders and generation commands are documented; expected GLSL/reflection and safe rejection tests pass. |
| Documentation and support | Keep README, hardware matrix, testing, changelog, support, and roadmap aligned with each accepted change. | PR checklist links docs and test evidence; no undocumented capability or stale production instruction. |

## Versioning rule

A patch/minor version describes reviewed source, not a cumulative feature
banner. Experimental capabilities use branch/pre-release identifiers until
their acceptance rows and independent release gates are complete. A rollback
or security fix must update `CHANGELOG.md` with affected versions and concrete
mitigation steps.
