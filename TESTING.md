# Testing vkgl

## Test lanes

### 1. Host unit — no GPU required

```sh
scripts/test-host.sh build/host -DVKGL_FETCH_SPIRV_CROSS=ON
```

The CTest `host-unit` label includes:

- `host-fence`: deterministic C++ tests for submission, cleanup, host signal,
  fallback, wait-any polling, wait-all timeout, empty waits, and errors;
- `host-protocol`: C checks of generated Wayland interface versions and message
  signatures;
- `host-repository-policy`: Python checks for the branch-pinned source hash,
  required community files, private paths, source-only artifacts, shader source,
  architecture-specific compile flags, and manifest/library agreement;
- `host-secret-scan`: offline detection of common credential forms without
  printing candidate secret contents.

The shader fixtures are public GLSL source, not precompiled opaque binaries.
Host tests never open `/dev/dri`, replace an ICD, or establish hardware
correctness.

### 2. Reproducible build

```sh
scripts/reproduce-build.sh "$PWD/build/repro-a" "$PWD/build/repro-b" \
  -DVKGL_FETCH_SPIRV_CROSS=ON
```

The script configures and builds two clean directories, prints both SHA256
values, and requires byte-for-byte equality. The claim is limited to the same
declared toolchain and dependency provider; it is not universal reproducibility
across arbitrary compilers.

### 3. Optional LG110 smoke

```sh
scripts/test-lg110.sh build/hardware \
  "$PWD/build/hardware/vkgl_icd_build.json"
```

This builds `tests/hardware/lg110_vulkan_smoke.cpp`, selects the staged manifest
in the child process, and requires a non-llvmpipe LoongGPU instance/device. It
is intentionally absent from the `host-unit` CTest label. Record a skip or
failure with its environment; never relabel it as a pass.

### 4. Experimental external dma-buf

On `experimental/external-dmabuf`, build and run the separately documented
`tests/hardware/external-memory-client.cpp` entry point. It is a candidate-only
LG110 test and is never part of host-unit. See `EXTERNAL_DMABUF.md` on that
branch for commands, validated scope, exclusions, and the measured regression.

## Evidence rules

A result is tied to source only when it includes the full commit, source hash
where required, build directory, exact commands, dependency provider, and test
output. Keep large logs and generated binaries outside the source repository.
Redact usernames, tokens, device serials, customer data, and private paths from
public reports without changing the semantic result.

For a bug fix, include a minimal public source reproducer and relevant negative
cases. For performance, disclose workload, warm-up, sample count, baseline and
candidate medians, and regressions. For an unsupported feature, a correct
`VK_ERROR_*` result can be the expected test outcome; silently returning
`VK_SUCCESS` is not.

## Suggested report block

```text
Branch/commit:
Architecture/device:
OS/compiler/Wayland:
SPIRV-Cross provider/version:
Configure/build commands:
Test command:
Result (pass/fail/not run):
Relevant output:
Known limitations and residual risk:
```
