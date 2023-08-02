#!/usr/bin/env python

import argparse
import collections
import contextlib
import logging
import os
from os.path import exists
from pathlib import Path
import platform
import re
import subprocess
import sys
import tempfile
import textwrap
import typing
import zipfile

logger = logging.getLogger(__name__)


VcArchDevel = collections.namedtuple("VcArchDevel", ("dll", "imp", "test"))


class Executer:
    def __init__(self, root: Path, dry: bool=False):
        self.root = root
        self.dry = dry

    def run(self, cmd, stdout=False, dry_out=None, force=False):
        sys.stdout.flush()
        logger.info("Executing args=%r", cmd)
        if self.dry:
            if stdout:
                return subprocess.run(["echo", dry_out or ""], stdout=subprocess.PIPE if stdout else None, text=True, cwd=self.root)
        else:
            return subprocess.run(cmd, stdout=subprocess.PIPE if stdout else None, text=True)


class SectionPrinter:
    @contextlib.contextmanager
    def group(self, title: str):
        print(f"{title}:")
        yield


class GitHubSectionPrinter(SectionPrinter):
    def __init__(self):
        super().__init__()
        self.in_group = False

    @contextlib.contextmanager
    def group(self, title: str):
        print(f"::group::{title}")
        assert not self.in_group, "Can enter a group only once"
        self.in_group = True
        yield
        self.in_group = False
        print("::endgroup::")


class VisualStudio:
    def __init__(self, executer: Executer, year: typing.Optional[str]=None):
        self.executer = executer
        self.vsdevcmd = self.find_vsdevcmd(year)
        self.msbuild = self.find_msbuild()

    @property
    def dry(self):
        return self.executer.dry

    VS_YEAR_TO_VERSION = {
        "2022": 17,
        "2019": 16,
        "2017": 15,
        "2015": 14,
        "2013": 12,
    }

    def find_vsdevcmd(self, year: typing.Optional[str]=None) -> typing.Optional[Path]:
        vswhere_spec = ["-latest"]
        if year is not None:
            try:
                version = cls.VS_YEAR_TO_VERSION[year]
            except KeyError:
                logger.error("Invalid Visual Studio year")
                return None
            vswhere_spec.extend(["-version", f"[{version},{version+1})"])
        vswhere_cmd = ["vswhere"] + vswhere_spec + ["-property", "installationPath"]
        vs_install_path = Path(self.executer.run(vswhere_cmd, stdout=True, dry_out="/tmp").stdout.strip())
        logger.info("VS install_path = %s", vs_install_path)
        assert vs_install_path.is_dir(), "VS installation path does not exist"
        vsdevcmd_path = vs_install_path / "Common7/Tools/vsdevcmd.bat"
        logger.info("vsdevcmd path = %s", vsdevcmd_path)
        if self.dry:
            vsdevcmd_path.parent.mkdir(parents=True, exist_ok=True)
            vsdevcmd_path.touch(exist_ok=True)
        assert vsdevcmd_path.is_file(), "vsdevcmd.bat batch file does not exist"
        return vsdevcmd_path

    def find_msbuild(self) -> typing.Optional[Path]:
        vswhere_cmd = ["vswhere", "-latest", "-requires", "Microsoft.Component.MSBuild", "-find", "MSBuild\**\Bin\MSBuild.exe"]
        msbuild_path = Path(self.executer.run(vswhere_cmd, stdout=True, dry_out="/tmp/MSBuild.exe").stdout.strip())
        logger.info("MSBuild path = %s", msbuild_path)
        if self.dry:
            msbuild_path.parent.mkdir(parents=True, exist_ok=True)
            msbuild_path.touch(exist_ok=True)
        assert msbuild_path.is_file(), "MSBuild.exe does not exist"
        return msbuild_path

    def build(self, arch: str, platform: str, configuration: str, projects: list[Path]):
        assert projects, "Need at least one project to build"

        vsdev_cmd_str = f"\"{self.vsdevcmd}\" -arch={arch}"
        msbuild_cmd_str = " && ".join([f"\"{self.msbuild}\" \"{project}\" /m /p:BuildInParallel=true /p:Platform={platform} /p:Configuration={configuration}" for project in projects])
        bat_contents = f"{vsdev_cmd_str} && {msbuild_cmd_str}\n"
        bat_path = Path(tempfile.gettempdir()) / "cmd.bat"
        with bat_path.open("w") as f:
            f.write(bat_contents)

        logger.info("Running cmd.exe script (%s): %s", bat_path, bat_contents)
        cmd = ["cmd.exe", "/D", "/E:ON", "/V:OFF", "/S", "/C", f"CALL {str(bat_path)}"]
        self.executer.run(cmd)


