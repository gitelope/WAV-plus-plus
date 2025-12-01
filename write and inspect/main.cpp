
#include "../shared/Writer.hpp"
#include "../shared/ChunkInfo.hpp"

// 1. Write WAV file. (wav::Write)
// 2. Display detailed file info. (wav::ChunkInfo)
int main ()
{
    const auto filename {"out.WAV"};
    const std::vector<double> samples {-1, 0, 1};

    wav::Writer writer {filename, wav::ExportFormat::FloatPCM_32, 48000, 1};
    writer.write (samples);
    writer.Close(); // not necessary if Writer is destroyed before attempt to access file

    wav::ChunkInfo info {filename};
    std::cout << '\n' << filename << "\n\n" << info << "\n" << info.summary() << std::endl;

    return 0;
}
