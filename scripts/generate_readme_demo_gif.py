#!/usr/bin/env python3
from __future__ import annotations

import argparse
import gzip
import io
import struct
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BINARY = ROOT / "build" / "sample" / "example_completion"
DEFAULT_OUTPUT = ROOT / "docs" / "repository" / "assets" / "argparse-c-demo.gif"
FONT_CANDIDATES = (
    Path("/usr/share/consolefonts/Lat7-VGA16.psf.gz"),
    Path("/usr/share/consolefonts/Uni1-VGA16.psf.gz"),
    Path("/usr/share/consolefonts/Lat2-Terminus16.psf.gz"),
)

WIDTH = 960
HEIGHT = 540
MARGIN_X = 34
MARGIN_Y = 28
TITLE_BAR_HEIGHT = 34
PADDING_X = 22
PADDING_Y = 18
FONT_SCALE = 1
MAX_HELP_LINES = 18
MAX_COMPLETION_LINES = 8
MAX_ERROR_LINES = 6
FRAME_DELAY_TYPING_CS = 5
FRAME_DELAY_STEP_CS = 10
FRAME_DELAY_PAUSE_CS = 85

PALETTE = (
    (17, 24, 39),
    (30, 41, 59),
    (65, 90, 119),
    (134, 239, 172),
    (237, 242, 247),
    (148, 163, 184),
    (248, 113, 113),
    (250, 204, 21),
)

COLOR_BG = 0
COLOR_PANEL = 1
COLOR_TITLE = 2
COLOR_PROMPT = 3
COLOR_TEXT = 4
COLOR_MUTED = 5
COLOR_CURSOR = 6
COLOR_ACCENT = 7


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate the README terminal animation GIF from the sample binary."
    )
    parser.add_argument(
        "--binary",
        type=Path,
        default=DEFAULT_BINARY,
        help=f"path to example_completion binary (default: {DEFAULT_BINARY})",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"output GIF path (default: {DEFAULT_OUTPUT})",
    )
    return parser.parse_args()


def run_command(binary: Path, *args: str, expect_success: bool = True) -> list[str]:
    completed = subprocess.run(
        [str(binary), *args],
        text=True,
        capture_output=True,
        cwd=ROOT,
    )
    if expect_success and completed.returncode != 0:
        raise subprocess.CalledProcessError(
            completed.returncode,
            completed.args,
            output=completed.stdout,
            stderr=completed.stderr,
        )

    output = completed.stdout if completed.returncode == 0 else completed.stderr
    return output.rstrip("\n").splitlines()


def load_psf_font(path: Path) -> tuple[int, int, list[bytes]]:
    if path.suffix == ".gz":
        raw = gzip.decompress(path.read_bytes())
    else:
        raw = path.read_bytes()

    if raw[:2] == b"\x36\x04":
        mode = raw[2]
        char_size = raw[3]
        glyph_count = 512 if (mode & 0x01) else 256
        width = 8
        height = char_size
        data = raw[4 : 4 + glyph_count * char_size]
        glyphs = [
            data[index * char_size : (index + 1) * char_size]
            for index in range(glyph_count)
        ]
        return width, height, glyphs

    if raw[:4] == b"\x72\xb5\x4a\x86":
        _, _, header_size, _, glyph_count, char_size, height, width = struct.unpack(
            "<IIIIIIII", raw[:32]
        )
        glyph_data = raw[header_size : header_size + glyph_count * char_size]
        glyphs = [
            glyph_data[index * char_size : (index + 1) * char_size]
            for index in range(glyph_count)
        ]
        return width, height, glyphs

    raise ValueError(f"unsupported PSF font format: {path}")


def find_font() -> Path:
    for candidate in FONT_CANDIDATES:
        if candidate.exists():
            return candidate
    raise FileNotFoundError("no supported console font found in /usr/share/consolefonts")


def build_lines(binary: Path) -> list[tuple[str, list[str]]]:
    help_lines = run_command(binary, "--help")[:MAX_HELP_LINES]
    completion_lines = run_command(
        binary, "__complete", "--shell", "bash", "--", "sample/example_completion.c", "yaml", "b"
    )[:MAX_COMPLETION_LINES]
    error_lines = run_command(
        binary,
        "sample/example_completion.c",
        "yml",
        "build",
        expect_success=False,
    )[:MAX_ERROR_LINES]

    return [
        (
            "./build/sample/example_completion --help",
            help_lines,
        ),
        (
            "./build/sample/example_completion __complete --shell bash -- sample/example_completion.c yaml b",
            completion_lines,
        ),
        (
            "./build/sample/example_completion sample/example_completion.c yml build",
            error_lines,
        ),
    ]


