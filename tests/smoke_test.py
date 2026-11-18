#!/usr/bin/env python3
"""Smoke test for ASCII QUEST.

Runs the game inside a pseudo terminal, drives its menus with real
keyboard input and checks that the important screens are rendered.
No third-party modules are required.
"""

import fcntl
import os
import pty
import select
import struct
import subprocess
import sys
import termios
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BINARY = os.path.join(ROOT, "rpg")

failures = []


class Screen:
    """Just enough of an ANSI terminal for this game's output."""

    def __init__(self, cols=100, rows=30):
        self.cols = cols
        self.rows = rows
        self.reset()
        self.pending = ""

    def reset(self):
        self.grid = [[" "] * self.cols for _ in range(self.rows)]
        self.row = 0
        self.col = 0

    def feed(self, text):
        text = self.pending + text
        self.pending = ""
        i = 0
        while i < len(text):
            ch = text[i]
            if ch == "\x1b":
                j = i + 1
                if j < len(text) and text[j] == "[":
                    j += 1
                    while j < len(text) and not ("@" <= text[j] <= "~"):
                        j += 1
                    if j >= len(text):
                        self.pending = text[i:]
                        return
                    self.csi(text[i + 2:j], text[j])
                    i = j + 1
                    continue
                if j >= len(text):
                    self.pending = text[i:]
                    return
                i = j + 1
                continue
            if ch == "\r":
                self.col = 0
            elif ch == "\n":
                self.row = min(self.rows - 1, self.row + 1)
            elif ch == "\b":
                self.col = max(0, self.col - 1)
            elif ch >= " ":
                if 0 <= self.row < self.rows and 0 <= self.col < self.cols:
                    self.grid[self.row][self.col] = ch
                self.col += 1
            i += 1

    def csi(self, params, final):
        if final == "H":
            parts = params.split(";") if params else []
            try:
                r = int(parts[0]) - 1 if parts and parts[0] else 0
                c = int(parts[1]) - 1 if len(parts) > 1 and parts[1] else 0
            except ValueError:
                r, c = 0, 0
            self.row = max(0, min(self.rows - 1, r))
            self.col = max(0, min(self.cols - 1, c))
        elif final == "J":
            for r in range(self.row, self.rows):
                start = self.col if r == self.row else 0
                for c in range(start, self.cols):
                    self.grid[r][c] = " "
        elif final == "K":
            for c in range(self.col, self.cols):
                self.grid[self.row][c] = " "
        # SGR and mode changes do not matter for text checks.

    def text(self):
        return "\n".join("".join(row).rstrip() for row in self.grid)


class Session:
    def __init__(self, args, cols=100, rows=30):
        self.screen = Screen(cols, rows)
        self.master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ,
                    struct.pack("HHHH", rows, cols, 0, 0))
        self.proc = subprocess.Popen(
            [BINARY] + args,
            stdin=slave, stdout=slave, stderr=slave,
            close_fds=True, start_new_session=True,
        )
        os.close(slave)
        os.set_blocking(self.master, False)
        self.raw = b""

    def read_some(self, timeout=1.0):
        end = time.time() + timeout
        got = False
        while time.time() < end:
            r, _, _ = select.select([self.master], [], [], 0.05)
            if not r:
                continue
            try:
                chunk = os.read(self.master, 65536)
            except OSError:
                return got
            if not chunk:
                return got
            got = True
            self.raw += chunk
            self.screen.feed(chunk.decode("utf-8", "replace"))
        return got

    def screen_text(self):
        return self.screen.text()

    def send(self, data):
        os.write(self.master, data.encode())

    def wait_for(self, needle, timeout=3.0):
        end = time.time() + timeout
        while time.time() < end:
            if needle in self.screen_text():
                return True
            if not self.read_some(0.15):
                break
        return needle in self.screen_text()

    def settle(self, seconds=0.25):
        self.read_some(seconds)

    def wait_gone(self, needle, timeout=3.0):
        end = time.time() + timeout
        while time.time() < end:
            if needle not in self.screen_text():
                return True
            if not self.read_some(0.15):
                break
        return needle not in self.screen_text()

    def close(self):
        try:
            os.close(self.master)
        except OSError:
            pass
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait()


