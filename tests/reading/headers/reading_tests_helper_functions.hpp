
#ifndef wavplusplus_reading_tests_helper_functions_hpp
#define wavplusplus_reading_tests_helper_functions_hpp

// Write byte vector to a file.
inline void writeFile (std::string const & filepath, std::vector<uint8_t> const & bytes)
{
    auto out = std::unique_ptr<FILE, decltype(&fclose)> (fopen (filepath.c_str(), "wb"), &fclose);

    if (!out)
    {
        throw std::runtime_error ("Failed to open file for writing: " + filepath);
    }

    if (fwrite (bytes.data(), 1, bytes.size(), out.get()) != bytes.size())
    {
        throw std::runtime_error ("Failed to write file: " + filepath);
    }
}

// Append a little-endian uint16 to byte vector.
inline void push_uint16_le (std::vector<uint8_t> & v, uint16_t value)
{
    v.push_back (static_cast<uint8_t> (value     ));
    v.push_back (static_cast<uint8_t> (value >> 8));
}

// Append a little-endian uint32 to byte vector.
inline void push_uint32_le (std::vector<uint8_t> & v, uint32_t value)
{
    v.push_back (static_cast<uint8_t> (value      ));
    v.push_back (static_cast<uint8_t> (value >> 8 ));
    v.push_back (static_cast<uint8_t> (value >> 16));
    v.push_back (static_cast<uint8_t> (value >> 24));
}

// Append a 4-character string to a byte vector.
inline void push_fourCC (std::vector<uint8_t> & v, const char * fourCC)
{
    v.push_back (static_cast<uint8_t> (fourCC[0]));
    v.push_back (static_cast<uint8_t> (fourCC[1]));
    v.push_back (static_cast<uint8_t> (fourCC[2]));
    v.push_back (static_cast<uint8_t> (fourCC[3]));
}

inline void push_riff_header (std::vector<uint8_t> & v, uint32_t riffChunkSize)
{
    // RIFF header
    push_fourCC    (v, "RIFF");
    push_uint32_le (v, riffChunkSize);
    push_fourCC    (v, "WAVE");
}

inline void push_silent_data_chunk (std::vector<uint8_t> & v, uint32_t dataChunkSize)
{
    // header
    push_fourCC    (v, "data");
    push_uint32_le (v, dataChunkSize);

    // silence
    v.resize (v.size() + dataChunkSize, 0);

    // padding
    if (dataChunkSize % 2) v.push_back (0);
}

inline void push_fmt_header (std::vector<uint8_t> & v, uint32_t formatChunkSize)
{
    push_fourCC    (v, "fmt ");
    push_uint32_le (v, formatChunkSize);
}

inline void push_WAVEFORMAT_fmt (std::vector<uint8_t> & v, uint16_t formatCode,
                                 uint16_t channels, uint32_t sampleRate,
                                 uint32_t byteRate, uint16_t blockAlign)
{
    push_uint16_le (v, formatCode);
    push_uint16_le (v, channels);
    push_uint32_le (v, sampleRate);
    push_uint32_le (v, byteRate);
    push_uint16_le (v, blockAlign);
}

inline void push_PCMWAVEFORMAT_fmt (std::vector<uint8_t> & v, uint16_t bitDepth)
{
    push_uint16_le (v, bitDepth);
}

#endif
