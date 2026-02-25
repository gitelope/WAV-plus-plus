
// Works with std::vector, C-style arrays, and std::array.
// If the number of samples desired is given, an attempt will be made to read or write
// that number of samples, else the size of the container will be used as the number
// of samples to read or write.
// For C-style arrays whose size is known at compile time, (even arrays created with new?),
// both options exist. For dynamic (?) C-style arrays, the number of sample to read/write must be supplied.
//
// writer.write (vec);
// writer.write (vec, samplesRequested);
//
// writer.write (stdArray);
// writer.write (stdArray, samplesResquested);
//
// writer.write (C_array_fixed);
// writer.write (C_array_fixed, samplesRequested);
//
// writer.write (C_array_dynamic, samplesRequested);
//
// The above options also apply to reading with Reader::read().

/*

 sample encoding options:
 linear PCM: 8-bit, 16-bit, 24-bit, and 32-bit
 float PCM: 32-bit and 64-bit

 channel options:
 mono, stereo, and multichannel

 format:
 PCMWAVEFORMAT (very common, but technically non-standard for some of the above options)

 ---

 quantization (linear PCM only):

 double or float -> signed int (16, 24 and 32 bits):
 1. Multiply by 2^(bits-1) - 1
 (e.g. 32,767 for 16-bit linear PCM)
 2. Round to nearest integer.*
 3. Clamp to [1 - 2^(bits-1), 2^(bits-1) - 1]
 (e.g. [-32,767, 32,767] for 16-bit linear PCM)

 double or float -> unsigned 8-bit int:
 1. Multiply by 127
 2. Add 128.
 3. Round to nearest integer.*
 4. Clamp to [1, 255].

 * Optional TPDF dither (not implemented) would come before rounding.

 */

# License
WAV++ is licensed under the MIT License. See LICENSE.txt for details.
