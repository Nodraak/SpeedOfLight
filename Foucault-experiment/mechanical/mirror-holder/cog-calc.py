"""
Load an horizontal slice of the mirror holder as an image, and compute its center
of gravity.
Allow to later tune the OpenScad design to balance the finished piece.
"""

from PIL import Image
import numpy as np
import sys

assert len(sys.argv) == 2

# Load image
img = Image.open(sys.argv[1]).convert("L")  # convert to grayscale
arr = np.array(img)

# Black pixels = 0 (adjust threshold if needed)
black_pixels = np.where(arr < 128)  # returns (y_coords, x_coords)

# Compute centroid
center_y = black_pixels[0].mean()
center_x = black_pixels[1].mean()

D = len(arr)
dx_pxl = center_x - D/2
dy_pxl = center_y - D/2

dx_mm = dx_pxl * 40/D
dy_mm = dy_pxl * 40/D

print(f"Center               : ({center_x:.2f}, {center_y:.2f}) pxl")
print(f"Center of gravity pxl: ({dx_pxl:.2f}, {dy_pxl:.2f}) pxl")
print(f"Center of gravity mm : ({dx_mm:.2f}, {dy_mm:.2f}) mm")

"""
Results:

# build/mirror-laser.png (Note: mirror width=3 mm)  => -1.36 mm
# build/mirror-base-40-4.15.png                     => +1.03 mm
"""
