import io
import os
from PIL import Image, ImageDraw

# Helper to draw a modern user/profile icon programmatically.
def generate_user_icon(size):
    scale = 4
    s = size * scale
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    shadow = (49, 50, 68, 255)
    face = (166, 173, 200, 255)
    highlight = (236, 239, 244, 255)
    collar = (129, 141, 166, 255)

    head_r = int(s * 0.26)
    head_cx = s // 2
    head_cy = int(s * 0.28)
    draw.ellipse([head_cx - head_r, head_cy - head_r, head_cx + head_r, head_cy + head_r], fill=shadow)
    draw.ellipse([head_cx - head_r + 6, head_cy - head_r + 6, head_cx + head_r - 6, head_cy + head_r - 6], fill=face)
    draw.ellipse([head_cx - head_r + 12, head_cy - head_r + 12, head_cx + head_r - 12, head_cy + head_r - 12], fill=highlight)

    torso_w = int(s * 0.58)
    torso_h = int(s * 0.28)
    torso_x0 = (s - torso_w) // 2
    torso_y0 = int(s * 0.52)
    torso_x1 = torso_x0 + torso_w
    torso_y1 = torso_y0 + torso_h
    draw.rounded_rectangle([torso_x0, torso_y0, torso_x1, torso_y1], radius=int(s * 0.08), fill=face)
    draw.rounded_rectangle([torso_x0 + 8, torso_y0 + 8, torso_x1 - 8, torso_y1 - 8], radius=int(s * 0.06), fill=highlight)

    collar_y = torso_y0 + torso_h // 2
    draw.rectangle([torso_x0 + 6, collar_y - 2, torso_x1 - 6, collar_y + 6], fill=collar)

    return img.resize((size, size), Image.LANCZOS)

# File-backed icons can be generated from source assets.
# The user icon is loaded from assets/user_icon.png and embedded as PNG.
icons = {
    "folder": (32, r"C:\Users\admin\.gemini\antigravity\brain\c242a3cb-f417-48cf-a017-e667eb3e684a\folder_icon_1783947759608.jpg"),
    "settings": (32, r"C:\Users\admin\.gemini\antigravity\brain\c242a3cb-f417-48cf-a017-e667eb3e684a\settings_icon_1783947776188.jpg"),
    "terminal": (32, r"C:\Users\admin\.gemini\antigravity\brain\c242a3cb-f417-48cf-a017-e667eb3e684a\terminal_icon_1783947787285.jpg"),
    "trash": (32, r"C:\Users\admin\.gemini\antigravity\brain\c242a3cb-f417-48cf-a017-e667eb3e684a\trash_icon_1783947797137.jpg"),
    "start": (32, r"C:\Users\admin\.gemini\antigravity\brain\c242a3cb-f417-48cf-a017-e667eb3e684a\start_icon_1783948139752.jpg"),
    "cursor": (24, r"C:\Users\admin\.gemini\antigravity\brain\c242a3cb-f417-48cf-a017-e667eb3e684a\cursor_icon_1783948148625.jpg"),
    "user": (128, r"assets\user_icon.png")
}

# Generate raw ARGB arrays for fallback and runtime PNG assets for the loader.
with open("include/smooth_icons.h", "w", encoding="utf-8") as f:
    f.write("#pragma once\n\n")
    f.write("#include <stdint.h>\n\n")

    for name, data in icons.items():
        icon_size = data[0]
        generator = data[2] if len(data) > 2 else None
        path = data[1] if len(data) > 1 else None

        img = None
        if generator is not None:
            img = generator(icon_size)
        elif path and os.path.exists(path):
            img = Image.open(path).convert("RGBA")
            w, h = img.size
            min_dim = min(w, h)
            left = (w - min_dim) // 2
            top = (h - min_dim) // 2
            img = img.crop((left, top, left + min_dim, top + min_dim))
            img = img.resize((icon_size, icon_size), Image.LANCZOS)
        else:
            print(f"Skipping {name}, no source available")
            continue

        pixels = list(img.getdata())

        f.write(f"// Generated icon for {name} ({icon_size}x{icon_size})\n")
        f.write(f"static const uint32_t icon_{name}[{icon_size * icon_size}] = {{\n        ")

        for i, px in enumerate(pixels):
            r, g, b, a = px
            if a == 255:
                brightness = max(r, g, b)
                if brightness < 10:
                    a = 0
                elif brightness < 60:
                    a = int((brightness - 10) * (255 / 50))
                else:
                    a = 255

            if 0 < a < 255:
                boost = 255.0 / a
                r = min(255, int(r * boost))
                g = min(255, int(g * boost))
                b = min(255, int(b * boost))

            val = (a << 24) | (r << 16) | (g << 8) | b
            f.write(f"0x{val:08X},")
            if (i + 1) % icon_size == 0:
                f.write("\n        ")
        f.write("};\n\n")

png_lines = ['#pragma once', '', '#include <stdint.h>', '#include <stddef.h>', '']
for name, data in icons.items():
    icon_size = data[0]
    generator = data[2] if len(data) > 2 else None
    path = data[1] if len(data) > 1 else None

    img = None
    if generator is not None:
        img = generator(icon_size)
    elif path and os.path.exists(path):
        img = Image.open(path).convert("RGBA")
        w, h = img.size
        min_dim = min(w, h)
        left = (w - min_dim) // 2
        top = (h - min_dim) // 2
        img = img.crop((left, top, left + min_dim, top + min_dim))
        img = img.resize((icon_size, icon_size), Image.LANCZOS)
    else:
        continue

    buffer = io.BytesIO()
    img.save(buffer, format="PNG")
    png = buffer.getvalue()

    png_lines.append(f"// Embedded PNG for icon_{name} ({icon_size}x{icon_size})")
    png_lines.append(f"static const unsigned char icon_{name}_png[] = {{")
    line = '    '
    for idx, byte in enumerate(png):
        line += f"0x{byte:02X},"
        if (idx + 1) % 16 == 0:
            png_lines.append(line)
            line = '    '
    if line.strip():
        png_lines.append(line)
    png_lines.append('};')
    png_lines.append(f"static const size_t icon_{name}_png_size = {len(png)};")
    png_lines.append('')

with open("include/png_icons.h", "w", encoding="utf-8") as f:
    f.write("\n".join(png_lines))

print("Generated include/smooth_icons.h and include/png_icons.h with high-quality icon assets")
