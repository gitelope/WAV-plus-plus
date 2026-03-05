
# WAV++
A header-only C++ library for reading and writing WAV files.

## Encoding Options
Amplitudes can be encoded in linear PCM using 8, 16, 24, or 32 bits and float PCM using 32 or 64 bits. Mono, stereo, and multichannel layouts are supported.

---

## Reading a WAV

### Opening a file
```
#include <wav-plus-plus/Reader.hpp>
#include <wav-plus-plus/Exceptions.hpp>

wav::Reader reader {filepath};
const auto channels {reader.channels()};
const auto frames {reader.frames()};
const auto samples {reader.samples()};
```

### Reading while preserving interleaved formatting

#### entire file into std::vector

```
std::vector<float> buffer (samples);
const auto samplesRead = reader.read (buffer);
```
#### entire file into array

```
float * const buffer = new float [samples];
const auto samplesRead = reader.read (buffer, samples);
delete[] buffer;
```

#### incremental read into std::vector, array, or std::array
```
constexpr size_t bufferSize {1024}; // in samples

// Choose one:
std::vector<float> buffer (bufferSize);
std::array<float,bufferSize> buffer;
float buffer [bufferSize];

size_t samplesLeft {reader.samples()};
while (samplesLeft)
{
    samplesLeft -= reader.read (buffer);
    // process buffer
}
```

### Reading into separate channels (deinterleaving)

To separate the interleaved audio into channels, pass an array of pointers to destinations (`T**`) for each channel's audio, along with desired channel and frame counts. There are many possibilities including but not limited to:

#### a fixed number of channels into named vectors

```
std::vector<float> left (frames);
std::vector<float> right (frames);
std::vector<float *> destinations {left.data(), right.data()};

reader.read (destinations.data(), destinations.size(), frames);
```

#### a fixed number of channels into named arrays
```
constexpr size_t channelsToRead {3};
float * left = new float [frames];
float * right = new float [frames];
float * center = new float [frames];
float * destinations[] = {left, right, center};

const auto framesRead = reader.read (destinations, channelsToRead, frames);

delete[] left;
delete[] right;
delete[] center;
```

#### all channels into a vector of vectors
```
std::vector<std::vector<float>> buffers (channels, std::vector<float> (frames));

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffers[i].data();
}

const size_t framesRead = reader.read (destinations.data(), channels, frames);
```

#### all channels into a single vector
```
std::vector<float> buffer (samples);

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffer.data() + frames * i;
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```
#### all channels into a single array
```
float * buffer = new float [samples];

float * * destinations = new float * [channels];
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffer + frames * i;
}

const auto framesRead = reader.read (destinations, channels, frames);

delete[] destinations;
delete[] buffer;
```

#### incremental de-interleaving reads

To perform an incremental de-interleaving read, pass in a value to the `frames` parameter that is less than the total number of frames. Similarly, you need not read all channels present in a file. You may instead request the first `n` channels less than or equal to the total number of channels in the file.

### Error handling

Both `Reader`'s constructor and `Reader::read` throw a `wav::ReadError` when a problem is encountered so it's a good idea to use a `try-catch` when constructing a `Reader` or calling `read`.

---


## License
WAV++ is licensed under the MIT License. See LICENSE.txt for details.
