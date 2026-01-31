#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
#
# prefix_delta_dirs.py - Stash/restore only NEW directories in PREFIX vs install_tree
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja

from __future__ import annotations

import argparse
import os
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ProjectPaths:
    project_root: Path
    install_tree: Path


def _project_paths() -> ProjectPaths:
    project_root = Path(__file__).resolve().parent.parent
    return ProjectPaths(project_root=project_root, install_tree=project_root / "resources" / "install_tree")


def _norm_relpath(rel: str) -> str:
    rel = rel.replace("\\", "/")
    rel = rel.lstrip("/")
    return rel


def _safe_stash_name(prefix: Path) -> str:
    s = str(prefix.resolve())
    s = s.replace(os.sep, "_")
    s = s.replace(":", "_")
    if s.startswith("_"):
        s = s[1:]
    return s


def _ensure_not_dangerous_dir(p: Path, *, label: str) -> None:
    resolved = p.resolve()
    if str(resolved) == os.sep:
        raise ValueError(f"Refusing to operate on {label}=/")


def _collect_dirs(root: Path) -> set[str]:
    dirs: set[str] = set()
    for dirpath, dirnames, _filenames in os.walk(root):
        base = Path(dirpath)
        dirnames.sort()
        for d in dirnames:
            rel = (base / d).relative_to(root).as_posix()
            dirs.add(_norm_relpath(rel))
    return dirs


def _is_prefix_path(parent: str, child: str) -> bool:
    parent = _norm_relpath(parent).rstrip("/")
    child = _norm_relpath(child)
    if parent == "":
        return True
    if child == parent:
        return True
    return child.startswith(parent + "/")


def _minimize_dir_roots(new_dirs: set[str]) -> list[str]:
    # Keep only top-most new directories so we don't duplicate work.
    out: list[str] = []
    for rel in sorted({_norm_relpath(d) for d in new_dirs}, key=lambda s: (s.count("/"), s)):
        if any(_is_prefix_path(parent=r, child=rel) for r in out):
            continue
        out.append(rel)
    return out


def _compute_new_dir_roots(prefix: Path, install_tree: Path) -> list[str]:
    prefix_dirs = _collect_dirs(prefix)
    base_dirs = _collect_dirs(install_tree)
    new_dirs = prefix_dirs - base_dirs
    return _minimize_dir_roots(new_dirs)


def _manifest_path(stash_dir: Path) -> Path:
    return stash_dir / "manifest.txt"


def _write_manifest(stash_dir: Path, new_dir_roots: list[str]) -> None:
    stash_dir.mkdir(parents=True, exist_ok=True)
    manifest = _manifest_path(stash_dir)
    with manifest.open("w", encoding="utf-8") as f:
        for d in new_dir_roots:
            f.write(f"{_norm_relpath(d)}\n")


def _read_manifest(stash_dir: Path) -> list[str]:
    manifest = _manifest_path(stash_dir)
    if not manifest.is_file():
        raise FileNotFoundError(f"Manifest not found: {manifest}")
    out: list[str] = []
    with manifest.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip("\n")
            if not line:
                continue
            out.append(_norm_relpath(line))
    return out


def _list_files_under(prefix: Path, dir_rel: str) -> list[str]:
    # Returns file paths (relative to prefix) under the given directory.
    root = prefix / dir_rel
    out: list[str] = []
    if not root.is_dir():
        return out

    for dirpath, _dirnames, filenames in os.walk(root):
        base = Path(dirpath)
        filenames.sort()
        for name in filenames:
            p = (base / name).relative_to(prefix).as_posix()
            out.append(_norm_relpath(p))
    return out


