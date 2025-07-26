// FullDataParamsLogger.cpp
#include "FullParamsDataLogger.h"
#include <fstream>
#include <filesystem>

namespace FullParamsDataLogger {

    void log(
        const std::string& csvPath,
        const std::string& modeName,
        int hnswM,
        int hnswEfConstruction,
        int hnswEfSearch,
        size_t totalSampleCount)
    {
        bool exists = std::filesystem::exists(csvPath);
        std::ofstream out(csvPath, exists ? std::ios::app : std::ios::out);
        // write header if new
        if (!exists) {
            out << "RenderMode,M,EfConstruction,EfSearch,TotalSamples\n";
        }
        out
            << modeName << ","
            << hnswM << ","
            << hnswEfConstruction << ","
            << hnswEfSearch << ","
            << totalSampleCount << "\n";
    }

} // namespace VolumeBench