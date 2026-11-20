# Hardware support and boundaries

## Support matrix

| Platform | Status on `main` | Evidence boundary |
| --- | --- | --- |
| Loongson 3A6000 + LG110, AOSC OS 14.0.1 | Reference development platform for the 0.1.36 baseline | GCC 16.2/Wayland 1.26 environment and standalone clean build; hardware tests are opt-in and must be reported separately |
| loongarch64 without LG110 | Host build/unit only | No device capability is claimed |
| Other architectures | Portable host build/unit supported by architecture-scoped flags | No LoongArch ISA/tuning flags; GPU support requires independent testing |
| Other GPUs / vendor stacks | Contributions welcome | Not advertised as supported until a public capability matrix and hardware tests exist |
| llvmpipe / software Vulkan | Not an LG110 validation target | The LG110 smoke client rejects a software device masquerading as the selected result |

## Current LG110 assumptions

The baseline vendor stack exposes OpenGL ES through EGL/GBM. The source
currently selects `/dev/dri/renderD128`, the `gsgpu` GBM backend, and known
vendor EGL JSON names. Those are baseline limitations, not universal device
discovery rules. Tests must record render-node permissions, selected EGL
vendor, compositor, and actual Vulkan device name without publishing sensitive
system information.

The ICD targets selected Vulkan 1.0 behavior needed by simple applications. It
is not a full Vulkan driver and does not implement VPU decode/encode, Vulkan
Video, arbitrary YUV formats, tiled layouts, external synchronization, or
zero-copy guarantees. A Vulkan extension string alone is not proof of a usable
path.

## Required hardware report

For every hardware result include:

- branch and full commit;
- device/SoC and actual Vulkan device name;
- OS, kernel, compiler, Wayland/EGL/GBM/vendor stack versions;
- build provider and SPIRV-Cross version/hash;
- exact staged manifest selection and command;
- workload, dimensions/format/modifier/stride/offset, sample count, and result;
- unsupported/negative cases and teardown behavior;
- performance baseline/candidate medians and variance when relevant.

Do not turn one LG110 result into a claim about all Loongson GPUs or all
LoongArch systems. Other hardware should add its own row only after independent
validation.
