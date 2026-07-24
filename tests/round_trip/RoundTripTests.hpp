// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_RoundTripTests_hpp
#define wavplusplus_tests_RoundTripTests_hpp

#include "../../include/wav-plus-plus/Reader.hpp"
#include "../../include/wav-plus-plus/Writer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

struct WriteParameters
{
    wav::ExportFormat format;
    std::uint32_t sampleRate;
    std::uint16_t channels;
};

inline bool same (std::string const & filename, std::vector<float> const & expected)
{
    wav::Reader reader {filename};
    std::vector<float> actual (expected.size());
    const auto n = reader.read (actual);
    return n == expected.size() && areEqual (actual, expected);
}

// WRITE OVERLOADS //////////////////////////////////////////////////////////////
//
// size_t write (T const * source, size_t samplesToWrite);
// size_t write (std::vector<T> const & source, size_t samplesToWrite);
// size_t write (std::vector<T> const & source);

inline void Write_PointerAndCount (std::vector<float> const & signal,
                                   WriteParameters const & wp,
                                   TestLog & log)
{
    const auto filename {"test_round_trip_write_pointer_and_count.wav"};

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    const auto samplesWritten = writer.write (signal.data(), signal.size());
    writer.Close();

    const bool result {samplesWritten == signal.size() && same (filename, signal)};
    log.Record (result, "Round Trip - write (T const *, size_t)");

    std::remove (filename);
}

inline void Write_VectorAndCount (std::vector<float> const & signal,
                                  WriteParameters const & wp,
                                  TestLog & log)
{
    const auto filename {"test_round_trip_write_vector_and_count.wav"};

    // Write one frame less than the full vector to make sure the number of
    // samples written is from the supplied count and not the vector's size.
    const auto totalFrames {signal.size() / wp.channels};
    const auto samplesToWrite {(totalFrames - 1) * wp.channels};

    const std::vector<float> expected (signal.begin(), signal.begin() + static_cast<std::ptrdiff_t> (samplesToWrite));

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    const auto samplesWritten = writer.write (signal, samplesToWrite);
    writer.Close();

    const bool result {samplesWritten == samplesToWrite && same (filename, expected)};
    log.Record (result, "Round Trip - write (vector const &, size_t)");

    std::remove (filename);
}

inline void Write_Vector (std::vector<float> const & signal,
                          WriteParameters const & wp,
                          TestLog & log)
{
    const auto filename {"test_round_trip_write_vector.wav"};

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    const auto samplesWritten = writer.write (signal);
    writer.Close();

    const bool result {samplesWritten == signal.size() && same (filename, signal)};
    log.Record (result, "Round Trip - write (vector const &)");

    std::remove (filename);
}

// WRITEFRAMES OVERLOADS ////////////////////////////////////////////////////////
//
// size_t writeFrames (T const * source, size_t framesToWrite);
// size_t writeFrames (std::vector<T> const & source, size_t framesToWrite);
// size_t writeFrames (std::vector<T> const & source);

inline void WriteFrames_PointerAndCount (std::vector<float> const & signal,
                                         WriteParameters const & wp,
                                         TestLog & log)
{
    const auto filename {"test_round_trip_writeFrames_pointer_and_count.wav"};

    const size_t framesToWrite {signal.size() / wp.channels};

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    const auto framesWritten = writer.writeFrames (signal.data(), framesToWrite);
    writer.Close();

    const bool result {framesWritten == framesToWrite && same (filename, signal)};
    log.Record (result, "Round Trip - writeFrames (T const *, size_t)");

    std::remove (filename);
}

inline void WriteFrames_VectorAndCount (std::vector<float> const & signal,
                                        WriteParameters const & wp,
                                        TestLog & log)
{
    const auto filename {"test_round_trip_writeFrames_vector_and_count.wav"};

    // Write one frame less than the full vector to make sure the number of
    // samples written is from the supplied count and not the vector's size.
    const auto totalFrames {signal.size() / wp.channels};
    const auto framesToWrite {totalFrames - 1};

    const std::vector<float> expected (signal.begin(), signal.begin() + static_cast<std::ptrdiff_t> (framesToWrite * wp.channels));

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    const auto framesWritten = writer.writeFrames (signal, framesToWrite);
    writer.Close();

    const bool result {framesWritten == framesToWrite && same (filename, expected)};
    log.Record (result, "Round Trip - writeFrames (vector const &, size_t)");

    std::remove (filename);
}

