#ifndef BELADY_ANOMALY_DETECTOR_HPP
#define BELADY_ANOMALY_DETECTOR_HPP

#include "MemoryTypes.hpp"
#include <vector>
#include <string>
#include <utility>

namespace vmem {

struct AnomalyOccurrence {
    size_t lowerFrames;
    size_t higherFrames;
    size_t lowerFaults;
    size_t higherFaults;
    size_t deltaFaults;
};

struct AnomalyResult {
    std::string algorithmName;
    std::vector<MemoryReference> referenceString;
    std::vector<std::pair<size_t, size_t>> frameFaultPairs; // {frameCount, pageFaults}
    bool anomalyDetected;
    std::vector<AnomalyOccurrence> occurrences;
};

class BeladyAnomalyDetector {
public:
    static AnomalyResult analyze(const std::vector<MemoryReference>& references,
                                 const std::string& algorithmType,
                                 size_t minFrames = 2,
                                 size_t maxFrames = 7);

    // Get classic canonical Bélády's anomaly string: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5
    static std::vector<MemoryReference> getCanonicalReferenceString();

    // Secondary classic string: 3, 2, 1, 0, 3, 2, 4, 3, 2, 1, 0, 4
    static std::vector<MemoryReference> getAlternativeReferenceString();

    // Mine a random reference string exhibiting Bélády's anomaly
    static std::vector<MemoryReference> generateRandomString(size_t length = 20, size_t uniquePages = 6, unsigned int seed = 42);
};

} // namespace vmem

#endif // BELADY_ANOMALY_DETECTOR_HPP
