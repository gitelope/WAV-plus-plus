# Quantization

When saving to a linear PCM format, floating-point values are converted to integers:

## 16, 24 and 32-bit signed integer
Let max = 2<sup>(n-1)</sup> - 1, where n is the bit depth.
1. Multiply by max.
2. Round to the nearest integer.*
3. Clamp to [-max, max].



## 8-bit unsigned integer

1. Multiply by 127.
2. Add 128.
3. Round to the nearest integer.*
4. Clamp to [1, 255].

\* Dither---not yet implemented---would come before rounding.
