#!/usr/bin/env python3
import sys

pi1_path = sys.argv[1]
header_path = sys.argv[2]

with open(pi1_path, 'rb') as f:
    data = f.read()

palette_data = data[2:34]
screen_data = data[34:]

with open(header_path, 'w') as f:
    f.write("const unsigned short title_palette[16] = {\n")
    for i in range(0, 32, 2):
        color = (palette_data[i] << 8) | palette_data[i+1]
        f.write("    0x%04X" % color)
        if i < 30:
            f.write(",")
        f.write("\n")
    f.write("};\n\n")
    
    f.write("const unsigned char title_screen[32000] = {\n")
    for i in range(0, 32000, 16):
        f.write("    ")
        for j in range(16):
            if i + j < 32000:
                f.write("0x%02X" % screen_data[i+j])
                if i + j < 31999:
                    f.write(",")
        f.write("\n")
    f.write("};\n")

print("Done!")

