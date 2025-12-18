from __future__ import annotations

import argparse
import logging
import sys
from pathlib import Path

from .builder import build_extension
from .manifest import load_manifest
from .scaffold import init_extension


logging.basicConfig(level=logging.INFO, format="[%(levelname)s] %(message)s")
logger = logging.getLogger("fusionpy.cli")


def cmd_init(args: argparse.Namespace) -> None:
    init_extension(args.name, Path(args.path))
    logger.info("Scaffold created at %s", Path(args.path).resolve())


def cmd_validate(args: argparse.Namespace) -> None:
    manifest = load_manifest(args.manifest)
    logger.info("Manifest valid for '%s' v%s (entrypoint: %s)", manifest.name, manifest.version, manifest.entrypoint)
    logger.info("Capabilities: %s", ", ".join(manifest.capabilities) or "none")
    logger.info("Actions: %d | Conditions: %d | Expressions: %d", len(manifest.actions), len(manifest.conditions), len(manifest.expressions))


def cmd_build(args: argparse.Namespace) -> None:
    archive = build_extension(Path(args.manifest), output_dir=Path(args.output), target=args.target, mode=args.mode)
    logger.info("Build complete: %s", archive)


def cmd_run(args: argparse.Namespace) -> None:
    fusion_path = Path(args.fusion_path)
    if not fusion_path.exists():
        logger.error("Fusion executable not found at %s", fusion_path)
        sys.exit(1)
    logger.info("Would launch Fusion at %s with hot-reload enabled: %s", fusion_path, args.hot_reload)
    logger.info("Integration with the host bridge is required for actual injection.")


def cmd_publish(args: argparse.Namespace) -> None:
    manifest = load_manifest(args.manifest)
    logger.info("Publishing '%s' version %s", manifest.name, manifest.version)
    logger.info("Feed: %s | Channel: %s", args.feed or "local", args.channel)
    logger.info("Package signing and upload to feeds are intentionally stubbed in this prototype.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Fusion Python Extension SDK CLI")
    sub = parser.add_subparsers(dest="command", required=True)

    p_init = sub.add_parser("init", help="Create a new extension scaffold")
    p_init.add_argument("name", help="Extension name, e.g. MyExtension")
    p_init.add_argument("--path", default=".", help="Target directory for the scaffold")
    p_init.set_defaults(func=cmd_init)

    p_validate = sub.add_parser("validate", help="Validate a fusion_extension manifest")
    p_validate.add_argument("manifest", help="Path to fusion_extension.yml or .json")
    p_validate.set_defaults(func=cmd_validate)

    p_build = sub.add_parser("build", help="Build the extension package")
    p_build.add_argument("manifest", help="Path to fusion_extension.yml or .json")
    p_build.add_argument("--output", default="fusionpy_build", help="Output directory for build artifacts")
    p_build.add_argument("--target", default="win32", help="Build target (win32 by default)")
    p_build.add_argument("--mode", default="debug", choices=["debug", "release"], help="Build profile")
    p_build.set_defaults(func=cmd_build)

    p_run = sub.add_parser("run", help="Launch Fusion with the extension injected")
    p_run.add_argument("--fusion-path", required=True, help="Path to Fusion executable")
    p_run.add_argument("--hot-reload", action="store_true", help="Enable hot reload when safe")
    p_run.set_defaults(func=cmd_run)

    p_publish = sub.add_parser("publish", help="Publish the built extension to a feed")
    p_publish.add_argument("manifest", help="Path to fusion_extension.yml or .json")
    p_publish.add_argument("--feed", help="Target feed URL or filesystem path")
    p_publish.add_argument("--channel", default="stable", help="Release channel (stable/beta/dev)")
    p_publish.set_defaults(func=cmd_publish)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
