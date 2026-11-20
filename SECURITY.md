# Security policy

## Supported scope

Security fixes are prioritized for reviewed `main` releases. Experimental
branches, including external-memory candidates, are not production-supported
and must not be deployed as a security boundary. Host unit tests and secret
scans are useful checks, not a security certification.

## Report privately

Do not open a public issue for an undisclosed vulnerability. In the repository
where vkgl is published, use **Security → Advisories → Report a vulnerability**
(GitHub private vulnerability reporting) when available. If that facility is
not enabled, use the repository owner's listed private contact or the private
security channel of the distribution carrying the package. Do not send reports
to a public issue, pull request, mailing list, or chat.

This standalone source export does not invent a public mailbox or online
repository. Publication owners must enable and monitor one of the private
channels above rather than treating this file as permission to disclose
publicly.

Include the affected commit/release, hardware and software environment, impact,
reproduction steps, and a minimal safe proof of concept. Remove credentials,
private paths, personal data, and unrelated GPU/memory contents. A maintainer
should acknowledge within seven days and coordinate severity, fix, disclosure
timing, and credit with the reporter; this is a response goal, not a guarantee.

## Priority areas

- DMA-buf/FD ownership, duplication, close-on-failure, and cross-process access;
- memory lifetime, integer overflow, mapping, and malformed image/plane layout;
- untrusted SPIR-V and shader translation through SPIRV-Cross;
- command/fence synchronization and device-reset behavior;
- ICD manifest/library selection and unsafe global environment overrides;
- dependency substitution, unpinned downloads, secrets, and CI runner trust.

Dependency fallback downloads use HTTPS and a pinned SHA256. Reviewers should
prefer a trusted system SPIRV-Cross, verify source provenance, and never place
publishing tokens on an untrusted hardware runner.

## Coordinated release and rollback

A security release must identify affected versions, provide a reviewed patch
and clean build/test evidence, stage the package, and document a reversible
rollback/mitigation. Do not overwrite a running production ICD as part of
validation. If safe details cannot yet be published, provide a truthful
mitigation without claiming the root cause is fixed.
