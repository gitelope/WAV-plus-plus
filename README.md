# WAV++
A header-only C++ library for reading and writing WAV files.

## Encoding options
Amplitudes can be encoded in linear PCM using 8, 16, 24, or 32 bits and float PCM using 32 or 64 bits. Mono, stereo, and multichannel layouts are supported.

## Usage

### Reading an entire file into memory
```
#include <wav-plus-plus/Reader.hpp>

wav::Reader reader {filepath};
std::vector<float> in (reader.samples());
reader.read (in);
```
Works with std::vector and C arrays.

### Reading incrementally
```
wav::Reader reader {filepath};
std::vector<float> buffer (1024);
size_t samplesLeft {reader.samples()};

while (samplesLeft)
{
    samplesLeft -= reader.read (buffer);
    // process buffer
}
```
Works with std::vector, std::array, and C arrays.

### Channel-separated reading
```
wav::Reader reader {filepath};
const size_t frames {reader.frames()};

std::vector<float> left (frames);
std::vector<float> right (frames);

float * channels[] = {left.data(), right.data()};
reader.read (channels, 2, frames);
```
## License
WAV++ is licensed under the MIT License. See LICENSE.txt for details.
