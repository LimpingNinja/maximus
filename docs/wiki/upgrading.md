---
layout: default
title: "Upgrading"
section: "getting-started"
description: "How to upgrade Maximus BBS to a new release"
---

Upgrading between Maximus NG releases is straightforward — your configuration,
user database, and message bases are never touched by an upgrade. You're
essentially just dropping in new binaries, updating display files if you
haven't customized them, and optionally refreshing the language delta. The
whole process takes a few minutes.

If you're coming from legacy Maximus 3.0 (with `.ctl` and `.mad` files), that's
a migration, not an upgrade — see
[Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) instead.

---

## What to Update

When a new release is available, download and extract it alongside your existing
installation. Then copy the updated pieces into your live BBS:

### Always Update

| Directory | Why |
|-----------|-----|
| `bin/` | Executables — bug fixes, new features, security patches |
| `lib/` | Shared libraries — must match the binaries |

These are always safe to overwrite. They contain no user data or configuration.

```bash
cp -r maximus-NEW/bin/* /path/to/your/bbs/bin/
cp -r maximus-NEW/lib/* /path/to/your/bbs/lib/
```

### Update Unless Customized

| Directory | Why | Skip if... |
|-----------|-----|------------|
| `display/` | Help screens, menu files, compiled `.bbs` | You customized `.mec` source |
| `scripts/*.vm` | Compiled MEX scripts | You customized `.mex` source |
| `scripts/include/` | MEX include headers | You added custom includes |

If you have customized display files or MEX scripts, check the release notes
for changes to the stock files and merge by hand.

### Always Keep

| Directory | Contains |
|-----------|----------|
| `config/` | Your TOML configuration — BBS settings, areas, menus, access |
| `config/lang/english.toml` | Your language strings (prompts, messages) |
| `data/` | User database, message bases, file areas |

**Never overwrite these** with files from a new release — they contain your
site-specific data.

---

## Configuration: New TOML Keys

When new configuration keys are added in a release, the BBS writes **sane
defaults** for any missing keys at startup. Your existing configuration
continues to work without changes.

To take advantage of new features, read the release notes and
[CHANGES.md](https://github.com/LimpingNinja/maximus/blob/main/CHANGES.md)
for new keys and their purpose. You can then add them to your TOML files at
your convenience.

There is no automated delta or merge system for configuration TOML files — the
runtime default mechanism handles forward compatibility.

---

## Language Files: Delta Upgrades

Language strings (`english.toml`) are upgraded via the
[delta overlay system]({{ site.baseurl }}{% link legacy-delta-overlays.md %}). Each release ships
an updated `delta_english.toml` that may contain:

- New parameter metadata (names, types, descriptions for the language editor)
- Fixed conversion artifacts
- New or updated lightbar prompt strings
- Semantic theme color refinements

### Upgrade Steps

1. **Replace the delta file** from the new release:

   ```bash
   cp maximus-NEW/config/lang/delta_english.toml \
      /path/to/your/bbs/config/lang/delta_english.toml
   ```

2. **Re-apply the delta** to your existing language file:

   ```bash
   # If you have customized colors — keeps your colors, merges metadata
   bin/maxcfg --apply-delta config/lang/english.toml --merge-only

   # If you use stock colors — applies everything
   bin/maxcfg --apply-delta config/lang/english.toml
   ```

Your language customizations are preserved. The delta only touches metadata
and (optionally) theme color mappings.

See [Delta Overlays]({{ site.baseurl }}{% link legacy-delta-overlays.md %}) for details on
the tier system and merge modes.

---

## Quick Reference

```bash
# 1. Download and extract the new release
tar -xzvf maximus-NEW.tar.gz

# 2. Update binaries and libraries (always)
cp -r maximus-NEW/bin/* /path/to/your/bbs/bin/
cp -r maximus-NEW/lib/* /path/to/your/bbs/lib/

# 3. Update display and scripts (if not customized)
cp -r maximus-NEW/display/* /path/to/your/bbs/display/
cp -r maximus-NEW/scripts/* /path/to/your/bbs/scripts/

# 4. Update and re-apply the language delta
cp maximus-NEW/config/lang/delta_english.toml \
   /path/to/your/bbs/config/lang/delta_english.toml
cd /path/to/your/bbs
bin/maxcfg --apply-delta config/lang/english.toml --merge-only

# 5. Restart
bin/maxtel -p 2323 -n 4
```

---

## Coming from Maximus 3.0?

If you are upgrading from a classic Maximus 3.0 installation (with `.ctl`
configuration files and `.mad` language files), this is not a version-to-version
upgrade — it is a migration to a new configuration system.

See [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) for the full guide
covering [CTL → TOML]({{ site.baseurl }}{% link legacy-ctl-to-toml.md %}) configuration export
and [MAD → TOML]({{ site.baseurl }}{% link legacy-convert-mad.md %}) language conversion.

---

## See Also

- [Quick Start]({{ site.baseurl }}{% link quick-start.md %}) — fresh install from a release
  package
- [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) — migrating from
  Maximus 3.0
- [Delta Overlays]({{ site.baseurl }}{% link legacy-delta-overlays.md %}) — the delta overlay
  system for language file upgrades
- [Updates & Changelog]({{ site.baseurl }}{% link updates.md %}) — release notes and change
  history
- [Building Maximus]({{ site.baseurl }}{% link building.md %}) — building from source
