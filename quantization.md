
# double or float -> signed integer (16, 24 and 32 bits):
1. Multiply by 2^(bits-1) - 1
(e.g. 32,767 for 16-bit linear PCM)
2. Round to nearest integer.*
3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1]
(e.g. [-32,767, 32,767] for 16-bit linear PCM)

double or float -> unsigned 8-bit integer:
1. Multiply by 127
2. Add 128.
3. Round to nearest integer.*
4. Clamp to [1, 255].

* Optional TPDF dither (not implemented) would come before rounding.


ORIGINAL TEXT
-------------

quantization (linear PCM only):
-
- double or float -> signed int (16, 24 and 32 bits):
- 1. Multiply by 2^(bits-1) - 1
- (e.g. 32,767 for 16-bit linear PCM)
- 2. Round to nearest integer.*
- 3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1]
- (e.g. [-32,767, 32,767] for 16-bit linear PCM)
-
- double or float -> unsigned 8-bit int:
- 1. Multiply by 127
- 2. Add 128.
- 3. Round to nearest integer.*
- 4. Clamp to [1, 255].
-
- * Optional TPDF dither (not implemented) would come before rounding.