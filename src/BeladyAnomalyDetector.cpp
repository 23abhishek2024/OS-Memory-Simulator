#include "BeladyAnomalyDetector.hpp"
#include "FIFOReplacement.hpp"
#include "LRUReplacement.hpp"
#include "OptimalReplacement.hpp"
#include "LFUReplacement.hpp"
#include "MFUReplacement.hpp"
#include "ClockReplacement.hpp"
#include <random>
#include <memory>

namespace vmem {

std::vector<MemoryReference> BeladyAnomalyDetector::getCanonicalReferenceString() {
    // Canonical 12-reference string: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5
    // Frames 3 -> 9 faults
    // Frames 4 -> 10 faults (Bélády's Anomaly!)
    std::vector<PageNumber> pages = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    std::vector<MemoryReference> refs;
    for (auto p : pages) {
        refs.emplace_back(p, AccessType::READ);
    }
    return refs;
}

std::vector<MemoryReference> BeladyAnomalyDetector::getAlternativeReferenceString() {
    // Alternative string: 3, 2, 1, 0, 3, 2, 4, 3, 2, 1, 0, 4
    std::vector<PageNumber> pages = {3, 2, 1, 0, 3, 2, 4, 3, 2, 1, 0, 4};
    std::vector<MemoryReference> refs;
    for (auto p : pages) {
        refs.emplace_back(p, AccessType::READ);
    }
    return refs;
}

std::vector<MemoryReference> BeladyAnomalyDetector::generateRandomString(size_t length,
                                                                         size_t uniquePages,
                                                                         unsigned int seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<PageNumber> dist(1, static_cast<PageNumber>(uniquePages));
    std::uniform_int_distribution<int> typeDist(0, 4); // 20% writes

    std::vector<MemoryReference> refs;
    refs.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        AccessType t = (typeDist(rng) == 0) ? AccessType::WRITE : AccessType::READ;
        refs.emplace_back(dist(rng), t);
    }
    return refs;
}

AnomalyResult BeladyAnomalyDetector::analyze(const std::vector<MemoryReference>& references,
                                             const std::string& algorithmType,
                                             size_t minFrames,
                                             size_t maxFrames) {
    AnomalyResult result;
    result.algorithmName = algorithmType;
    result.referenceString = references;
    result.anomalyDetected = false;

    for (size_t f = minFrames; f <= maxFrames; ++f) {
        std::unique_ptr<PageReplacementAlgorithm> algo;
        if (algorithmType == "FIFO") {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new FIFOReplacement(f));
        } else if (algorithmType == "LRU") {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new LRUReplacement(f));
        } else if (algorithmType == "Optimal" || algorithmType == "OPT") {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new OptimalReplacement(f));
        } else if (algorithmType == "LFU") {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new LFUReplacement(f));
        } else if (algorithmType == "MFU") {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new MFUReplacement(f));
        } else if (algorithmType == "Clock") {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new ClockReplacement(f));
        } else {
            algo = std::unique_ptr<PageReplacementAlgorithm>(new FIFOReplacement(f));
        }

        algo->runSimulation(references);
        size_t faults = algo->getMetrics().pageFaults;
        result.frameFaultPairs.emplace_back(f, faults);
    }

    // Check for anomalies where faults(F_k+1) > faults(F_k)
    for (size_t i = 1; i < result.frameFaultPairs.size(); ++i) {
        size_t prevFrames = result.frameFaultPairs[i - 1].first;
        size_t prevFaults = result.frameFaultPairs[i - 1].second;
        size_t currFrames = result.frameFaultPairs[i].first;
        size_t currFaults = result.frameFaultPairs[i].second;

        if (currFaults > prevFaults) {
            result.anomalyDetected = true;
            result.occurrences.push_back({
                prevFrames,
                currFrames,
                prevFaults,
                currFaults,
                currFaults - prevFaults
            });
        }
    }

    return result;
}

} // namespace vmem
