
#include "../shared/SaveWAV.hpp"
#include "../shared/WaveFile.hpp"

#include <vector>
#include <string>

// 1. Write with wav::SaveWAV.
// 2. Inspect with wav::File.

void LoadWaveFile (std::string const & filepath)
{
    // load wave file
    wav::File waveFile {filepath, true /*verbose*/};
    waveFile.PrintChunks();
    std::cout << std::endl;
    waveFile.PrintFileInfo();
    std::cout << std::endl;
    waveFile.PrintSamples();
}

int main (int argc, const char * argv[])
{
    const std::vector<double> samples {-1, 0, 1};
    const auto filename = "out.WAV";
    wav::SaveWAV (samples, wav::ExportFormat::FLOAT_PCM_64, 48000, 1, filename);
    LoadWaveFile (filename);
    return 0;
}
