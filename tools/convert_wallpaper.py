import sys
from PIL import Image

def generate_header(image_path, output_path):
    print(f"Loading {image_path}...")
    try:
        img = Image.open(image_path)
    except Exception as e:
        print(f"Failed to open image: {e}")
        return

    print("Resizing to 800x600...")
    img = img.resize((800, 600), Image.Resampling.LANCZOS)
    img = img.convert('RGB')
    pixels = img.load()

    print(f"Writing to {output_path}...")
    with open(output_path, 'w') as f:
        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")
        f.write("static const uint32_t wallpaper_data[480000] = {\n")
        
        count = 0
        for y in range(600):
            for x in range(800):
                r, g, b = pixels[x, y]
                # Combine RGB to 0xRRGGBB format
                color_val = (r << 16) | (g << 8) | b
                f.write(f"0x{color_val:06X},")
                count += 1
                if count % 16 == 0:
                    f.write("\n")
        f.write("\n};\n")
    print("Done!")

if __name__ == "__main__":
    image_path = r"C:\Users\admin\OneDrive\Desktop\mini Os\assets\wallpapers\jenos_bg.jpg"
    output_path = r"C:\Users\admin\OneDrive\Desktop\JenOS-Kernel\wallpaper.h"
    generate_header(image_path, output_path)
