
# WAV++
A header-only C++ library for reading and writing WAV files.

## Installation
```bash
git clone https://github.com/gitelope/WAV-plus-plus.git
```
Add the `include/` subdirectory to your header search paths.

## Reading a WAV

### Supported formats

Linear PCM using 8, 16, 24, or 32 bits and float PCM using 32 or 64 bits.

Mono, stereo, and multichannel layouts.

`WAVEFORMAT`, `PCMWAVEFORMAT`, `WAVEFORMATEX`, and `WAVEFORMATEXTENSIBLE`.

### Opening a file

```cpp
#include <wav-plus-plus/Reader.hpp>

wav::Reader reader {filepath};
const auto channels {reader.channels()};
const auto frames {reader.frames()};
const auto samples {reader.samples()};
```

### Reading while preserving interleaved formatting

#### entire file read

```cpp
std::vector<float> buffer (samples);
const auto samplesRead = reader.read (buffer);
```

```cpp
float * const buffer = new float [samples];
const auto samplesRead = reader.read (buffer, samples);
delete[] buffer;
```

Any container that can provide a pointer to the first element of underlying dynamically-sizeable contiguous storage can be used:

```cpp
CustomContainer cc {samples};
const auto samplesRead = reader.read (cc.ptrToFirstElement(), samples);
```


#### incremental read

```cpp
constexpr size_t bufferSize {1024}; // in samples

// Choose one:
float buffer [bufferSize]; <-- size fixed at compile time. Not so good.
std::vector<float> buffer (bufferSize);
std::array<float,bufferSize> buffer; <-- same here


auto samplesLeft {reader.samples()};
while (samplesLeft)
{
    samplesLeft -= reader.read (buffer);
    // process buffer
}
```

### Reading into separate channels (deinterleaving)

To separate the interleaved audio into channels, pass an array of pointers to destinations (`T**`) for each channel's audio, along with desired channel and frame counts. There are many possibilities including but not limited to:

#### a fixed number of channels into named vectors

```cpp
std::vector<float> left (frames);
std::vector<float> right (frames);
std::vector<float *> destinations {left.data(), right.data()};

const auto framesRead = reader.read (destinations.data(), destinations.size(), frames);
```

#### a fixed number of channels into named arrays

```cpp
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

```cpp
std::vector<std::vector<float>> buffers (channels, std::vector<float> (frames));

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffers[i].data();
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```

#### all channels into a single vector

```cpp
std::vector<float> buffer (samples);

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffer.data() + frames * i;
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```

#### all channels into a single array

```cpp
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

### Dequantization

See [dequantization.md](dequantization.md).

## Writing a WAV

### Encoding Options

Amplitudes can be encoded in linear PCM using 8, 16, 24, or 32 bits and float PCM using 32 or 64 bits:

```cpp
wav::ExportFormat::LinearPCM_8
wav::ExportFormat::LinearPCM_16
wav::ExportFormat::LinearPCM_24
wav::ExportFormat::LinearPCM_32

wav::ExportFormat::FloatPCM_32
wav::ExportFormat::FloatPCM_64
```
Mono, stereo, and multichannel layouts are supported.

`PCMWAVEFORMAT` is used.

### Opening a file

```cpp
#include <wav-plus-plus/Writer.hpp>

const auto filepath   {"test.wav"};
const auto encoding   {wav::ExportFormat::LinearPCM_24};
const auto sampleRate {48000};
const auto channels   {2};

wav::Writer writer {filepath, encoding, sampleRate, channels};
```

### Sample ordering

Samples should be interleaved (as they will be in the WAV file). WAV++ does not  currently support deinterleaved input for writing.

### Writer::write()

The main overload to which all the convenience overloads delegate:

```cpp
template <FloatOrDouble T>
size_t Writer::write (T const *, size_t)
```
can be used with any container with underlying contiguous storage which can provide a pointer to its first element. For example, you can pass it a raw array directly:
```cpp
const auto samplesWritten = writer.write (array, samplesToWrite);
```
 and you can use it with `std::vector` and `std::array` by calling their `data()` members:

```cpp
const auto samplesWritten = writer.write (std_vector.data(), samplesToWrite);
const auto samplesWritten = writer.write (std_array.data(),  samplesToWrite);
```

However, for the latter two containers there are preferable overloads that provide a little more convenience and safety:
```cpp
const auto samplesWritten = writer.write (std_vector, samplesToWrite);
const auto samplesWritten = writer.write (std_array,  samplesToWrite);
```
They provide some additional safety by checking that `samplesToWrite` does not exceed the container size.

All of these can be used to write entire renders all at once or for repeated incremental writes with smaller buffers.

And finally, there's an overload for `std::vector` that uses its size to determine the number of samples to write:
```cpp
const auto samplesWritten = writer.write (std_vector);
```
It can be useful when an entire render has been stored in memory and completely fills its container. It's not so useful for writing incrementally as the last buffer of samples is likely to be only partially filled and there's no way to indicate the number of valid samples.

### Writer::writeFrames()

To specify the number of frames instead of samples when writing use `writeFrames()`. As with `write()` it can be used with any container with underlying contiguous storage that provides a pointer to its first element:

```cpp
const auto framesWritten = writer.writeFrames (array,             framesToWrite);
const auto framesWritten = writer.writeFrames (std_vector.data(), framesToWrite);
const auto framesWritten = writer.writeFrames (std_array.data(),  framesToWrite);
```
And similarly for a little more convenience (and safety):

```cpp
const auto framesWritten = writer.writeFrames (std_vector, framesToWrite);
const auto framesWritten = writer.writeFrames (std_array,  framesToWrite);
```
These check the client hasn't requested a number off frames exceeding the container's capacity.

All of these can be used equally well for writing entire renders all at once or a buffer at a time.

And finally:
```cpp
const auto framesWritten = writer.writeFrames (std_vector);
```
It checks to make sure it's `std::vector`'s length corresponds to a whole number of frames. Like its `write()` counterpart, it's convenient for writing an entire render all at once, assuming the render fills its container entirely, but not well-suited for incremental writing.

### Closing a file

The file will be closed when the `Writer` is destroyed but it can be closed earlier with `wav::Writer::Close()`.

### Quantization

See [quantization.md](quantization.md).

## Inspecting a WAV

To inspect the non-data information in a WAV, load it into a `wav::ChunkInfo`.

## License
WAV++ is licensed under the MIT License. See [LICENSE.txt](LICENSE.txt) for details.
