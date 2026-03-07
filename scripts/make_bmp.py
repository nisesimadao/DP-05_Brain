from PIL import Image
import sys
import os

def convert_to_bmps(input_path, output_dir):
    try:
        img = Image.open(input_path)
        # Background composite
        if img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info):
            bg = Image.new('RGB', img.size, (242, 242, 242))
            if img.mode == 'RGBA':
                bg.paste(img, (0, 0), img)
            else:
                bg.paste(img.convert('RGBA'), (0, 0), img.convert('RGBA'))
            img = bg
        else:
            img = img.convert('RGB')
        
        # Ensure output directory exists
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        # 1. Standard 24-bit BMP
        img.save(os.path.join(output_dir, "icon.bmp"), 'BMP')
        print(f"Generated {os.path.join(output_dir, 'icon.bmp')} (24-bit)")

        # 3. 8-bit Palette BMP
        img_8bit = img.convert('P', palette=Image.ADAPTIVE, colors=256)
        img_8bit.save(os.path.join(output_dir, "icon8.bmp"), 'BMP')
        print(f"Generated {os.path.join(output_dir, 'icon8.bmp')} (8-bit palette)")

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python make_bmp.py <input_png> <output_dir>")
        sys.exit(1)
    convert_to_bmps(sys.argv[1], sys.argv[2])
