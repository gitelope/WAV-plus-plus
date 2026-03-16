
# WAV++
A header-only C++ library for reading and writing WAV files.

## Installation
```bash
git clone https://github.com/gitelope/WAV-plus-plus.git
```
Add the `include/` subdirectory to your header search paths.

## Reading a WAV

### Supported Formats

Linear PCM using 8, 16, 24, or 32 bits and float PCM using 32 or 64 bits.

Mono, stereo, and multichannel layouts.

`WAVEFORMAT`, `PCMWAVEFORMAT`, `WAVEFORMATEX`, and `WAVEFORMATEXTENSIBLE`.

### Opening a file

```cpp
#include <wav-plus-plus/Reader.hpp>

const auto filepath {"in.wav"};
wav::Reader reader {filepath};
```
### File Metadata

Information for allocating a buffer, reading, and playback:
```cpp
const auto channels   {reader.channels()};
const auto frames     {reader.frames()};
const auto samples    {reader.samples()};     // = frames * channels
const auto sampleRate {reader.sampleRate()};
```

Additional file encoding information:
```cpp
const auto bitDepth            {reader.bitDepth()};
const auto containerSizeInBits {reader.containerSizeInBits()};
const auto isLinearPCM         {reader.isLinearPCM()};
const auto isFloatPCM          {reader.isFloatPCM()};
```

Alternatively, you can get all the metadata packaged in a single `wav::FileInfo` object:
```cpp
const auto fileInfo            {reader.getInfo()};

const auto channels            {fileInfo.channels};
const auto frames              {fileInfo.frames};
const auto samples             {channels * frames};
const auto sampleRate          {fileInfo.sampleRate};
const auto bitDepth            {fileInfo.bitDepth};
const auto containerSizeInBits {fileInfo.containerSizeInBits};
const auto isLinearPCM         {fileInfo.isLinearPCM};
const auto isFloatPCM          {fileInfo.isFloatPCM};

```

### Sample ordering

Samples can be read in interleaved as they are in the WAV file or deinterleaved into separate channels.

### Interleaved Reads

#### Reading the entire file all at once

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


#### Incremental reads

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

### Deinterleaved Reads

To separate the interleaved audio into channels, pass an array of pointers to destinations (`T**`) for each channel's audio, along with desired channel and frame counts. There are many possibilities including but not limited to:

#### A fixed number of channels into named vectors

```cpp
std::vector<float> left (frames);
std::vector<float> right (frames);
std::vector<float *> destinations {left.data(), right.data()};

const auto framesRead = reader.read (destinations.data(), destinations.size(), frames);
```

#### A fixed number of channels into named arrays

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

#### All channels into a vector of vectors

```cpp
std::vector<std::vector<float>> buffers (channels, std::vector<float> (frames));

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffers[i].data();
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```

#### All channels into a single vector

```cpp
std::vector<float> buffer (samples);

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffer.data() + frames * i;
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```

#### All channels into a single array

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

#### Incremental de-interleaving reads

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

The pointer + size overload works with any contiguous-storage container that provides a pointer to its first element:
```cpp
const auto samplesWritten = writer.write (array, samplesToWrite);
const auto samplesWritten = writer.write (container.ptrToFirstElement(), samplesToWrite);
```

There are convenience overloads for use with `std::vector`:

```cpp
const auto samplesWritten = writer.write (std_vector, samplesToWrite);
const auto samplesWritten = writer.write (std_vector);
```

To be clear, you could use the pointer + size overload with `std::vector`---it's just a little more verbose:
```cpp
const auto samplesWritten = writer.write (std_vector.data(), samplesToWrite);
const auto samplesWritten = writer.write (std_vector.data(), std_vector.size());
```

### Writer::writeFrames()

`writeFrames` also has three overloads:

```cpp
template <FloatOrDouble T>
size_t writeFrames (T const *, size_t);

template <FloatOrDouble T>
size_t writeFrames (std::vector<T> const &, size_t);

template <FloatOrDouble T>
size_t writeFrames (std::vector<T> const &);
```

They function the same way as their `write` counterparts except they take the number of *frames* to write and return the number of *frames* written.

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
