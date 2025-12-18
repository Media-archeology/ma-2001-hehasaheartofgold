import sys
import os
from PIL import Image

def rgb_to_st(r, g, b):
    return ((r >> 5) << 8) | ((g >> 5) << 4) | (b >> 5)

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 png2st_bg.py background.png background.h")
        return
    img = Image.open(sys.argv[1]).convert("RGB")
    w, h = img.size
    pixels = list(img.getdata())
    raw_palette = []
    for p in pixels:
        if p not in raw_palette: raw_palette.append(p)
    final_palette = [(0,0,0)] * 16
    final_palette[0] = (0,0,0)      # 0번 검정 고정
    final_palette[15] = (255,255,255) # 15번 흰색 고정
    fill_idx = 1
    for p in raw_palette:
        if fill_idx >= 15: break
        if p == (0,0,0) or p == (255,255,255): continue
        final_palette[fill_idx] = p
        fill_idx += 1
    data = bytearray()
    for y in range(200):
        for x_block in range(0, 320, 16):
            p0=p1=p2=p3=0
            for i in range(16):
                px = pixels[y * 320 + x_block + i]
                try: idx = final_palette.index(px)
                except ValueError: idx = 0
                bit = 15 - i
                if idx & 1: p0 |= (1 << bit)
                if idx & 2: p1 |= (1 << bit)
                if idx & 4: p2 |= (1 << bit)
                if idx & 8: p3 |= (1 << bit)
            data += bytes([(p0>>8)&0xFF, p0&0xFF, (p1>>8)&0xFF, p1&0xFF, (p2>>8)&0xFF, p2&0xFF, (p3>>8)&0xFF, p3&0xFF])
    var_name = os.path.splitext(os.path.basename(sys.argv[2]))[0]
    with open(sys.argv[2], "w") as f:
        f.write(f"#ifndef {var_name.upper()}_H\n#define {var_name.upper()}_H\n\n")
        f.write(f"static const unsigned short {var_name}_palette[16] = {{\n")
        for i, p in enumerate(final_palette):
            f.write(f"  0x{rgb_to_st(*p):04X}{',' if i < 15 else ''}")
            if (i+1)%4==0: f.write("\n")
        f.write("};\n\n")
        f.write(f"static const unsigned char {var_name}_data[32000] = {{\n")
        for i in range(0, len(data), 16):
            f.write("  " + ", ".join(f"0x{b:02X}" for b in data[i:i+16]) + ",\n")
        f.write("};\n\n#endif\n")
    print(f"Done: {sys.argv[2]}")
if __name__ == "__main__": main()