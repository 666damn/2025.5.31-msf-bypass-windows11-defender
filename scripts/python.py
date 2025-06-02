# encrypt.py
key = 0x5C

with open("shell.bin", "rb") as f:
    data = f.read()

enc = bytes([b ^ key for b in data])

with open("enc_shellcode.h", "w") as f:
    f.write("unsigned char shellcode[] = {\n")
    for i, b in enumerate(enc):
        f.write(f"0x{b:02x},")
        if (i + 1) % 16 == 0:
            f.write("\n")
    f.write("\n};\n")
    f.write(f"unsigned int shellcode_len = {len(enc)};\n")
