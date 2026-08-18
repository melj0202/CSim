#!/usr/bin/env python3
"""Convenient front end for Illumo's CMake build and verification commands."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
from typing import Sequence


REPOSITORY_ROOT = Path(__file__).resolve().parent
SOURCE_DIRECTORY = REPOSITORY_ROOT / "Illumo"
DEFAULT_BUILD_DIRECTORY = Path("build")
DEFAULT_COVERAGE_DIRECTORY = Path("build-coverage")

ANSI_RESET = "\x1b[0m"
ANSI_BOLD = "\x1b[1m"
ANSI_DIM = "\x1b[2m"
ANSI_CYAN = "\x1b[38;5;45m"
ANSI_GREEN = "\x1b[38;5;82m"
ANSI_YELLOW = "\x1b[38;5;220m"
ANSI_BLUE = "\x1b[38;5;111m"
ANSI_REVERSE = "\x1b[7m"
ANSI_CLEAR = "\x1b[2J\x1b[H"
ANSI_ENTER_SCREEN = "\x1b[?1049h"
ANSI_LEAVE_SCREEN = "\x1b[?1049l"
ANSI_HIDE_CURSOR = "\x1b[?25l"
ANSI_SHOW_CURSOR = "\x1b[?25h"

DASHBOARD_CONFIGURATIONS = ("Release", "Debug", "RelWithDebInfo")
DASHBOARD_PARALLEL_OPTIONS = (
    ("Auto", 0),
    ("Off", None),
    ("2 jobs", 2),
    ("4 jobs", 4),
    ("8 jobs", 8),
    ("16 jobs", 16),
)
DASHBOARD_ITEMS = (
    ("setting", "Configuration", "configuration"),
    ("setting", "Documentation", "documentation"),
    ("setting", "Tracy profiling", "tracy"),
    ("setting", "Parallel build", "parallel"),
    ("action", "Build everything", "build"),
    ("action", "Build application", "application"),
    ("action", "Run headless tests", "test"),
    ("action", "Build and run Illumo", "run"),
    ("action", "Run existing build", "launch"),
    ("action", "Build documentation", "docs"),
    ("action", "Run LLVM coverage", "coverage"),
    ("action", "Exit", "quit"),
)
DASHBOARD_DESCRIPTIONS = {
    "build": "application, tests, and optional PDFs",
    "application": "focused Illumo target",
    "test": "all isolated CTest cases",
    "run": "build, then launch",
    "launch": "skip configure and build",
    "docs": "illumo.pdf and architecture-map.pdf",
    "coverage": "Ninja, Clang, and the 85% gate",
    "quit": "return to the shell",
}


class BuildError(RuntimeError):
    """A user-facing build orchestration failure."""

    def __init__(self, message: str, exit_code: int = 1) -> None:
        super().__init__(message)
        self.exit_code = exit_code


@dataclass
class DashboardState:
    selected: int = 0
    configuration_index: int = 0
    documentation_enabled: bool = True
    tracy_enabled: bool = False
    parallel_index: int = 0
    status: str = "Ready"
    status_kind: str = "normal"

    @property
    def configuration(self) -> str:
        return DASHBOARD_CONFIGURATIONS[self.configuration_index]

    @property
    def parallel_value(self) -> int | None:
        return DASHBOARD_PARALLEL_OPTIONS[self.parallel_index][1]


class DashboardTerminal:
    """Own the alternate screen and raw keyboard mode while the menu is open."""

    def __init__(self) -> None:
        self.input_fd: int | None = None
        self.original_attributes: object | None = None

    def enter(self) -> None:
        enable_virtual_terminal_processing()
        if os.name != "nt":
            import termios
            import tty

            self.input_fd = sys.stdin.fileno()
            self.original_attributes = termios.tcgetattr(self.input_fd)
            tty.setraw(self.input_fd)
        sys.stdout.write(ANSI_ENTER_SCREEN + ANSI_HIDE_CURSOR)
        sys.stdout.flush()

    def leave(self) -> None:
        if os.name != "nt" and self.original_attributes is not None:
            import termios

            termios.tcsetattr(
                self.input_fd, termios.TCSADRAIN, self.original_attributes
            )
            self.original_attributes = None
            self.input_fd = None
        sys.stdout.write(ANSI_RESET + ANSI_SHOW_CURSOR + ANSI_LEAVE_SCREEN)
        sys.stdout.flush()


def enable_virtual_terminal_processing() -> None:
    if os.name != "nt":
        return
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        output_handle = kernel32.GetStdHandle(-11)
        mode = ctypes.c_ulong()
        if kernel32.GetConsoleMode(output_handle, ctypes.byref(mode)):
            kernel32.SetConsoleMode(output_handle, mode.value | 0x0004)
    except (AttributeError, OSError):
        return


def dashboard_style(text: str, style: str, ansi: bool) -> str:
    if not ansi or not style:
        return text
    return f"{style}{text}{ANSI_RESET}"


def dashboard_value(state: DashboardState, key: str) -> str:
    if key == "configuration":
        return state.configuration
    if key == "documentation":
        return "On" if state.documentation_enabled else "Off"
    if key == "tracy":
        return "On" if state.tracy_enabled else "Off"
    if key == "parallel":
        return DASHBOARD_PARALLEL_OPTIONS[state.parallel_index][0]
    return ""


def render_dashboard(
    state: DashboardState, terminal_width: int, ansi: bool = True
) -> str:
    width = max(54, min(94, terminal_width - 2))
    inner_width = width - 2
    lines: list[str] = []
    encoding = sys.stdout.encoding or "utf-8"
    try:
        "╭─╮│├┤╰╯▶‹›↑↓←→".encode(encoding)
        glyphs = {
            "top_left": "╭",
            "top_right": "╮",
            "middle_left": "├",
            "middle_right": "┤",
            "bottom_left": "╰",
            "bottom_right": "╯",
            "horizontal": "─",
            "vertical": "│",
            "marker": "▶",
            "left": "‹",
            "right": "›",
            "help": "↑↓ navigate   ←→ change   Enter select   q quit",
        }
    except UnicodeEncodeError:
        glyphs = {
            "top_left": "+",
            "top_right": "+",
            "middle_left": "+",
            "middle_right": "+",
            "bottom_left": "+",
            "bottom_right": "+",
            "horizontal": "-",
            "vertical": "|",
            "marker": ">",
            "left": "<",
            "right": ">",
            "help": "Up/Down navigate   Left/Right change   Enter select   q quit",
        }

    def border(left: str, fill: str, right: str) -> None:
        lines.append(left + (fill * inner_width) + right)

    def content(
        value: str = "", style: str = "", align: str = "left"
    ) -> None:
        if len(value) > inner_width - 2:
            value = value[: inner_width - 5] + "..."
        if align == "center":
            padded = value.center(inner_width)
        else:
            padded = (" " + value).ljust(inner_width)
        lines.append(
            glyphs["vertical"]
            + dashboard_style(padded, style, ansi)
            + glyphs["vertical"]
        )

    border(glyphs["top_left"], glyphs["horizontal"], glyphs["top_right"])
    content("ILLUMO BUILD CONSOLE", ANSI_BOLD + ANSI_CYAN, "center")
    content(
        "CMake orchestration without the ceremony",
        ANSI_DIM,
        "center",
    )
    border(
        glyphs["middle_left"],
        glyphs["horizontal"],
        glyphs["middle_right"],
    )
    content("Settings", ANSI_BOLD + ANSI_BLUE)

    for index, (kind, label, key) in enumerate(DASHBOARD_ITEMS):
        if index == 4:
            border(
                glyphs["middle_left"],
                glyphs["horizontal"],
                glyphs["middle_right"],
            )
            content("Actions", ANSI_BOLD + ANSI_BLUE)

        marker = glyphs["marker"] if index == state.selected else " "
        if kind == "setting":
            value = dashboard_value(state, key)
            available = max(
                1, inner_width - len(label) - len(value) - 8
            )
            raw = (
                f" {marker} {label}{' ' * available}"
                f"{glyphs['left']} {value} {glyphs['right']} "
            )
        else:
            description = DASHBOARD_DESCRIPTIONS[key]
            if inner_width >= 76:
                available = max(1, inner_width - len(label) - len(description) - 7)
                raw = f" {marker} {label}{' ' * available}{description} "
            else:
                raw = f" {marker} {label} "
        raw = raw[:inner_width].ljust(inner_width)
        style = ANSI_REVERSE if index == state.selected else ""
        lines.append(
            glyphs["vertical"]
            + dashboard_style(raw, style, ansi)
            + glyphs["vertical"]
        )

    border(
        glyphs["middle_left"],
        glyphs["horizontal"],
        glyphs["middle_right"],
    )
    content(glyphs["help"], ANSI_DIM)
    status_style = ""
    if state.status_kind == "success":
        status_style = ANSI_GREEN
    elif state.status_kind == "failure":
        status_style = ANSI_YELLOW
    content(f"Status: {state.status}", status_style)
    border(
        glyphs["bottom_left"],
        glyphs["horizontal"],
        glyphs["bottom_right"],
    )
    return "\n".join(lines)


def read_dashboard_key() -> str:
    if os.name == "nt":
        import msvcrt

        character = msvcrt.getwch()
        if character in ("\x00", "\xe0"):
            return {
                "H": "up",
                "P": "down",
                "K": "left",
                "M": "right",
            }.get(msvcrt.getwch(), "unknown")
        if character == "\x03":
            raise KeyboardInterrupt
        return {
            "\r": "enter",
            "q": "quit",
            "Q": "quit",
            "j": "down",
            "k": "up",
            "h": "left",
            "l": "right",
        }.get(character, "unknown")

    character = sys.stdin.read(1)
    if character == "\x03":
        raise KeyboardInterrupt
    if character == "\x1b":
        import select

        sequence = ""
        while len(sequence) < 2 and select.select([sys.stdin], [], [], 0.03)[0]:
            sequence += sys.stdin.read(1)
        return {
            "[A": "up",
            "[B": "down",
            "[C": "right",
            "[D": "left",
        }.get(sequence, "quit")
    return {
        "\r": "enter",
        "\n": "enter",
        "q": "quit",
        "Q": "quit",
        "j": "down",
        "k": "up",
        "h": "left",
        "l": "right",
    }.get(character, "unknown")


def adjust_dashboard_setting(state: DashboardState, direction: int) -> None:
    key = DASHBOARD_ITEMS[state.selected][2]
    if key == "configuration":
        state.configuration_index = (
            state.configuration_index + direction
        ) % len(DASHBOARD_CONFIGURATIONS)
    elif key == "documentation":
        state.documentation_enabled = not state.documentation_enabled
    elif key == "tracy":
        state.tracy_enabled = not state.tracy_enabled
    elif key == "parallel":
        state.parallel_index = (state.parallel_index + direction) % len(
            DASHBOARD_PARALLEL_OPTIONS
        )


def dashboard_parallel_arguments(state: DashboardState) -> list[str]:
    parallel = state.parallel_value
    if parallel is None:
        return []
    if parallel == 0:
        return ["--parallel"]
    return ["--parallel", str(parallel)]


def dashboard_common_arguments(state: DashboardState) -> list[str]:
    arguments = ["--config", state.configuration]
    if not state.documentation_enabled:
        arguments.append("--no-docs")
    if state.tracy_enabled:
        arguments.append("--tracy")
    arguments.extend(dashboard_parallel_arguments(state))
    return arguments


def dashboard_action_arguments(
    state: DashboardState, action: str
) -> list[str]:
    if action == "build":
        return ["build", *dashboard_common_arguments(state)]
    if action == "application":
        return [
            "build",
            *dashboard_common_arguments(state),
            "--target",
            "Illumo",
        ]
    if action == "test":
        return ["test", *dashboard_common_arguments(state)]
    if action == "run":
        return ["run", *dashboard_common_arguments(state)]
    if action == "launch":
        return ["run", "--config", state.configuration, "--no-build"]
    if action == "docs":
        return ["docs"]
    if action == "coverage":
        return ["coverage", *dashboard_parallel_arguments(state)]
    raise BuildError(f"Unknown dashboard action: {action}")


def execute_dashboard_action(
    state: DashboardState,
    action: str,
    terminal: DashboardTerminal,
) -> None:
    arguments = dashboard_action_arguments(state, action)
    command = [sys.executable, str(Path(__file__).resolve()), *arguments]
    terminal.leave()
    print(f"\n=== {DASHBOARD_ITEMS[state.selected][1]} ===\n")
    print(f"> {format_command(command)}\n", flush=True)
    try:
        result = subprocess.run(command, cwd=REPOSITORY_ROOT, check=False)
        if result.returncode == 0:
            state.status = f"{DASHBOARD_ITEMS[state.selected][1]} succeeded"
            state.status_kind = "success"
        else:
            state.status = (
                f"{DASHBOARD_ITEMS[state.selected][1]} failed "
                f"(exit {result.returncode})"
            )
            state.status_kind = "failure"
    except OSError as error:
        state.status = f"Could not start action: {error}"
        state.status_kind = "failure"
    try:
        input("\nPress Enter to return to the build console...")
    except EOFError:
        pass
    terminal.enter()


def run_dashboard() -> int:
    if not sys.stdin.isatty() or not sys.stdout.isatty():
        print(
            "error: the interactive build console requires a terminal; "
            "use an explicit build.py subcommand instead.",
            file=sys.stderr,
        )
        return 2

    state = DashboardState()
    terminal = DashboardTerminal()
    terminal.enter()
    try:
        while True:
            terminal_width = shutil.get_terminal_size((96, 30)).columns
            sys.stdout.write(
                ANSI_CLEAR + render_dashboard(state, terminal_width)
            )
            sys.stdout.flush()
            key = read_dashboard_key()
            if key == "quit":
                return 0
            if key == "up":
                state.selected = (state.selected - 1) % len(DASHBOARD_ITEMS)
            elif key == "down":
                state.selected = (state.selected + 1) % len(DASHBOARD_ITEMS)
            elif key in ("left", "right"):
                if DASHBOARD_ITEMS[state.selected][0] == "setting":
                    adjust_dashboard_setting(
                        state, -1 if key == "left" else 1
                    )
            elif key == "enter":
                kind, _label, action = DASHBOARD_ITEMS[state.selected]
                if kind == "setting":
                    adjust_dashboard_setting(state, 1)
                elif action == "quit":
                    return 0
                else:
                    execute_dashboard_action(state, action, terminal)
    finally:
        terminal.leave()


class CommandRunner:
    """Print and execute external commands from a predictable directory."""

    def __init__(self, dry_run: bool) -> None:
        self.dry_run = dry_run

    def run(
        self,
        command: Sequence[str],
        working_directory: Path = REPOSITORY_ROOT,
    ) -> None:
        print(f"> {format_command(command)}", flush=True)
        if self.dry_run:
            return

        result = subprocess.run(
            list(command),
            cwd=working_directory,
            check=False,
        )
        if result.returncode != 0:
            raise BuildError(
                f"Command failed with exit code {result.returncode}: "
                f"{format_command(command)}",
                result.returncode,
            )


def format_command(command: Sequence[str]) -> str:
    if os.name == "nt":
        return subprocess.list2cmdline(list(command))
    return shlex.join(command)


def existing_tool(name: str, dry_run: bool) -> str:
    path = shutil.which(name)
    if path is not None:
        return path
    if dry_run:
        return name
    raise BuildError(
        f"Required tool '{name}' was not found on PATH. "
        "Install it or open a developer shell that provides it."
    )


def resolve_build_directory(value: Path) -> Path:
    if value.is_absolute():
        return value.resolve()
    return (REPOSITORY_ROOT / value).resolve()


def add_common_build_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--config",
        choices=("Debug", "Release", "RelWithDebInfo", "MinSizeRel"),
        default="Release",
        help="CMake build configuration (default: Release)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=DEFAULT_BUILD_DIRECTORY,
        metavar="PATH",
        help="build tree, relative to the repository root by default",
    )
    parser.add_argument("--generator", help="CMake generator passed with -G")
    parser.add_argument(
        "--architecture", help="generator architecture passed with -A"
    )
    parser.add_argument(
        "--parallel",
        nargs="?",
        const=0,
        type=positive_job_count,
        metavar="JOBS",
        help="build in parallel, optionally with a job limit",
    )
    parser.add_argument(
        "--tracy",
        action="store_true",
        help="enable Tracy instrumentation with ILLUMO_ENABLE_TRACY",
    )
    parser.add_argument(
        "--no-docs",
        action="store_true",
        help="disable the optional IllumoDocs target for this build tree",
    )
    parser.add_argument(
        "--cmake-arg",
        action="append",
        default=[],
        metavar="ARG",
        help="extra configure argument; repeat and use --cmake-arg=-DNAME=VALUE",
    )
    parser.add_argument(
        "--verbose", action="store_true", help="request verbose build output"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print commands without executing them",
    )


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Configure, build, test, run, and measure Illumo through its "
            "existing CMake targets. Running without a command opens the "
            "interactive build console in a terminal."
        )
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    common = argparse.ArgumentParser(add_help=False)
    add_common_build_arguments(common)

    menu_parser = subparsers.add_parser(
        "menu", help="open the interactive terminal build console"
    )
    menu_parser.add_argument(
        "--snapshot",
        action="store_true",
        help=argparse.SUPPRESS,
    )

    subparsers.add_parser(
        "configure", parents=[common], help="configure the selected build tree"
    )
    subparsers.add_parser(
        "build",
        parents=[common],
        help="configure and build the selected configuration",
    ).add_argument(
        "--target", help="build a focused CMake target instead of the default"
    )

    test_parser = subparsers.add_parser(
        "test", parents=[common], help="build and run the headless tests"
    )
    test_mode = test_parser.add_mutually_exclusive_group()
    test_mode.add_argument(
        "--test", metavar="NAME", help="run one exact IllumoTests case"
    )
    test_mode.add_argument(
        "--list-tests",
        action="store_true",
        help="list exact IllumoTests case names",
    )

    run_parser = subparsers.add_parser(
        "run", parents=[common], help="build and launch Illumo"
    )
    run_parser.add_argument(
        "--no-build",
        action="store_true",
        help="launch the existing executable without configuring or building",
    )
    run_parser.add_argument(
        "app_arguments",
        nargs=argparse.REMAINDER,
        help="arguments after -- are passed to Illumo",
    )

    coverage_parser = subparsers.add_parser(
        "coverage", help="configure and run the Clang/LLVM coverage target"
    )
    coverage_parser.add_argument(
        "--build-dir",
        type=Path,
        default=DEFAULT_COVERAGE_DIRECTORY,
        metavar="PATH",
        help="coverage build tree (default: build-coverage)",
    )
    coverage_parser.add_argument(
        "--parallel",
        nargs="?",
        const=0,
        type=positive_job_count,
        metavar="JOBS",
        help="build in parallel, optionally with a job limit",
    )
    coverage_parser.add_argument(
        "--cmake-arg",
        action="append",
        default=[],
        metavar="ARG",
        help="extra configure argument; repeat and use --cmake-arg=-DNAME=VALUE",
    )
    coverage_parser.add_argument(
        "--verbose", action="store_true", help="request verbose build output"
    )
    coverage_parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print commands without executing them",
    )

    docs_parser = subparsers.add_parser(
        "docs", help="build the two documentation PDFs through docs/build.ps1"
    )
    docs_parser.add_argument(
        "--dry-run",
        action="store_true",
        help="print commands without executing them",
    )
    return parser


def normalize_arguments(arguments: Sequence[str]) -> list[str]:
    if not arguments:
        return ["build"]
    if arguments[0] in ("-h", "--help"):
        return list(arguments)
    if arguments[0].startswith("-"):
        return ["build", *arguments]
    return list(arguments)


def positive_job_count(value: str) -> int:
    try:
        count = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("job count must be an integer") from error
    if count < 1:
        raise argparse.ArgumentTypeError("job count must be at least 1")
    return count


def configure_command(arguments: argparse.Namespace, cmake: str) -> list[str]:
    build_directory = resolve_build_directory(arguments.build_dir)
    command = [
        cmake,
        "-S",
        str(SOURCE_DIRECTORY),
        "-B",
        str(build_directory),
    ]
    if arguments.generator:
        command.extend(("-G", arguments.generator))
    if arguments.architecture:
        command.extend(("-A", arguments.architecture))

    docs_enabled = "OFF" if arguments.no_docs else "ON"
    tracy_enabled = "ON" if arguments.tracy else "OFF"
    command.extend(
        (
            f"-DCMAKE_BUILD_TYPE={arguments.config}",
            f"-DILLUMO_BUILD_DOCUMENTATION={docs_enabled}",
            f"-DILLUMO_ENABLE_TRACY={tracy_enabled}",
            "-DILLUMO_ENABLE_COVERAGE=OFF",
        )
    )
    command.extend(arguments.cmake_arg)
    return command


def build_command(
    arguments: argparse.Namespace,
    cmake: str,
    target: str | None = None,
) -> list[str]:
    build_directory = resolve_build_directory(arguments.build_dir)
    command = [
        cmake,
        "--build",
        str(build_directory),
        "--config",
        arguments.config,
    ]
    selected_target = (
        target if target is not None else getattr(arguments, "target", None)
    )
    if selected_target:
        command.extend(("--target", selected_target))
    if arguments.parallel is not None:
        command.append("--parallel")
        if arguments.parallel > 0:
            command.append(str(arguments.parallel))
    if arguments.verbose:
        command.append("--verbose")
    return command


def configure(arguments: argparse.Namespace, runner: CommandRunner) -> str:
    cmake = existing_tool("cmake", runner.dry_run)
    runner.run(configure_command(arguments, cmake))
    return cmake


def executable_path(
    build_directory: Path,
    configuration: str,
    name: str,
    dry_run: bool,
) -> Path:
    suffix = ".exe" if os.name == "nt" else ""
    candidates = (
        build_directory / configuration / f"{name}{suffix}",
        build_directory / f"{name}{suffix}",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    if dry_run:
        return candidates[0]
    rendered = " or ".join(str(candidate) for candidate in candidates)
    raise BuildError(f"Expected executable was not produced at {rendered}")


def run_configure(arguments: argparse.Namespace) -> None:
    runner = CommandRunner(arguments.dry_run)
    configure(arguments, runner)


def run_build(arguments: argparse.Namespace) -> None:
    runner = CommandRunner(arguments.dry_run)
    cmake = configure(arguments, runner)
    runner.run(build_command(arguments, cmake))


def run_tests(arguments: argparse.Namespace) -> None:
    runner = CommandRunner(arguments.dry_run)
    cmake = configure(arguments, runner)
    runner.run(build_command(arguments, cmake, "IllumoTests"))

    build_directory = resolve_build_directory(arguments.build_dir)
    if arguments.test or arguments.list_tests:
        test_binary = executable_path(
            build_directory,
            arguments.config,
            "IllumoTests",
            runner.dry_run,
        )
        test_command = [str(test_binary)]
        if arguments.list_tests:
            test_command.append("--list")
        else:
            test_command.extend(("--run", arguments.test))
        runner.run(test_command, test_binary.parent)
        return

    ctest = existing_tool("ctest", runner.dry_run)
    runner.run(
        (
            ctest,
            "--test-dir",
            str(build_directory),
            "-C",
            arguments.config,
            "-L",
            "Illumo",
            "--output-on-failure",
        )
    )


def run_application(arguments: argparse.Namespace) -> None:
    runner = CommandRunner(arguments.dry_run)
    if not arguments.no_build:
        cmake = configure(arguments, runner)
        runner.run(build_command(arguments, cmake, "Illumo"))

    build_directory = resolve_build_directory(arguments.build_dir)
    application = executable_path(
        build_directory,
        arguments.config,
        "Illumo",
        runner.dry_run,
    )
    app_arguments = list(arguments.app_arguments)
    if app_arguments and app_arguments[0] == "--":
        app_arguments.pop(0)
    runner.run((str(application), *app_arguments), application.parent)


def run_coverage(arguments: argparse.Namespace) -> None:
    runner = CommandRunner(arguments.dry_run)
    cmake = existing_tool("cmake", runner.dry_run)
    build_directory = resolve_build_directory(arguments.build_dir)
    configure_coverage = [
        cmake,
        "-S",
        str(SOURCE_DIRECTORY),
        "-B",
        str(build_directory),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        "-DILLUMO_BUILD_DOCUMENTATION=OFF",
        "-DILLUMO_ENABLE_TRACY=OFF",
        "-DILLUMO_ENABLE_COVERAGE=ON",
    ]
    configure_coverage.extend(arguments.cmake_arg)
    runner.run(configure_coverage)

    build_coverage = [
        cmake,
        "--build",
        str(build_directory),
        "--target",
        "IllumoCoverage",
    ]
    if arguments.parallel is not None:
        build_coverage.append("--parallel")
        if arguments.parallel > 0:
            build_coverage.append(str(arguments.parallel))
    if arguments.verbose:
        build_coverage.append("--verbose")
    runner.run(build_coverage)


def run_docs(arguments: argparse.Namespace) -> None:
    runner = CommandRunner(arguments.dry_run)
    powershell = shutil.which("pwsh") or shutil.which("powershell")
    if powershell is None:
        if runner.dry_run:
            powershell = "powershell"
        else:
            raise BuildError(
                "PowerShell was not found on PATH; docs/build.ps1 requires it."
            )
    runner.run(
        (
            powershell,
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(REPOSITORY_ROOT / "docs" / "build.ps1"),
        )
    )


def main(arguments: Sequence[str] | None = None) -> int:
    parser = create_parser()
    command_line = sys.argv[1:] if arguments is None else list(arguments)
    if not command_line:
        if sys.stdin.isatty() and sys.stdout.isatty():
            try:
                return run_dashboard()
            except KeyboardInterrupt:
                print("\nBuild console closed.", file=sys.stderr)
                return 130
        command_line = ["build"]
    parsed = parser.parse_args(normalize_arguments(command_line))

    if parsed.command == "menu":
        if parsed.snapshot:
            print(render_dashboard(DashboardState(), 96, ansi=False))
            return 0
        try:
            return run_dashboard()
        except KeyboardInterrupt:
            print("\nBuild console closed.", file=sys.stderr)
            return 130

    actions = {
        "configure": run_configure,
        "build": run_build,
        "test": run_tests,
        "run": run_application,
        "coverage": run_coverage,
        "docs": run_docs,
    }
    try:
        actions[parsed.command](parsed)
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return error.exit_code
    except KeyboardInterrupt:
        print("\nBuild interrupted.", file=sys.stderr)
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
