---
layout: default
title: "CTL to TOML"
section: "configuration"
description: "Converting legacy CTL configuration files to TOML"
---

This page covers converting your legacy Maximus 3.0 `.ctl` configuration files
to the TOML format used by Maximus NG.

---

## The Legacy Pipeline

In classic Maximus, configuration lived in a tree of `.ctl` text files rooted
at `max.ctl`. The SILT compiler parsed this tree and produced binary `.prm` and
`.dat` files that the BBS loaded at runtime:

```
max.ctl ──SILT──► max.prm   (binary config blob)
                ├─► area.dat  (area definitions)
                ├─► access.dat (access levels)
                └─► ...
```

You edited `.ctl` files by hand (or through `maxcfg`), then ran SILT to
recompile before the changes took effect.

## What Replaced It

Maximus NG loads TOML files directly at runtime. There is no compile step, no
binary blobs. Edit TOML, restart the BBS, and the changes are live.

The entire SILT pipeline — including `max.prm`, `area.dat`, and all compiled
outputs — is retired.

---

## Running the Export

The `maxcfg` tool parses your legacy CTL file tree and emits TOML:

```bash
bin/maxcfg --export-nextgen /path/to/max.ctl
```

By default, TOML files are written to the `config/` directory relative to your
system path. To write to a different location:

```bash
bin/maxcfg --export-nextgen /path/to/max.ctl --export-dir /tmp/ng-config
```

The export is **one-way** — CTL in, TOML out. Your original `.ctl` files are
not modified or deleted.

See [maxcfg CLI]({{ site.baseurl }}{% link maxcfg-cli.md %}) for the full flag reference.

---

## What Gets Converted

The exporter walks the full CTL include tree starting from `max.ctl`. Each
major configuration section becomes its own TOML file:

| CTL Section | TOML Output | Contents |
|-------------|-------------|----------|
| System settings | `config/system.toml` | Paths, system name, node count, logging |
| Area definitions | `config/areas/*.toml` | Message and file area configurations |
| Access levels | `config/access.toml` | Privilege levels and flag definitions |
| Menu definitions | `config/menus/*.toml` | Menu trees, hotkeys, option types |
| Colour definitions | `config/colours.toml` | Legacy colour table mappings |
| Protocol definitions | `config/protocols.toml` | Transfer protocol configuration |
| Event scheduling | `config/events.toml` | Timed event definitions |

The exact output structure may vary depending on your CTL layout. The exporter
preserves all configuration values — it does not silently drop settings.

---

## After Export

Once the TOML files are generated:

1. **Review the output.** Open the generated files in any text editor. Settings
   are now human-readable key-value pairs.

2. **Check path references.** The exporter converts paths as-is from the CTL
   files. If your legacy install used different directory names (e.g., `etc/`
   vs. `config/`, `m/` vs. `scripts/`), you may need to update path values to
   match the Maximus NG
   [directory structure]({{ site.baseurl }}{% link directory-structure.md %}).

3. **Verify access levels.** Custom privilege levels and flag definitions should
   come through intact, but confirm the numeric mappings match your
   expectations.

4. **Test.** Run `bin/max config/maximus -c` in local console mode to verify
   the BBS starts correctly with the new configuration.

---

## Edge Cases

- **Nested includes.** The exporter follows `#include` directives in CTL files
  just like SILT did. All included files must be accessible at their referenced
  paths.

- **Macro definitions.** `#define` values in CTL files are resolved during
  export. The resulting TOML contains the expanded values, not the macro names.

- **Relative paths.** If your CTL files use relative paths, they are resolved
  relative to the location of `max.ctl`. Verify that the resulting TOML paths
  are correct for your NG installation.

- **Custom keywords.** Any CTL keywords that the exporter does not recognize are
  logged as warnings. These typically indicate third-party patches or
  site-specific extensions that will need manual TOML entries.

---

## See Also

- [Legacy Migration]({{ site.baseurl }}{% link legacy-migration.md %}) — migration overview and
  the MAD → TOML language conversion path
- [Convert Legacy MAD]({{ site.baseurl }}{% link legacy-convert-mad.md %}) — converting language
  files
- [Delta Overlays]({{ site.baseurl }}{% link legacy-delta-overlays.md %}) — the delta system for
  language file upgrades
- [maxcfg CLI]({{ site.baseurl }}{% link maxcfg-cli.md %}) — full command-line reference for
  `--export-nextgen` and other flags
- [Directory Structure]({{ site.baseurl }}{% link directory-structure.md %}) — where files live
  after installation
