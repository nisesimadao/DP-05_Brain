from PIL import Image
import sys
import os

def convert_to_bmps(input_path):
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
        
        # 1. Standard 24-bit BMP
        img.save("icon.bmp", 'BMP')
        print("Generated icon.bmp (24-bit)")

        # 2. 16-bit BMP (RGB565) - Often more compatible for CE
        # Pillow doesn't support direct 16-bit BMP save easily, 
        # but standard save with RGB might be 24-bit.
        # We can try to force 8-bit (palette) which is also very compatible.
        
        # 3. 8-bit Palette BMP
        img_8bit = img.convert('P', palette=Image.ADAPTIVE, colors=256)
        img_8bit.save("icon8.bmp", 'BMP')
        print("Generated icon8.bmp (8-bit palette)")

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    convert_to_bmps("icon.png")
