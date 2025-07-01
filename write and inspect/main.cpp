
#include "../shared/Write.hpp"
#include "../shared/File.hpp"

#include <vector>
#include <string>

// 1. Write with wav::Write.
// 2. Inspect with wav::File.

void LoadWaveFile (std::string const & filepath)
{
    // load wave file
    wav::File file {filepath, true /*verbose*/};
    file.PrintChunks();
    std::cout << std::endl;
    file.PrintInfo();
    std::cout << std::endl;
    file.PrintSamples();
}

int main (int argc, const char * argv[])
{
    const std::vector<double> samples {-1, 0, 1};
    const auto filename = "out.WAV";
    wav::Write (samples, wav::ExportFormat::FLOAT_PCM_64, 48000, 1, filename);
    LoadWaveFile (filename);
    return 0;
}
