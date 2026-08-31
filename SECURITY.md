# Security Policy

<div align="center">

```
  ┌─────────────────────────────────────┐
  │  🔒  External Base Security Policy │
  └─────────────────────────────────────┘
```

</div>

Thanks for helping keep this repo clean. This is the **open base** — not the
closed FRONTIER product — but secrets and sketchy PRs still hurt everyone.

---

## Report a vulnerability or leaked secret

**Do not** open a public issue for active secrets or exploitable vulnerabilities.

| Channel | When to use |
|---------|-------------|
| [GitHub Security Advisories](https://github.com/binx-ux/external-base/security/advisories/new) | Vulnerabilities in open base code |
| GitHub issue (private details omitted) | General security questions |
| Discord / maintainer DM | Leaked tokens found in a commit |

Include:

- what you found
- file path(s) and commit hash if applicable
- steps to reproduce (for code issues)
- suggested fix if you have one

We aim to acknowledge reports within **72 hours**.

---

## What belongs in this repo

| OK to commit | Never commit |
|--------------|--------------|
| Memory / SDK source | `.exe`, `.dll`, `.sys`, `.pdb` |
| Offset mirrors | Discord webhooks, API keys, OAuth secrets |
| Driver **source** | Built driver binaries |
| Docs + scripts | Closed product source (menu, aimbot, loader) |
| Example configs (no secrets) | License servers / auth bypass code |

---

## PR rules

Before you push:

- [ ] No `.env`, tokens, or webhook URLs
- [ ] No closed product paths (see [docs/OPEN_VS_CLOSED.md](docs/OPEN_VS_CLOSED.md))
- [ ] No unsigned `.sys` or release binaries
- [ ] Offset changes are sourced / labeled, not guessed silently

If you accidentally committed a secret:

1. **Rotate/revoke it immediately** — git history keeps it forever
2. Contact the maintainer — we may need to filter history or force-push

---

## Responsible use

This codebase is a **research / educational base layer**.

Please do **not**:

- use it against systems or accounts you do not own or lack permission to test
- paste live credentials into issues or PRs
- add hidden phone-home, keyloggers, or undisclosed telemetry to open paths
- ship malware dressed up as a "base layer" fork

---

## Scope limits

Security reports for the **closed FRONTIER product** (menu, loader, private
releases) are still welcome — but fixes may land in the private tree, not here.

---

<div align="center">

**See something weird? Say something.**

[Open a private advisory →](https://github.com/binx-ux/external-base/security/advisories/new)

</div>