def _rm_tree(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink(missing_ok=True)
        return
    if path.is_dir():
        shutil.rmtree(path)


def _copy_dir(src: Path, dst: Path) -> None:
    # Copy entire directory tree.
    shutil.copytree(src, dst, dirs_exist_ok=True, copy_function=shutil.copy2)


def cmd_whatif(prefix: Path, stash_dir: Path) -> int:
    paths = _project_paths()
    if not paths.install_tree.is_dir():
        print(f"Error: install_tree not found: {paths.install_tree}", file=sys.stderr)
        return 2
    if not prefix.is_dir():
        print(f"Error: PREFIX is not a directory: {prefix}", file=sys.stderr)
        return 2

    new_dir_roots = _compute_new_dir_roots(prefix, paths.install_tree)

    print(f"PREFIX:       {prefix}")
    print(f"install_tree: {paths.install_tree}")
    print(f"stash:        {stash_dir}")
    print("")

    print("[New directories being copied]")
    if new_dir_roots:
        for d in new_dir_roots:
            print(d)
    else:
        print("(none)")

    print("")
    print("[New files being copied]")
    any_files = False
    for d in new_dir_roots:
        files = _list_files_under(prefix, d)
        for f in files:
            print(f)
            any_files = True
    if not any_files:
        print("(none)")

    return 0


def cmd_copyout(prefix: Path, stash_dir: Path, *, keep: bool) -> int:
    paths = _project_paths()
    if not paths.install_tree.is_dir():
        print(f"Error: install_tree not found: {paths.install_tree}", file=sys.stderr)
        return 2
    if not prefix.is_dir():
        print(f"Error: PREFIX is not a directory: {prefix}", file=sys.stderr)
        return 2

    _ensure_not_dangerous_dir(prefix, label="PREFIX")
    _ensure_not_dangerous_dir(stash_dir, label="stash")

    new_dir_roots = _compute_new_dir_roots(prefix, paths.install_tree)
    _write_manifest(stash_dir, new_dir_roots)

    for d in new_dir_roots:
        src = prefix / d
        dst = stash_dir / d
        if not src.is_dir():
            continue

        print(f"Copying dir: {d}")
        if dst.exists():
            _rm_tree(dst)
        dst.parent.mkdir(parents=True, exist_ok=True)
        _copy_dir(src, dst)

        if not keep:
            _rm_tree(src)

    return 0


def cmd_copyin(prefix: Path, stash_dir: Path, *, force: bool) -> int:
    if not prefix.is_dir():
        print(f"Error: PREFIX is not a directory: {prefix}", file=sys.stderr)
        return 2
    if not stash_dir.is_dir():
        print(f"Error: stash directory does not exist: {stash_dir}", file=sys.stderr)
        return 2

    _ensure_not_dangerous_dir(prefix, label="PREFIX")
    _ensure_not_dangerous_dir(stash_dir, label="stash")

    try:
        new_dir_roots = _read_manifest(stash_dir)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 2

    for d in new_dir_roots:
        src = stash_dir / d
        dst = prefix / d
        if not src.is_dir():
            continue

        if dst.exists() and not force:
            print(f"Skipping existing dir (use --force): {d}")
            continue

        print(f"Restoring dir: {d}")
        if dst.exists() and force:
            _rm_tree(dst)
        dst.parent.mkdir(parents=True, exist_ok=True)
        _copy_dir(src, dst)

    return 0


def _parse_args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(add_help=True)

    sub = p.add_subparsers(dest="cmd", required=True)

    p_whatif = sub.add_parser("whatif")
    p_whatif.add_argument("prefix")
    p_whatif.add_argument("--stash")

    p_out = sub.add_parser("copyout")
    p_out.add_argument("prefix")
    p_out.add_argument("--stash")
    p_out.add_argument("--keep", action="store_true")

    p_in = sub.add_parser("copyin")
    p_in.add_argument("prefix")
    p_in.add_argument("--stash")
    p_in.add_argument("--force", action="store_true")

    return p.parse_args(argv)


def main(argv: list[str]) -> int:
    args = _parse_args(argv)

    prefix = Path(args.prefix).expanduser()

    paths = _project_paths()
    stash_dir = (
        Path(args.stash).expanduser()
        if getattr(args, "stash", None)
        else paths.project_root / "tmp" / "prefix-dir-deltas" / _safe_stash_name(prefix)
    )

    if args.cmd == "whatif":
        return cmd_whatif(prefix, stash_dir)
    if args.cmd == "copyout":
        return cmd_copyout(prefix, stash_dir, keep=bool(getattr(args, "keep", False)))
    if args.cmd == "copyin":
        return cmd_copyin(prefix, stash_dir, force=bool(getattr(args, "force", False)))

    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