inline void WriteFrames_Vector (std::vector<float> const & signal,
                                WriteParameters const & wp,
                                TestLog & log)
{
    const auto filename {"test_round_trip_writeFrames_vector.wav"};

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    const auto framesWritten = writer.writeFrames (signal);
    writer.Close();

    const auto totalFrames {signal.size() / wp.channels};
    const bool result {framesWritten == totalFrames && same (filename, signal)};
    log.Record (result, "Round Trip - writeFrames (vector const &)");

    std::remove (filename);
}

// READ OVERLOADS ///////////////////////////////////////////////////////////////
//
// size_t read (T * destination, size_t samplesRequested);
// size_t read (std::vector<T> & destination, size_t samplesRequested);
// size_t read (std::vector<T> & destination);

bool partialReadSuccessful(std::vector<float> const &,std::vector<float> const &,std::size_t,float);

inline void Read_PointerAndCount (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};
    std::vector<float> in (reader.samples());
    const auto samplesRead = reader.read (in.data(), in.size());

    const bool result {samplesRead == reader.samples() && areEqual (in, signal)};
    log.Record (result, "Round Trip - read (T *, size_t)");
}

inline void Read_VectorAndCount (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};

    // Request one frame fewer than the file holds,
    // into a vector sized for the full file to make
    // sure the number of samples read is the supplied
    // count and not the vector's size.
    const auto samplesRequested {(reader.frames() - 1) * reader.channels()};

    // Allocate enough room to store the entire file's contents,
    // and fill with a sentinel value so we can detect if read
    // past the requested count occurs.
    constexpr float sentinelValue {42};
    std::vector<float> in (reader.samples(), sentinelValue);

    const auto samplesRead = reader.read (in, samplesRequested);

    const bool result
    {
        partialReadSuccessful (in, signal, samplesRequested, sentinelValue) &&
        samplesRead == samplesRequested
    };
    log.Record (result, "Round Trip - read (vector &, size_t)");
}

inline void Read_Vector (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};
    std::vector<float> in (reader.samples());
    const auto samplesRead = reader.read (in);

    const bool result {samplesRead == reader.samples() && areEqual (in, signal)};
    log.Record (result, "Round Trip - read (vector &)");
}

// READFRAMES OVERLOADS /////////////////////////////////////////////////////////
//
// PRESERVING INTERLEAVING
//
// size_t readFrames (T * destination, size_t framesRequested);
// size_t readFrames (std::vector<T> & destination, size_t framesRequested);
// size_t readFrames (std::vector<T> & destination);
//
// DEINTERLEAVING
//
// size_t readFrames (T ** const destinations, size_t numChannels, size_t framesRequested);

inline void ReadFrames_PointerAndCount (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};
    const size_t framesRequested {reader.frames()};
    std::vector<float> in (reader.samples());
    const auto framesRead = reader.readFrames (in.data(), framesRequested);

    const bool result {framesRead == framesRequested && areEqual (in, signal)};
    log.Record (result, "Round Trip - readFrames (T *, size_t)");
}

inline void ReadFrames_VectorAndCount (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};

    // Create a destination with enough room to read the whole
    // file and fill with sentinel values.
    constexpr float sentinelValue {42};
    std::vector<float> in (reader.samples(), sentinelValue);

    // Request one frame fewer than the file holds,
    // into a vector sized for the full file to make
    // sure the number of frames read is the supplied
    // count and not the vector's size (in frames).
    const auto framesRequested {reader.frames() - 1};
    const auto framesRead = reader.readFrames (in, framesRequested);

    const auto samplesRequested {framesRequested * reader.channels()};
    const bool result
    {
        partialReadSuccessful (in, signal, samplesRequested, sentinelValue) &&
        framesRead == framesRequested
    };
    log.Record (result, "Round Trip - readFrames (vector &, size_t)");
}

inline void ReadFrames_Vector (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};
    std::vector<float> in (reader.samples());
    const auto framesRead = reader.readFrames (in);

    const bool result {framesRead == reader.frames() && areEqual (in, signal)};
    log.Record (result, "Round Trip - readFrames (vector &)");
}

