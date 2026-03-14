
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

const auto filepath {"in.wav"};

wav::Reader reader {filepath};
const auto channels {reader.channels()};
const auto frames {reader.frames()};
const auto samples {reader.samples()};
```

### Reading while preserving interleaved formatting

#### reading the entire file all at once

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


#### reading incrementally

Buffer size fixed at compile time:
```cpp
float buffer [fixedSize];
std::array<float,fixedSize> buffer;
```
Dynamically-sized buffer size determined at runtime:
```cpp
std::vector<float> buffer (dynamicSize);
auto * buffer = new float[dynamicSize]; <--- this one would require supplying the size so would not work as "reader.read (buffer)" down below
```

```cpp
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

### Dequantization

See [dequantization.md](dequantization.md).

### Error handling

`Reader::Reader`, `Reader::read`, and `Reader::readFrames` throw a `wav::ReadError` when a problem is encountered so it's a good idea to use a `try-catch` when constructing a `Reader`, or calling `read` or `readFrames`.

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

### Opening a file for writing

```cpp
#include <wav-plus-plus/Writer.hpp>

const auto filepath   {"out.wav"};
const auto encoding   {wav::ExportFormat::LinearPCM_24};
const auto sampleRate {48000};
const auto channels   {2};

wav::Writer writer {filepath, encoding, sampleRate, channels};
```

### Sample ordering

Samples should be interleaved (as they will be in the WAV file). WAV++ does not  currently support deinterleaved input for writing.

### Writer::write()

There are three overloads of `write`:

```cpp
template <FloatOrDouble T>
size_t Writer::write (T const *, size_t);

template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const &, size_t);

template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const &);
```

The main, general-purpose overload to which all the convenience overloads delegate:

```cpp
template <FloatOrDouble T>
size_t Writer::write (T const *, size_t);
```
can be used with raw arrays:
```cpp
const auto samplesWritten = writer.write (array, samplesToWrite);
```
as well as any container with underlying contiguous storage which can provide a pointer to its first element:
```cpp
const auto samplesWritten = writer.write (container.ptrToFirstElement(), samplesToWrite);
```
So, for example, it can be used with `std::vector`:

```cpp
const auto samplesWritten = writer.write (std_vector.data(), samplesToWrite);
```

However, there is a preferable overload for `std::vector` that provides a little more convenience:

```cpp
template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const &, size_t);
```
because it takes a `std::vector` directly:

```cpp
const auto samplesWritten = writer.write (std_vector, samplesToWrite);
```
It also provides a little more safety by checking that `samplesToWrite` does not exceed the `vector`'s size.

Both of these overloads can be used to write entire renders all at once or for repeated incremental writes with smaller buffers.

Lastly, there's a more specialized overload for `std::vector` that uses its size to determine the number of samples to write:
```cpp
template <FloatOrDouble T>
size_t Writer::write (std::vector<T> const &);
```

```cpp
const auto samplesWritten = writer.write (std_vector);
```
It can be useful when an entire render has been stored in memory and completely fills its container. In general, it's not useful for writing incrementally as the last buffer of samples is likely to be only partially filled and there's no way to indicate the number of valid samples.

### Writer::writeFrames()

Similarly to `write`, `writeFrames` has three overloads:

```cpp
template <FloatOrDouble T>
size_t writeFrames (T const *, size_t);

template <FloatOrDouble T>
size_t writeFrames (std::vector<T> const &, size_t);

template <FloatOrDouble T>
size_t writeFrames (std::vector<T> const &);
```

They function the same way as their `write` counterparts except they take the number of *frames* to write and return the number of *frames* written as opposed to the number of *samples*.

Like its counterpart, the first `std::vector` overload performs bounds checking and provides convenience for all `std::vector` use cases. The second checks that the `vector` contains a whole number of frames and, like its counterpart, provides more convenience for a more specialized use case.

### Closing a file

The file will be closed when the `Writer` is destroyed but it can be closed earlier with `wav::Writer::Close()`.

### Quantization

See [quantization.md](quantization.md).

### Error handling

`Writer::Writer`, `Writer::write`, and `Writer::writeFrames` throw a `wav::WriteError` when a problem is encountered so it's a good idea to use a `try-catch` when constructing a `Writer`, or calling `write` or `writeFrames`.

## Inspecting a WAV

To inspect the non-data information in a WAV, load it into a `wav::ChunkInfo`.

## License
WAV++ is licensed under the MIT License. See [LICENSE.txt](LICENSE.txt) for details.
