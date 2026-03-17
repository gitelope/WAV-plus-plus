# Quantization

When saving to a linear PCM format, floating-point values are converted to integers:

## 16, 24 and 32-bit signed integer

Let max = 2<sup>(n-1)</sup> - 1, where n is the bit depth.

1. Multiply by max.
2. Round to the nearest integer.*
3. Clamp to [-max, max].

\* Dither&mdash;not yet implemented&mdash;would come before rounding.

## 8-bit unsigned integer

1. Multiply by 127.
2. Add 128.
3. Round to the nearest integer.*
4. Clamp to [1, 255].

\* Dither&mdash;not yet implemented&mdash;would come before rounding.

## Properties

Input in the range [-1.0, +1.0] will quantize without clipping.

Signed linear PCM uses an integer range symmetric about zero; unsigned 8-bit PCM uses an integer range centered on 128.

When read by WAV++ and most other libraries*, the dequantized sample range will be symmetric about zero and lie within [-1.0, +1.0] without clipping.

\* This assumes most other libraries divide all samples by a single constant, which is either 2<sup>(n-1)</sup> or 2<sup>(n-1)</sup> - 1, and interpret 128 as the midpoint for unsigned 8-bit LPCM.