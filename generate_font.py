import os
from PIL import Image, ImageDraw, ImageFont

# We will generate an 8x16 anti-aliased font array
char_width = 8
char_height = 16
font_size = 14

# Use monospace fonts!
font_paths = [
    "C:/Windows/Fonts/consola.ttf",
    "C:/Windows/Fonts/lucon.ttf",
    "C:/Windows/Fonts/cour.ttf"
]

font = None
for path in font_paths:
    if os.path.exists(path):
        font = ImageFont.truetype(path, font_size)
        print(f"Using font: {path}")
        break

if not font:
    font = ImageFont.load_default()
    print("Using default PIL font")

with open("include/smooth_font.h", "w") as f:
    f.write("#pragma once\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"// Generated smooth anti-aliased font: {char_width}x{char_height}\n")
    f.write(f"static const uint8_t smooth_font[128][{char_width * char_height}] = {{\n")
    
    for ascii_val in range(128):
        if ascii_val < 32 or ascii_val > 126:
            f.write(f"    // {ascii_val}\n")
            f.write("    { 0 },\n")
            continue
            
        char = chr(ascii_val)
        
        # Create a new image for the character
        img = Image.new("L", (char_width, char_height), 0)
        draw = ImageDraw.Draw(img)
        
        # Monospace rendering: draw from left edge, vertically centered
        bbox = draw.textbbox((0, 0), char, font=font)
        text_h = bbox[3] - bbox[1]
        y = (char_height - text_h) // 2 - bbox[1]
        
        draw.text((0, y), char, font=font, fill=255)
        
        pixels = list(img.getdata())
        
        f.write(f"    // {ascii_val} '{char}'\n")
        f.write("    {\n        ")
        for i, px in enumerate(pixels):
            f.write(f"0x{px:02X},")
            if (i + 1) % char_width == 0:
                f.write("\n        ")
        f.write("    },\n")
        
    f.write("};\n")
    f.write(f"static const int smooth_font_width = {char_width};\n")
    f.write(f"static const int smooth_font_height = {char_height};\n")

print("Generated include/smooth_font.h")
