
#ifndef wavplusplus_AudioSummary_hpp
#define wavplusplus_AudioSummary_hpp

#include <cstdint>
#include <format>
#include <string>

namespace wav
{

inline std::string channelString (uint16_t channels);
inline std::string durationString (uint32_t, uint32_t);
inline std::string sizeString (uint64_t);

// Synchronizes the order of parameter output for ChunkInfo::summary
// and std::ostream & operator<< (std::ostream &, FileInfo const &)
// Does a little formatting but delegates formatting for length in time and storage.

inline std::string audioSummary (std::string const & formatString,
                                 uint32_t sampleRate,
                                 uint16_t channels,
                                 uint32_t frames,
                                 uint64_t bytes)
{
    return "WAVE " + formatString
    + " / " + std::to_string (sampleRate / 1000) + " kHz"
    + " / " + channelString (channels)
    + " / " + durationString (frames, sampleRate)
    + " / " + std::to_string (frames) + " frames"
    + " / " + sizeString (bytes);
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

inline std::string sizeString (uint64_t bytes)
{
    constexpr uint64_t kB = 1'000;
    constexpr uint64_t MB = 1'000'000;
    constexpr uint64_t GB = 1'000'000'000;

    if      (bytes >= GB) return std::format ("{:.1f} GB of audio", static_cast<double> (bytes) / GB);
    else if (bytes >= MB) return std::format ("{:.1f} MB of audio", static_cast<double> (bytes) / MB);
    else if (bytes >= kB) return std::format ("{:.1f} kB of audio", static_cast<double> (bytes) / kB);
    else                  return std::to_string (bytes) + " bytes of audio";
}



} // namespace wav

#endif
