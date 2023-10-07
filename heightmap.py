#! /usr/bin/env python3

from PIL import Image
img = Image.open("./res/depth.png")

with open('./res/depth.bin', 'wb') as o:
    o.write(img.convert('L').tobytes())
