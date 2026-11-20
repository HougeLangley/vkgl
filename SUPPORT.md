# Support

## Scope

vkgl is experimental and community supported. There is no service-level
guarantee, complete Vulkan conformance claim, or promise of support for every
GPU, compositor, format, or application. Read `HARDWARE.md` and the relevant
branch documentation before reporting a problem.

## Where to ask

After this source is published, use the hosting repository's issue tracker for
reproducible bugs and its discussion/feature channel for design questions. Use
the issue forms and `SUPPORT.md` checklist. Use the private process in
`SECURITY.md` instead of a public issue for vulnerabilities.

No online repository was created during the standalone export, so this file
intentionally does not invent a project URL or support mailbox. Distribution
users may also use their distribution's normal package support channel; identify
that you are testing a standalone/staged vkgl build rather than the production
package.

## Before requesting help

Provide:

1. `main` or experimental branch and full commit;
2. architecture, GPU/SoC, actual Vulkan device name, OS and kernel;
3. compiler, Wayland/EGL/GBM/vendor stack and SPIRV-Cross provider/version;
4. exact configure/build/install/test commands and whether the ICD was staged;
5. minimal public source reproducer and expected versus actual result;
6. relevant redacted log excerpt and negative/unsupported behavior;
7. whether host-unit, LG110 hardware, and reproducibility tests were run.

Never upload tokens, private keys, customer data, full private paths, device
serials, historical driver binaries, or large core/GPU dumps. A maintainer may
ask for a smaller reproducer or additional public test evidence.

## Production issues

If a production system is affected, stop the experiment, restore the prior
package/manifest using the distribution's tested rollback path, and report
through that distribution. Do not replace additional production files while
trying to make a standalone test pass.
