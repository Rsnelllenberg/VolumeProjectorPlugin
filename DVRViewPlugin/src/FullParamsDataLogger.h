#pragma once
#include <string>

namespace FullParamsDataLogger {
    /// Append one line to a CSV of “render‐mode, M, efConstruction, efSearch, totalSamples”
    void log(
        const std::string& csvPath,
        const std::string& modeName,
        int hnswM,
        int hnswEfConstruction,
        int hnswEfSearch,
        size_t totalSampleCount);
}