def check(name, condition, detail=""):
    if condition:
        print("PASS  %s" % name)
    else:
        print("FAIL  %s %s" % (name, detail))
        failures.append(name)


def main():
    if not os.path.exists(BINARY):
        print("FAIL  binary not found: %s (run make first)" % BINARY)
        return 1

    # The deliverable must really be a 32-bit x86 ELF executable.
    with open(BINARY, "rb") as fh:
        ident = fh.read(20)
    check("ELF magic", ident[:4] == b"\x7fELF")
    check("ELF class 32-bit", ident[4] == 1)
    check("ELF machine i386", int.from_bytes(ident[18:20], "little") == 3)

    # --help must work without a terminal.
    help_out = subprocess.run([BINARY, "--help"], capture_output=True,
                              text=True)
    check("--help exits 0", help_out.returncode == 0)
    check("--help mentions options",
          "--seed" in help_out.stdout and "--floor" in help_out.stdout)

    bad = subprocess.run([BINARY, "--bogus"], capture_output=True, text=True)
    check("unknown option exits 2", bad.returncode == 2)

    s = Session(["--seed", "42"])
    try:
        check("title screen", s.wait_for("Begin your descent"))
        check("title art", "Crypt of the ASCII King" in s.screen_text())

        s.send("\r")
        check("map screen", s.wait_for("HERO"))
        check("status panel",
              all(t in s.screen_text() for t in
                  ("LEGEND", "Floor 1: The Mossy Halls", "ATK")))
        check("player glyph", "@" in s.screen_text())
        check("map borders", "#" in s.screen_text())

        before = s.screen_text()
        s.send("d")
        s.settle(0.3)
        check("movement redraws", s.screen_text() != before)
        s.send("s")
        s.settle(0.3)

        s.send("i")
        check("backpack opens", s.wait_for("BACKPACK"))
        check("backpack sections",
              all(t in s.screen_text() for t in
                  ("EQUIPMENT", "SUPPLIES", "CHARM", "Rusty Dagger", "Potion",
                   "Bomb", "Elixir")))
        s.send("2")
        s.settle(0.2)
        check("locked weapon refused", "do not own" in s.screen_text())
        s.send("b")
        s.settle(0.2)
        check("bomb is battle-only", "only useful in a fight" in s.screen_text())
        s.send("i")
        check("backpack returns to map", s.wait_for("HERO"))

        s.send("?")
        check("help screen", s.wait_for("HOW TO PLAY"))
        check("help lists controls", "Move" in s.screen_text() or
              "move" in s.screen_text())
        s.send("x")
        check("help returns to map", s.wait_for("LEGEND"))
        check("help overlay is gone", "HOW TO PLAY" not in s.screen_text())

        s.send("q")
        check("quit confirmation", s.wait_for("Quit the crypt?"))
        s.send("n")
        check("cancel quit", s.wait_gone("Quit the crypt?"))
        check("quit prompt is gone", "WASD/arrows" in s.screen_text())

        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
        s.read_some(0.3)
        check("clean exit", s.proc.returncode == 0)
        check("terminal restored", b"\x1b[?1049l" in s.raw)
        check("cursor restored", b"\x1b[?25h" in s.raw)
    finally:
        s.close()

    # Too-small terminals must be rejected with a clear message.
    small = Session([], cols=60, rows=20)
    try:
        small.settle(0.5)
        check("small terminal rejected", "at least 70x24" in small.screen_text())
    finally:
        small.close()

    # Every floor must load, render its header and exit cleanly.
    floor_names = [
        "The Mossy Halls", "The Sunken Cells", "The Bone Gallery",
        "The Ember Depths", "The Obsidian Vault", "The Ashen Gate",
        "Frozen Crypt", "Whisper Warrens", "Clockwork Sanctum",
        "The Molten Forge", "The Starless Span", "The Crimson Gate",
        "The Dragon's Maw",
    ]
    for number, name in enumerate(floor_names, 1):
        s = Session(["--seed", "7", "--floor", str(number)])
        try:
            s.send("\r")
            check("floor %d loads" % number,
                  s.wait_for("Floor %d: %s" % (number, name)))
            check("floor %d header" % number,
                  ("Floor %d/13" % number) in s.screen_text())
            s.send("q")
            s.wait_for("Quit the crypt?")
            s.send("y")
            s.proc.wait(timeout=3)
        finally:
            s.close()

    # Dev shortcut: start directly on the dragon floor.
    s = Session(["--seed", "7", "--floor", "13"])
    try:
        s.send("\r")
        check("final floor starts",
              s.wait_for("Floor 13: The Dragon's Maw"))
        check("final floor header", "Floor 13/13" in s.screen_text())
        check("dragon floor has art or glyph",
              "D" in s.screen_text() or "@" in s.screen_text())
        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
        check("final floor clean exit", s.proc.returncode == 0)
    finally:
        s.close()

    # Random mode must generate a playable floor and name it.
    s = Session(["--seed", "7", "--random"])
    try:
        s.wait_for("Begin your descent")
        s.send("\r")
        check("random floor loads", s.wait_for("Shifting Halls"))
        check("random floor header", "Floor 1/13" in s.screen_text())
        check("random floor has player", "@" in s.screen_text())
        s.send("d")
        s.settle(0.3)
        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
        check("random clean exit", s.proc.returncode == 0)
    finally:
        s.close()

    # Save with F5, then load the same file into a fresh process.
    save_path = "/tmp/aq_smoke_save.bin"
    if os.path.exists(save_path):
        os.remove(save_path)
    s = Session(["--seed", "7", "--save", save_path])
    try:
        s.wait_for("Begin your descent")
        s.send("\r")
        s.wait_for("HERO")
        s.send("d")
        s.settle(0.3)
        s.send("\x1b[15~")
        check("F5 saves", s.wait_for("Saved to"))
        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
    finally:
        s.close()

    s = Session(["--load", save_path])
    try:
        check("load notice", s.wait_for("Loaded"))
        check("load restores floor", "Floor 1: The Mossy Halls" in s.screen_text())
        check("load restores turn", "Turns 1" in s.screen_text())
        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
    finally:
        s.close()

    # F9 reloads the F5 snapshot in the same process.
    s = Session(["--seed", "7", "--save", save_path])
    try:
        s.wait_for("Begin your descent")
        s.send("\r")
        s.wait_for("HERO")
        s.send("d")
        s.settle(0.3)
        s.send("\x1b[15~")
        s.wait_for("Saved to")
        s.send("d")
        s.settle(0.3)
        s.send("\x1b[20~")
        check("F9 loads", s.wait_for("Loaded"))
        check("F9 restores turn", "Turns 1" in s.screen_text())
        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
    finally:
        s.close()

    # The Hermit stands two tiles up and nine right of the floor-2 start.
    s = Session(["--seed", "7", "--floor", "2"])
    try:
        s.wait_for("Begin your descent")
        s.send("\r")
        s.wait_for("HERO")
        s.send("ww" + "d" * 9)
        s.settle(0.4)
        if "BATTLE" in s.screen_text():
            # A skeleton patrols the corridor; cut it down and keep walking.
            s.send("a" * 40)
            s.settle(1.0)
            s.send("d" * 12)
            s.settle(0.6)
        check("NPC dialogue opens", s.wait_for("The Hermit"))
        check("NPC quest available", "Quest available" in s.screen_text())
        s.send("\r")
        s.settle(0.3)
        check("NPC quest offer", "Five of the crypt" in s.screen_text())
        s.send("1")
        s.settle(0.3)
        check("NPC quest accepted", s.wait_for("0/5"))
        s.send("0")
        s.settle(0.3)
        s.send("q")
        s.wait_for("Quit the crypt?")
        s.send("y")
        s.proc.wait(timeout=3)
    finally:
        s.close()

    if failures:
        print("\n%d check(s) failed." % len(failures))
        return 1
    print("\nAll checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