def fill_rect(
    pixels: bytearray, x: int, y: int, width: int, height: int, color: int
) -> None:
    x0 = max(0, x)
    y0 = max(0, y)
    x1 = min(WIDTH, x + width)
    y1 = min(HEIGHT, y + height)
    for row in range(y0, y1):
        start = row * WIDTH + x0
        end = row * WIDTH + x1
        pixels[start:end] = bytes([color]) * (end - start)


def draw_glyph(
    pixels: bytearray,
    glyphs: list[bytes],
    font_width: int,
    font_height: int,
    x: int,
    y: int,
    ch: str,
    color: int,
) -> None:
    code = ord(ch)
    if code >= len(glyphs):
        code = ord("?")
    glyph = glyphs[code]
    bytes_per_row = max(1, (font_width + 7) // 8)

    for row in range(font_height):
        for col in range(font_width):
            byte_value = glyph[row * bytes_per_row + (col // 8)]
            bit_mask = 0x80 >> (col % 8)
            if byte_value & bit_mask:
                px = x + col * FONT_SCALE
                py = y + row * FONT_SCALE
                fill_rect(pixels, px, py, FONT_SCALE, FONT_SCALE, color)


def draw_text(
    pixels: bytearray,
    glyphs: list[bytes],
    font_width: int,
    font_height: int,
    x: int,
    y: int,
    text: str,
    color: int,
) -> None:
    cursor_x = x
    advance = font_width * FONT_SCALE
    for ch in text:
        draw_glyph(pixels, glyphs, font_width, font_height, cursor_x, y, ch, color)
        cursor_x += advance


def draw_window_frame(pixels: bytearray) -> tuple[int, int, int, int]:
    fill_rect(pixels, 0, 0, WIDTH, HEIGHT, COLOR_BG)
    panel_x = MARGIN_X
    panel_y = MARGIN_Y
    panel_w = WIDTH - MARGIN_X * 2
    panel_h = HEIGHT - MARGIN_Y * 2
    fill_rect(pixels, panel_x, panel_y, panel_w, panel_h, COLOR_PANEL)
    fill_rect(pixels, panel_x, panel_y, panel_w, TITLE_BAR_HEIGHT, COLOR_TITLE)

    for offset, color in ((14, 6), (34, 7), (54, 3)):
        fill_rect(pixels, panel_x + offset, panel_y + 10, 10, 10, color)

    return (
        panel_x + PADDING_X,
        panel_y + TITLE_BAR_HEIGHT + PADDING_Y,
        panel_w - PADDING_X * 2,
        panel_h - TITLE_BAR_HEIGHT - PADDING_Y * 2,
    )


def stylized_segments(line: str) -> list[tuple[str, int]]:
    if line.startswith("# "):
        return [(line, COLOR_MUTED)]
    if line.startswith("$ "):
        return [("$ ", COLOR_PROMPT), (line[2:], COLOR_TEXT)]
    if line.startswith("error:"):
        return [(line, COLOR_CURSOR)]
    if line.startswith("usage:"):
        return [(line, COLOR_MUTED)]
    if (
        line.startswith("fast")
        or line.startswith("build")
        or line.startswith("bench")
        or line.startswith("bundle")
    ):
        return [(line, COLOR_ACCENT)]
    return [(line, COLOR_TEXT)]


def render_lines(
    glyphs: list[bytes],
    font_width: int,
    font_height: int,
    lines: list[str],
    show_cursor: bool,
) -> bytes:
    pixels = bytearray([COLOR_BG]) * (WIDTH * HEIGHT)
    text_x, text_y, text_w, text_h = draw_window_frame(pixels)
    char_w = font_width * FONT_SCALE
    line_h = font_height * FONT_SCALE + 2
    max_cols = max(1, text_w // char_w)
    max_rows = max(1, text_h // line_h)

    visible = [line[:max_cols] for line in lines[-max_rows:]]
    for row, line in enumerate(visible):
        y = text_y + row * line_h
        cursor_x = text_x
        for segment, color in stylized_segments(line):
            draw_text(pixels, glyphs, font_width, font_height, cursor_x, y, segment, color)
            cursor_x += len(segment) * char_w

    if show_cursor and visible:
        cursor_line = visible[-1]
        cursor_x = text_x + min(len(cursor_line), max_cols - 1) * char_w
        cursor_y = text_y + (len(visible) - 1) * line_h
        fill_rect(
            pixels,
            cursor_x,
            cursor_y + font_height * FONT_SCALE + 1,
            char_w - 1,
            2,
            COLOR_CURSOR,
        )

    return bytes(pixels)


def build_frames(
    glyphs: list[bytes], font_width: int, font_height: int, scenes: list[tuple[str, list[str]]]
) -> tuple[list[bytes], list[int]]:
    frames: list[bytes] = []
    delays: list[int] = []
    lines = [
        "# argparse-c: one parser definition powers discoverable CLI UX",
        "# captured from sample/example_completion.c",
        "",
    ]

    for command, output in scenes:
        typed = "$ "
        for ch in command:
            typed += ch
            frames.append(render_lines(glyphs, font_width, font_height, lines + [typed], True))
            delays.append(FRAME_DELAY_TYPING_CS)
        lines.append(typed)
        frames.append(render_lines(glyphs, font_width, font_height, lines, False))
        delays.append(FRAME_DELAY_STEP_CS)

        for line in output:
            lines.append(line)
            frames.append(render_lines(glyphs, font_width, font_height, lines, False))
            delays.append(FRAME_DELAY_STEP_CS)

        lines.append("")
        frames.append(render_lines(glyphs, font_width, font_height, lines, False))
        delays.append(FRAME_DELAY_PAUSE_CS)

    return frames, delays


def encode_lzw(indices: bytes, min_code_size: int) -> bytes:
    clear_code = 1 << min_code_size
    end_code = clear_code + 1
    next_code = end_code + 1
    dictionary = {bytes([value]): value for value in range(clear_code)}
    codes = [clear_code]

    if indices:
        current = bytes([indices[0]])
        for value in indices[1:]:
            candidate = current + bytes([value])
            if candidate in dictionary:
                current = candidate
                continue

            codes.append(dictionary[current])
            if next_code < 4096:
                dictionary[candidate] = next_code
                next_code += 1
            else:
                codes.append(clear_code)
                dictionary = {bytes([item]): item for item in range(clear_code)}
                next_code = end_code + 1
            current = bytes([value])

        codes.append(dictionary[current])

    codes.append(end_code)
    out = pack_codes(codes, min_code_size)
    return bytes(out)


def pack_codes(codes: list[int], min_code_size: int) -> bytes:
    clear_code = 1 << min_code_size
    end_code = clear_code + 1
    code_size = min_code_size + 1
    next_code = end_code + 1
    bit_buffer = 0
    bit_count = 0
    out = bytearray()
    prev_was_value = False

    for code in codes:
        bit_buffer |= code << bit_count
        bit_count += code_size
        while bit_count >= 8:
            out.append(bit_buffer & 0xFF)
            bit_buffer >>= 8
            bit_count -= 8

        if code == clear_code:
            code_size = min_code_size + 1
            next_code = end_code + 1
            prev_was_value = False
            continue
        if code == end_code:
            prev_was_value = False
            continue

        if prev_was_value:
            if next_code < 4096:
                next_code += 1
                if next_code == (1 << code_size) and code_size < 12:
                    code_size += 1
        else:
            prev_was_value = True

    if bit_count:
        out.append(bit_buffer & 0xFF)
    return bytes(out)


def gif_sub_blocks(data: bytes) -> bytes:
    out = bytearray()
    for index in range(0, len(data), 255):
        chunk = data[index : index + 255]
        out.append(len(chunk))
        out.extend(chunk)
    out.append(0)
    return bytes(out)


def encode_gif(frames: list[bytes], delays: list[int]) -> bytes:
    if not frames:
        raise ValueError("no frames to encode")

    min_code_size = 3
    packed_fields = 0x80 | 0x70 | (min_code_size - 1)
    output = io.BytesIO()
    output.write(b"GIF89a")
    output.write(struct.pack("<HHBBB", WIDTH, HEIGHT, packed_fields, 0, 0))
    for red, green, blue in PALETTE:
        output.write(bytes((red, green, blue)))

    output.write(b"!\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00")

    for frame, delay in zip(frames, delays):
        output.write(b"!\xF9\x04\x04")
        output.write(struct.pack("<H", delay))
        output.write(b"\x00\x00")
        output.write(b",")
        output.write(struct.pack("<HHHHB", 0, 0, WIDTH, HEIGHT, 0))
        output.write(bytes((min_code_size,)))
        output.write(gif_sub_blocks(encode_lzw(frame, min_code_size)))

    output.write(b";")
    return output.getvalue()


def main() -> int:
    args = parse_args()
    binary = args.binary.resolve()
    output = args.output.resolve()

    if not binary.exists():
        raise SystemExit(
            f"sample binary not found: {binary}\nrun `cmake --build build --target example_completion` first."
        )

    font_path = find_font()
    font_width, font_height, glyphs = load_psf_font(font_path)
    scenes = build_lines(binary)
    frames, delays = build_frames(glyphs, font_width, font_height, scenes)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(encode_gif(frames, delays))
    print(f"wrote {output.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