inline void ReadFrames_Deinterleaved (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    wav::Reader reader {filename};

    const auto frames {reader.frames()};

    std::vector<float> left (frames);
    std::vector<float> right (frames);
    float * destinations[] {left.data(), right.data()};

    const auto channels {reader.channels()};

    const auto framesRead = reader.readFrames (destinations, channels, frames);

    bool valuesMatch {true};
    for (size_t frame = 0; valuesMatch && frame < frames; ++frame)
    {
        valuesMatch =
        (left  [frame] == signal [frame * channels])  &&
        (right [frame] == signal [frame * channels + 1]);
    }
    const bool result
    {
        valuesMatch &&
        framesRead == frames &&
        channels == 2
    };
    log.Record (result, "Round Trip - readFrames (T ** const, size_t, size_t)");
}

inline void ReadStopsAtEOF (std::string const & filename, std::vector<float> const & signal, TestLog & log)
{
    // Make sure we handle the case where there are not enough samples
    // left to read the number of samples (or frames) requested by the
    // client.
    // Read what remains and return that count.

    constexpr float sentinelValue {42};
    {
        wav::Reader reader {filename};
        std::vector<float> in (reader.samples() + 1, sentinelValue);
        // Request more samples than samples remaining in the file:
        const auto samplesRead = reader.read (in.data(), in.size());

        const bool result
        {
            samplesRead == reader.samples() &&
            partialReadSuccessful (in, signal, signal.size(), sentinelValue)
        };
        log.Record (result, "Round Trip - read (T *, size_t) - client requests more samples than samples remaining in file");
    }
    {
        wav::Reader reader {filename};
        const auto framesRequested {reader.frames() + 1};
        std::vector<float> in (framesRequested * reader.channels(), sentinelValue);
        // Request more frames than frames remaining in the file:
        const auto framesRead = reader.readFrames (in.data(), framesRequested);

        const bool result
        {
            framesRead == reader.frames() &&
            partialReadSuccessful (in, signal, signal.size(), sentinelValue)
        };
        log.Record (result, "Round Trip - readFrames (T *, size_t) - client requests more frames than frames remaining in file");
    }
}

inline void TestWriteMethods (std::vector<float> const & signal,
                              WriteParameters const & writeParameters,
                              TestLog & log)
{
    Write_PointerAndCount       (signal, writeParameters, log);
    Write_VectorAndCount        (signal, writeParameters, log);
    Write_Vector                (signal, writeParameters, log);
    WriteFrames_PointerAndCount (signal, writeParameters, log);
    WriteFrames_VectorAndCount  (signal, writeParameters, log);
    WriteFrames_Vector          (signal, writeParameters, log);
}

inline void TestReadMethods (std::vector<float> const & signal,
                             WriteParameters const & wp,
                             TestLog & log)
{
    const auto filename {"test_read_methods.wav"};

    wav::Writer writer {filename, wp.format, wp.sampleRate, wp.channels};
    writer.write (signal);
    writer.Close();

    Read_PointerAndCount       (filename, signal, log);
    Read_VectorAndCount        (filename, signal, log);
    Read_Vector                (filename, signal, log);

    ReadFrames_PointerAndCount (filename, signal, log);
    ReadFrames_VectorAndCount  (filename, signal, log);
    ReadFrames_Vector          (filename, signal, log);
    ReadFrames_Deinterleaved   (filename, signal, log);

    ReadStopsAtEOF             (filename, signal, log);

    std::remove (filename);
}

void TestRoundTrip (TestLog & log)
{
    // interleaved stereo signal
    // L =  0 → 1
    // R = -1 → 0
    const std::vector<float> signal
    {
        0.f,    -1.f,
        0.25f,  -0.75f,
        0.5f,   -0.5f,
        0.75f,  -0.25f,
        1.f,     0.f
    };
    WriteParameters writeParameters {wav::ExportFormat::FloatPCM_32, 48000, 2};

    TestWriteMethods (signal, writeParameters, log);
    TestReadMethods  (signal, writeParameters, log);
}

inline bool partialReadSuccessful (std::vector<float> const & in, std::vector<float> const & signal, std::size_t samplesRequested, float sentinelValue)
{

    const bool partialReadMatchesSignal = areEqual (signal, in, samplesRequested);

    bool sentinelsUntouched {true};
    for (const auto sample : std::span {in}.subspan (samplesRequested))
    {
        if (sample != sentinelValue)
        {
            sentinelsUntouched = false;
            break;
        }
    }

    return partialReadMatchesSignal && sentinelsUntouched;
}

#endif
