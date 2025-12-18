#!/usr/bin/env python3
import sys, os
from PIL import Image

def rgb_to_st(r, g, b):
    return (((r >> 5) & 0x7) << 8) | (((g >> 5) & 0x7) << 4) | ((b >> 5) & 0x7)

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 png2st_final.py input.png output.h"); sys.exit(1)

    img = Image.open(sys.argv[1]).convert("RGB")
    w, h = img.size
    base = os.path.splitext(os.path.basename(sys.argv[1]))[0].lower()

    # 1. 팔레트 추출 (배경/글자용 2칸 제외한 14색만 추출)
    idx_img = img.quantize(colors=14, method=Image.MAXCOVERAGE).convert("P")
    temp_pal = idx_img.getpalette()[:42] # 14색 RGB
    
    # 2. 최종 팔레트 재구성 (0번 검정, 15번 흰색 고정)
    final_pal = [(0, 0, 0)] * 16
    for i in range(14):
        final_pal[i+1] = (temp_pal[i*3], temp_pal[i*3+1], temp_pal[i*3+2])
    final_pal[15] = (255, 255, 255) # 15번 흰색

    with open(sys.argv[2], "w") as f:
        f.write(f"#ifndef {base.upper()}_H\n#define {base.upper()}_H\n\n")
        f.write(f"static const unsigned short {base}_palette[16] = {{\n    ")
        for i, (r, g, b) in enumerate(final_pal):
            f.write(f"0x{rgb_to_st(r, g, b):03X}{', ' if i<15 else ''}")
            if (i+1)%8==0: f.write("\n    ")
        f.write("};\n\n")

        # 3. 데이터 변환 (인덱스를 1씩 밀어서 매핑)
        f.write(f"static const unsigned char {base}_data[] = {{\n    ")
        data_bytes = []
        for y in range(h):
            for x in range(0, w, 16):
                p = [0]*4
                for i in range(16):
                    # 이미지 원본 색상과 가장 가까운 final_pal 인덱스 찾기
                    px = img.getpixel((x+i, y))
                    best_idx = 0
                    min_dist = 1000000
                    for p_idx, p_color in enumerate(final_pal):
                        dist = sum((a-b)**2 for a, b in zip(px, p_color))
                        if dist < min_dist:
                            min_dist = dist
                            best_idx = p_idx
                    
                    for b in range(4):
                        if (best_idx >> b) & 1: p[b] |= (1 << (15-i))
                for val in p: data_bytes.extend([(val>>8)&0xFF, val&0xFF])
        
        for i, b in enumerate(data_bytes):
            f.write(f"0x{b:02X}, ")
            if (i+1)%16==0: f.write("\n    ")
        f.write("\n};\n#endif\n")

if __name__ == "__main__": main()