# WAV++

[![CI](https://github.com/gitelope/WAV-plus-plus/actions/workflows/ci.yml/badge.svg)](https://github.com/gitelope/WAV-plus-plus/actions/workflows/ci.yml)

[![License: MIT](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)

A header-only, dependency-free, C++ library for reading and writing WAV files.

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

const std::filesystem::path filepath {"in.wav"};
wav::Reader reader {filepath};
```

### File Metadata

Information for allocating a buffer, reading, and playback:

```cpp
const auto channels    {reader.channels()};
const auto frames      {reader.frames()};
const auto samples     {reader.samples()};     // = frames * channels
const auto sampleRate  {reader.sampleRate()};
const auto channelMask {reader.channelMask()};
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
const auto channelMask         {fileInfo.channelMask};
const auto bitDepth            {fileInfo.bitDepth};
const auto containerSizeInBits {fileInfo.containerSizeInBits};
const auto isLinearPCM         {fileInfo.isLinearPCM};
const auto isFloatPCM          {fileInfo.isFloatPCM};
```

You can log or display a short summary of `FileInfo` with the stream insertion operator, `<<`:

```cpp
std::cout << reader.getInfo() << std::endl;
```

```
WAVE 24-bit linear PCM / 48 kHz / stereo / 0.1 ms / 5 frames / 30 bytes of audio
```

### Sample ordering

Samples can be read in interleaved as they are in the WAV file or deinterleaved into separate channels.

### Interleaved Reading

#### `Reader::read()`

There are three overloads of `read` that read in samples in interleaved order:

```cpp
template <FloatOrDouble T>
size_t read (T *, size_t);

template <FloatOrDouble T>
size_t read (std::vector<T> &, size_t);

template <FloatOrDouble T>
size_t read (std::vector<T> &);
```

The pointer + size overload works with any contiguous-storage container that provides a pointer to its first element:

```cpp
const auto samplesRead = reader.read (array, samplesRequested);
const auto samplesRead = reader.read (container.ptrToFirstElement(), samplesRequested);
```

The remaining two are convenience overloads for use with `std::vector`:

```cpp
const auto samplesRead = reader.read (std_vector, samplesRequested);
const auto samplesRead = reader.read (std_vector);
```

To be clear, you can use the pointer + size overload with `std::vector`—it's just a little more verbose:

```cpp
const auto samplesRead = reader.read (std_vector.data(), samplesRequested);
const auto samplesRead = reader.read (std_vector.data(), std_vector.size());
```

#### `Reader::readFrames()`

`readFrames` also has three overloads:

```cpp
template <FloatOrDouble T>
size_t readFrames (T *, size_t);

template <FloatOrDouble T>
size_t readFrames (std::vector<T> &, size_t);

template <FloatOrDouble T>
size_t readFrames (std::vector<T> &);
```

They function the same way as their `read` counterparts except they take the number of *frames* to read and return the number of *frames* read.

### Deinterleaved Reading

To separate the interleaved audio into channels, pass an array of pointers to destinations (`T**`) for each channel's audio, along with desired channel and frame counts.

```cpp
template <FloatOrDouble T>
size_t read (T ** const destinations, size_t channelsRequested, size_t framesRequested);
```

Following are some examples of the many possible use cases and approaches.

Reading a fixed number of channels into named vectors:

```cpp
std::vector<float> left (frames);
std::vector<float> right (frames);

std::vector<float *> destinations {left.data(), right.data()};

const auto framesRead = reader.read (destinations.data(), destinations.size(), frames);
```

Reading a fixed number of channels into named arrays:

```cpp
float * left   = new float [frames];
float * right  = new float [frames];
float * center = new float [frames];

float * destinations[] = {left, right, center};

const auto framesRead = reader.read (destinations, 3, frames);

delete[] left;
delete[] right;
delete[] center;
```

Reading all channels into a single vector:

```cpp
std::vector<float> buffer (samples);

std::vector<float *> destinations (channels);
for (size_t i = 0; i < channels; ++i)
{
    destinations[i] = buffer.data() + frames * i;
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```

Reading all channels into a single array:

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

Reading all channels into a vector of vectors:

```cpp
std::vector<std::vector<float>> buffers (channels);
for (auto & buffer : buffers)
{
    buffer.resize (frames);
}

std::vector<float*> destinations;
destinations.reserve (channels);
for (auto & channel : buffers)
{
    destinations.push_back (channel.data());
}

const auto framesRead = reader.read (destinations.data(), channels, frames);
```

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

`WAVEFORMATEX` is used (18-byte `fmt` chunk) for mono and stereo 8-bit and 16-bit LPCM files without a channel mask.

`WAVEFORMATEXTENSIBLE` is used (40-byte `fmt` chunk) when the channel count is 3 or more, a channel mask is set, or the bit depth exceeds 16.

### Opening a file for writing

```cpp
#include <wav-plus-plus/Writer.hpp>

const std::filesystem::path filepath {"out.wav"};
const auto encoding   {wav::ExportFormat::LinearPCM_24};
const auto sampleRate {48000};
const auto channels   {2};

wav::Writer writer {filepath, encoding, sampleRate, channels};
```

#### Speaker positions

A channel mask can be provided to assign speaker positions:

```cpp
constexpr auto channelMask =
    wav::SpeakerPosition::FrontLeft    |
    wav::SpeakerPosition::FrontRight   |
    wav::SpeakerPosition::FrontCenter  |
    wav::SpeakerPosition::LowFrequency |
    wav::SpeakerPosition::BackLeft     |
    wav::SpeakerPosition::BackRight;

const auto numChannels {std::popcount (channelMask)};
wav::Writer writer {filepath, encoding, sampleRate, numChannels, channelMask};
```

There are 18 defined speaker positions. Listed in order of their position in the mask:

`FrontLeft`, `FrontRight`, `FrontCenter`,
`LowFrequency`,
`BackLeft`, `BackRight`,
`FrontLeftOfCenter`, `FrontRightOfCenter`,
`BackCenter`,
`SideLeft`, `SideRight`,
`TopCenter`,
`TopFrontLeft`, `TopFrontCenter`, `TopFrontRight`,
`TopBackLeft`, `TopBackCenter`, `TopBackRight`

When no speaker positions are specified but there are three or more channels, `WAVEFORMATEXTENSIBLE` is still used but the channel mask is set to `0`.

A few common speaker configurations have been defined for convenience:

```cpp
wav::SpeakerConfiguration::Unassigned
wav::SpeakerConfiguration::Stereo
wav::SpeakerConfiguration::Quadraphonic
wav::SpeakerConfiguration::Five_Point_One
wav::SpeakerConfiguration::Seven_Point_One
```

```cpp
constexpr auto channelMask {SpeakerConfiguration::Five_Point_One};
wav::Writer writer {filepath, encoding, sampleRate, numChannels, channelMask};
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

The remaining two are convenience overloads for use with `std::vector`:

```cpp
const auto samplesWritten = writer.write (std_vector, samplesToWrite);
const auto samplesWritten = writer.write (std_vector);
```

To be clear, you can use the pointer + size overload with `std::vector`—it's just a little more verbose:

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

To inspect the non-data information in a WAV, use a `wav::ChunkInfo`:

```cpp
#include <wav-plus-plus/ChunkInfo.hpp>

const std::filesystem::path filepath {"in.wav"};
wav::ChunkInfo info {filepath};
std::cout << info << std::endl;
```

```
chunks present
--------------
RIFF 0
fmt  12
data 60

RIFF chunk
----------
id: RIFF
chunk size: 1536060 (= file size in bytes minus 8)
format: WAVE

fmt  chunk
----------
format id: fmt 
format chunk size: 40 (WAVEFORMATEXTENSIBLE)
format: 0xFFFE (extensible)
channels: 2
sample rate: 48000
byte rate: 384000
block align: 8
bits per sample: 32
extra format information size (cbSize): 22
valid bits per sample: 32
channel mask: 0x00000000
[0] unassigned, [1] unassigned
sub format GUID: 00000003-0000-0010-8000-00AA00389B71 (float PCM)

data chunk
----------
id: data
chunk size: 1536000
[DATA]
```

For a short metadata summary:

```cpp
std::cout << info.summary() << std::endl;
```

```
WAVE 32-bit float PCM / 48 kHz / stereo / 4.0 seconds / 192000 frames / 1.5 MB of audio
```

## License

WAV++ is licensed under the MIT License. See [LICENSE](LICENSE) for details.
