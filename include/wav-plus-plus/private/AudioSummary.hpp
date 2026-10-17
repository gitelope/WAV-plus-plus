// WAV++
// Copyright (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_AudioSummary_hpp
#define wavplusplus_AudioSummary_hpp

#include <cstdint>
#include <format>
#include <string>

namespace wav
{

inline std::string channelString (uint16_t channels);
inline std::string durationString (uint32_t, uint32_t);
inline std::string formatString (bool, uint16_t, uint16_t);
inline std::string sizeString (uint32_t, bool);

// Synchronizes the order and formatting of parameter output for
// std::string ChunkInfo::summary()
// and
// std::ostream & operator<< (std::ostream &, FileInfo const &).

inline std::string audioSummary (bool isLinear,
                                 uint16_t bitDepth,
                                 uint16_t containerSize,
                                 uint32_t sampleRate,
                                 uint16_t channels,
                                 uint32_t frames,
                                 uint32_t bytes,
                                 bool isPadded)
{
    return "WAVE "
        + formatString (isLinear, bitDepth, containerSize)
        + " / " + std::to_string (sampleRate / 1000) + " kHz"
        + " / " + channelString (channels)
        + " / " + durationString (frames, sampleRate)
        + " / " + std::to_string (frames) + " frames"
        + " / " + sizeString (bytes, isPadded);
}

inline std::string formatString (bool isLinear, uint16_t bitDepth, uint16_t containerSize)
{
    // Sample encoding assumed to be either linear or float.
    const auto amplitudeEncoding {isLinear ? "linear" : "float"};
    auto result = std::format ("{}-bit {} PCM", bitDepth, amplitudeEncoding);

    if (containerSize > bitDepth)
    {
        result += std::format (" / {}-bit container", containerSize);
    }

    return result;
}

inline std::string channelString (uint16_t channels)
{
    switch (channels)
    {
        case 1:  return "mono";
        case 2:  return "stereo";
        default: return std::format ("{} channels", channels);
    }
}

inline std::string durationString (uint32_t frames, uint32_t sampleRate)
{
    const double seconds = static_cast<double> (frames) / sampleRate;
    return seconds < 0.1
        ? std::format ("{:.1f} ms",      seconds * 1000.0)
        : std::format ("{:.1f} seconds", seconds);
}

inline std::string sizeString (uint32_t bytes, bool isPadded)
{
    constexpr uint32_t kB = 1'000;
    constexpr uint32_t MB = 1'000'000;
    constexpr uint32_t GB = 1'000'000'000;

    std::string result;

    if      (bytes >= GB) result = std::format ("{:.1f} GB of audio", static_cast<double> (bytes) / GB);
    else if (bytes >= MB) result = std::format ("{:.1f} MB of audio", static_cast<double> (bytes) / MB);
    else if (bytes >= kB) result = std::format ("{:.1f} kB of audio", static_cast<double> (bytes) / kB);
    else                  result = std::to_string (bytes) + " bytes of audio";

    if (isPadded) result += " (includes padding)";
    return result;
}

} // namespace wav

#endif
