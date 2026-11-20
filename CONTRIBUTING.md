# Contributing to vkgl

## Everyone and every hardware target is welcome

Contributions from new and experienced developers, downstream distributions,
and other GPU/hardware communities are welcome. A change for one device must
not silently remove another device's path or force LoongArch compiler flags on
other architectures. Hardware-specific claims belong in `HARDWARE.md` with
tested environment details.

## Ground rules

1. Keep `main` production-baseline changes deliberate and reviewable. Put
   unproven capability work in an explicitly named experimental branch.
2. Do not modify a production installation, an original downstream monorepo,
   credentials, remotes, CI secrets, or release infrastructure as part of a
   source test.
3. Do not commit `.so`, `.o`, static libraries, historical binaries, large
   logs, core dumps, GPU dumps, private absolute paths, or credentials.
4. Preserve third-party copyright and license notices. Original contributions
   are licensed under Apache-2.0.
5. Never fabricate a feature, test run, device, performance number, Vulkan
   conformance result, or hardware capability. Clearly separate implemented,
   tested, inferred, and planned behavior.

## Developer Certificate of Origin

Every commit must include a Developer Certificate of Origin 1.1 sign-off:

```text
Signed-off-by: Your Name <your.email@example.org>
```

Use `git commit -s`. The sign-off means you certify the DCO 1.1 terms:
<https://developercertificate.org/>. In particular, you have the right to
submit the work under the project license and the contribution is public.
Keep author and sign-off identities truthful; pseudonyms accepted by the host
must still provide a durable review identity.

## Before sending a change

- Discuss large design changes in an issue first; update `ROADMAP.md` when the
  accepted direction or acceptance criteria change.
- Keep source layout and build instructions portable. Test both the native
  architecture and, where possible, a non-LoongArch build.
- Add or update public C/C++/Python tests and shader source fixtures. Hardware
  tests must remain opt-in and must accept an explicit staged ICD manifest.
- Run `scripts/test-host.sh BUILD_DIR` and, when behavior is affected, the
  relevant hardware test. Run `scripts/reproduce-build.sh A B` for build-system
  or dependency changes.
- Include exact commands, branch/commit, architecture, device, OS, compiler,
  dependency provider/version, output summary, and known limitations in the PR.
  Do not paste tokens, private logs, or customer data.

A host-only PR must say `Hardware: not run` rather than implying it passed. An
unsupported feature must fail honestly; it must not report success to satisfy
a test.

## Review expectations

Reviewers may ask for smaller commits, negative tests, clearer error handling,
format/layout validation, synchronization analysis, or a rollback path.
Performance changes need a reproducible workload, enough samples to explain
variance, baseline and candidate medians, and regressions disclosed alongside
improvements. Correctness and safe failure take priority over benchmark gains.

By participating, you agree to the Code of Conduct and the security reporting
process in `SECURITY.md`.
