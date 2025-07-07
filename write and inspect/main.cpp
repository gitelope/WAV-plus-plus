
#include "../shared/Write.hpp"
#include "../shared/ChunkInfo.hpp"

// 1. Write WAV file. (wav::Write)
// 2. Display detailed file info. (wav::ChunkInfo)
int main (int argc, const char * argv[])
{
    const auto filename {"out.WAV"};
    const std::vector<double> samples {-1, 0, 1};
    wav::Write (samples, wav::ExportFormat::FLOAT_PCM_32, 48000, 1, filename);

    wav::ChunkInfo info {filename};
    std::cout << '\n' << filename << "\n\n" << info << "\n" << info.summary() << std::endl;

    return 0;
}
