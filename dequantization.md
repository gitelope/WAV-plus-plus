# Dequantization

When reading from a linear PCM format, integer values are converted to floating-point:

## 16, 24 and 32-bit signed integer

Let N = 2<sup>(n-1)</sup> and Δ = 1/N,<br>where n is the bit depth.

Dequantization of signed integers consists solely of dividing the integer samples by N.

The results will be in the range [-1, 1 - Δ].

If the file was written by WAV++, the range is further constrained to [-1 + Δ, 1 - Δ].

## 8-bit unsigned integer

1. Subtract 128.
2. Divide by 128.

The results will be in the range [-1, 127/128].

If the file was written by WAV++, the range is further constrained to [-127/128, +127/128].

## Properties
Dequantized samples will lie within [-1.0, +1.0]; dequantization does not clip.

When dequantized, samples written by WAV++ will lie in a range that is symmetric about zero.

## Related
[Quantization in WAV++](quantization.md)
