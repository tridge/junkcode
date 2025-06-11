#!/usr/bin/env python3

import os
import sys
import argparse
from PIL import Image
from pathlib import Path
import io

def compress_image(input_path, output_path, max_size_bytes, image_format):
    quality = 95
    step = 5

    with Image.open(input_path) as img:
        img = img.convert("RGB")  # Ensure JPEG-compatible format
        while quality > 5:
            buffer = io.BytesIO()
            img.save(buffer, format=image_format, quality=quality, optimize=True)
            size = buffer.tell()
            if size <= max_size_bytes:
                with open(output_path, 'wb') as f:
                    f.write(buffer.getvalue())
                return size
            quality -= step
    return None

def main():
    parser = argparse.ArgumentParser(description="Re-compress images to be below a max file size.")
    parser.add_argument("path", help="Path to search for images")
    parser.add_argument("--max-size", type=int, required=True, help="Max file size in bytes")
    args = parser.parse_args()

    path = Path(args.path)
    if not path.exists():
        print(f"Error: path '{path}' does not exist.", file=sys.stderr)
        sys.exit(1)

    for root, dirs, files in os.walk(path):
        for name in files:
            ext = name.lower().rsplit(".", 1)[-1]
            if ext not in ("jpg", "jpeg", "png"):
                continue

            filepath = Path(root) / name
            original_size = filepath.stat().st_size

            if original_size <= args.max_size:
                continue  # Already small enough

            print(f"Compressing: {filepath} ({original_size} bytes)")
            tmp_path = filepath.with_suffix(filepath.suffix + ".tmp")

            format = "JPEG" if ext in ("jpg", "jpeg") else "PNG"
            new_size = compress_image(filepath, tmp_path, args.max_size, format)

            if new_size:
                os.replace(tmp_path, filepath)
                print(f" -> Compressed to {new_size} bytes")
            else:
                print(f" -> Failed to compress below {args.max_size} bytes")
                tmp_path.unlink(missing_ok=True)

if __name__ == "__main__":
    main()