class Releaser:
    def __init__(self, project: str, commit: str, root: Path, dist_path: Path, section_printer: SectionPrinter, executer=Executer):
        self.project = project
        self.version = self.extract_sdl_version(root=root, project=project)
        self.root = root
        self.commit = commit
        self.dist_path = dist_path
        self.section_printer = section_printer
        self.executer = executer

        self.artifacts = {}

    @property
    def dry(self):
        return self.executer.dry

    def prepare(self):
        logger.debug("Creating dist folder")
        self.dist_path.mkdir(parents=True, exist_ok=True)

    def _git_archive(self, out: Path):
        out.unlink(missing_ok=True)
        self.executer.run(["git", "archive", self.commit, "--prefix", f"{self.project}-{self.version}/", ":!.git*", "-o", out], force=True)
        if self.dry:
            out.touch(exist_ok=True)
        assert out.is_file(), "Source archive has not been created"

    def create_source_archives(self):
        archive_base = f"{self.project}-{self.version}"

        zip = self.dist_path / f"{archive_base}.zip"
        logger.debug("Creating zip source archive (%s)...", zip)
        self._git_archive(out=zip)
        self.artifacts["src-zip"] = zip

        tar_gz = self.dist_path / f"{archive_base}.tar.gz"
        logger.debug("Creating tar.gz source archive (%s)...", tar_gz)
        self._git_archive(out=tar_gz)
        self.artifacts["src-tar-gz"] = tar_gz

    def build_vs(self, arch: str, platform: str, vs: VisualStudio, configuration: str="Release"):
        dll_path = self.root / f"VisualC/SDL/{platform}/{configuration}/{self.project}.dll"
        imp_path = self.root / f"VisualC/SDL/{platform}/{configuration}/{self.project}.lib"
        test_path = self.root / f"VisualC/SDL_test/{platform}/{configuration}/{self.project}_test.lib"

        dll_path.unlink(missing_ok=True)
        imp_path.unlink(missing_ok=True)
        test_path.unlink(missing_ok=True)

        projects = [
            self.root / "VisualC/SDL/SDL.vcxproj",
            self.root / "VisualC/SDL_test/SDL_test.vcxproj",
        ]

        vs.build(arch=arch, platform=platform, configuration=configuration, projects=projects)

        if self.dry:
            dll_path.parent.mkdir(parents=True, exist_ok=True)
            dll_path.touch()
            imp_path.touch()
            test_path.parent.mkdir(parents=True, exist_ok=True)
            test_path.touch()

        assert dll_path.is_file(), "SDL3.dll has not been created"
        assert imp_path.is_file(), "SDL3.lib has not been created"
        assert test_path.is_file(), "SDL3_test.lib has not been created"

        zip_path = self.dist_path / f"{self.project}-{self.version}-win32-{arch}.zip"
        zip_path.unlink(missing_ok=True)
        logger.info("Creating %s", zip_path)
        with zipfile.ZipFile(zip_path, mode="w", compression=zipfile.ZIP_DEFLATED) as zf:
            logger.debug("Adding %s", dll_path.name)
            zf.write(dll_path, arcname=dll_path.name)
            logger.debug("Adding %s", "README-SDL.txt")
            zf.write(self.root / "README-SDL.txt", arcname="README-SDL.txt")
        self.artifacts[f"VC-{arch}"] = zip_path

        return VcArchDevel(dll=dll_path, imp=imp_path, test=test_path)

    def build_vs_devel(self, arch_vc: dict[str, VcArchDevel]):
        zip_path = self.dist_path / f"{self.project}-devel-{self.version}-VC.zip"
        archive_prefix = f"{self.project}-{self.version}"

        def zip_file(zf: zipfile.ZipFile, path: Path, arcrelpath: str):
            arcname = f"{archive_prefix}/{arcrelpath}"
            logger.debug("Adding %s to %s", path, arcname)
            zf.write(path, arcname=arcname)

        def zip_directory(zf: zipfile.ZipFile, directory: Path, arcrelpath: str):
            for f in directory.iterdir():
                if f.is_file():
                    arcname = f"{archive_prefix}/{arcrelpath}/{f.name}"
                    logger.debug("Adding %s to %s", f, arcname)
                    zf.write(f, arcname=arcname)

        with zipfile.ZipFile(zip_path, mode="w", compression=zipfile.ZIP_DEFLATED) as zf:
            for arch, binaries in arch_vc.items():
                zip_file(zf, path=binaries.dll, arcrelpath=f"lib/{arch}/{binaries.dll.name}")
                zip_file(zf, path=binaries.imp, arcrelpath=f"lib/{arch}/{binaries.imp.name}")
                zip_file(zf, path=binaries.test, arcrelpath=f"lib/{arch}/{binaries.test.name}")

            zip_directory(zf, directory=self.root / "include/SDL3", arcrelpath="include/SDL3")
            zip_directory(zf, directory=self.root / "docs", arcrelpath="docs")
            zip_directory(zf, directory=self.root / "VisualC/pkg-support/cmake", arcrelpath="cmake")

            for txt in ("BUGS.txt", "README-SDL.txt", "WhatsNew.txt"):
                zip_file(zf, path=self.root / txt, arcrelpath=txt)
            zip_file(zf, path=self.root / "LICENSE.txt", arcrelpath="COPYING.txt")
            zip_file(zf, path=self.root / "README.md", arcrelpath="README.txt")
        self.artifacts["VC-devel"] = zip_path

    @classmethod
    def extract_sdl_version(cls, root: Path, project: str):
        with open(root / f"include/{project}/SDL_version.h", "r") as f:
            text = f.read()
        major = next(re.finditer(r"^#define SDL_MAJOR_VERSION\s+([0-9]+)$", text, flags=re.M)).group(1)
        minor = next(re.finditer(r"^#define SDL_MINOR_VERSION\s+([0-9]+)$", text, flags=re.M)).group(1)
        patch = next(re.finditer(r"^#define SDL_PATCHLEVEL\s+([0-9]+)$", text, flags=re.M)).group(1)
        return f"{major}.{minor}.{patch}"


