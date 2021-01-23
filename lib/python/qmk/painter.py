"""Functions that help us work with Quantum Painter's file formats.
"""
import math
import lzf
from PIL import Image, ImageFont


def rescale_byte(val, maxval):
    """Rescales a byte value to the supplied range, i.e. [0,255] -> [0,maxval].
    """
    return int(round(val * maxval / 255.0))


def rgb888_to_rgb565(r, g, b):
    """Convert an rgb triplet to the equivalent rgb565 integer.
    """
    rgb565 = (rescale_byte(r, 31) << 11 | rescale_byte(g, 63) << 5 | rescale_byte(b, 31))
    return rgb565


def palettize_image(im, ncolors, mono=False):
    """Convert an image to a palette and packed byte array combo.
    """

    # Ensure we have a valid number of colors for the palette
    if ncolors <= 0 or ncolors > 256 or (ncolors & (ncolors - 1) != 0):
        raise ValueError("Number of colors must be 2, 4, 16, or 256.")

    # Work out where we're getting the bytes from
    if mono:
        # If mono, convert input to grayscale, then to RGB, then grab the raw bytes corresponding to the intensity of the red channel
        image_bytes = ImageOps.grayscale(im).convert("RGB").tobytes("raw", "R")
    else:
        # If color, convert input to RGB, palettize based on the supplied number of colours, then get the raw palette bytes
        image_bytes = im.convert("RGB").convert("P", palette=Image.ADAPTIVE, colors=ncolors).tobytes("raw", "P")

    # Work out how much data we're actually processing
    image_bytes_len = len(image_bytes)
    shifter = math.log2(ncolors)
    pixels_per_byte = int(8 / shifter)

    # If in RGB mode, convert the palette to rgb triplet
    palette = []
    if not mono:
        pal = im.getpalette()
        for n in range(0, ncolors * 3, 3):
            palette.append([pal[n + 0], pal[n + 1], pal[n + 2]])

    # Convert to packed pixel byte array
    bytearray = []
    for x in range(int(image_bytes_len / pixels_per_byte)):
        byte = 0
        for n in range(pixels_per_byte):
            byte_offset = x*pixels_per_byte + n
            if byte_offset < image_bytes_len:
                if mono:
                    # If mono, each input byte is a grayscale [0,255] pixel -- rescale to the range we want then pack together
                    byte = byte | (rescale(image_bytes[byte_offset], ncolors - 1) << int(n * shifter))
                else:
                    # If color, each input byte is the index into the colour palette -- pack them together
                    byte = byte | ((image_bytes[byte_offset] & (ncolors - 1)) << int(n * shifter))
        bytearray.append(byte)

    return (palette, bytearray)


def image_to_rgb565(im):
    """Convert an image to a rgb565 byte array.
    """
    # Create input to RGB, then create a byte array in 24 bit RGB format
    image_bytes = im.convert("RGB").tobytes("raw", "RGB")
    image_bytes_len = len(image_bytes)

    # Convert 24-bit RGB to 16-bit rgb565
    rgb565array = []
    for x in range(int(image_bytes_len / 3)):
        r = image_bytes[x*3 + 0]
        g = image_bytes[x*3 + 1]
        b = image_bytes[x*3 + 2]
        rgb565 = rgb888_to_rgb565(r, g, b)
        rgb565array.append((rgb565 >> 8) & 0xFF)
        rgb565array.append(rgb565 & 0xFF)

    return rgb565array