def main(argv=None):
    parser = argparse.ArgumentParser(allow_abbrev=False, description="Create SDL release artifacts")
    parser.add_argument("--root", metavar="DIR", type=Path, default=Path(__file__).resolve().parents[1], help="Root of SDL")
    parser.add_argument("--out", "-o", metavar="DIR", dest="dist_path", type=Path, default="dist", help="Output directory")
    parser.add_argument("--github", action="store_true", help="Script is running on a GitHub runner")
    parser.add_argument("--commit", default="HEAD", help="Git commit/tag of which a release should be created")
    parser.add_argument("--project", required=True, help="Name of the project")
    parser.add_argument("--create", choices=["source", "win32", "xcframework"], required=True,action="append", dest="actions", help="SDL version")
    parser.set_defaults(loglevel=logging.INFO)
    parser.add_argument('--vs-year', dest="vs_year", help="Visual Studio year")
    parser.add_argument('--debug', action='store_const', const=logging.DEBUG, dest="loglevel", help="Print script debug information")
    parser.add_argument('--dry-run', action='store_true', dest="dry", help="Don't execute anythin")

    args = parser.parse_args(argv)
    logging.basicConfig(level=args.loglevel, format='[%(levelname)s] %(message)s')
    args.actions = set(args.actions)
    args.dist_path = args.dist_path.resolve()
    args.root = args.root.resolve()
    args.dist_path = args.dist_path.resolve()
    if args.dry:
        args.dist_path = args.dist_path / "dry"

    if args.github:
        section_printer = GitHubSectionPrinter()
    else:
        section_printer = SectionPrinter()

    executer = Executer(root=args.root, dry=args.dry)

    releaser = Releaser(project=args.project, commit=args.commit, root=args.root, dist_path=args.dist_path, executer=executer, section_printer=section_printer)

    with section_printer.group("Arguments"):
        print(f"project = {args.project}")
        print(f"version = {releaser.version}")
        print(f"commit  = {args.commit}")
        print(f"out     = {args.dist_path}")
        print(f"actions = {args.actions}")
        print(f"dry     = {args.dry}")
        print(f"version = {args.dry}")

    releaser.prepare()

    if "source" in args.actions:
        with section_printer.group("Create source archives"):
            releaser.create_source_archives()

    if "xcframework" in args.actions:
        if platform.system() != "Darwin" and not args.dry:
            parser.error("xcframework artifact(s) can only be built on Darwin")
        raise NotImplementedError

    if "win32" in args.actions:
        if platform.system() != "Windows" and not args.dry:
            parser.error("win32 artifact(s) can only be built on Windows")
        with section_printer.group("Find Visual Studio"):
            vs = VisualStudio(executer=executer)
        with section_printer.group("Build x86 VS binary"):
            x86 = releaser.build_vs(arch="x86", platform="Win32", vs=vs)
        with section_printer.group("Build x64 VS binary"):
            x64 = releaser.build_vs(arch="x64", platform="x64", vs=vs)
        with section_printer.group("Create SDL VC development zip"):
            arch_vc = {
                "x86": x86,
                "x64": x64,
            }
            releaser.build_vs_devel(arch_vc)

    with section_printer.group("Summary"):
        print(f"artifacts = {releaser.artifacts}")

    if args.github:
        if args.dry:
            os.environ["GITHUB_OUTPUT"] = "/tmp/github_output.txt"
        with open(os.environ["GITHUB_OUTPUT"], "a") as f:
            f.write(f"project={releaser.project}\n")
            f.write(f"version={releaser.version}\n")
            for k, v in releaser.artifacts.items():
                f.write(f"{k}={v.name}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
