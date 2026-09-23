#include "BenchmarkRunner.hpp"
#include "FIFOReplacement.hpp"
#include "LRUReplacement.hpp"
#include "LFUReplacement.hpp"
#include "MFUReplacement.hpp"
#include "OptimalReplacement.hpp"
#include "ClockReplacement.hpp"
#include "BeladyAnomalyDetector.hpp"
#include <fstream>
#include <iomanip>
#include <random>

namespace vmem {

std::vector<MemoryReference> BenchmarkRunner::generateWorkload(WorkloadType type,
                                                              size_t length,
                                                              size_t uniquePages,
                                                              unsigned int seed) {
    std::mt19937 rng(seed);
    std::vector<MemoryReference> refs;
    refs.reserve(length);

    switch (type) {
        case WorkloadType::LOCALITY_OF_REFERENCE: {
            // 80/20 Rule: 80% accesses to top 20% pages
            size_t hotPageCount = std::max<size_t>(1, uniquePages / 5);
            std::uniform_int_distribution<PageNumber> hotDist(1, static_cast<PageNumber>(hotPageCount));
            std::uniform_int_distribution<PageNumber> coldDist(static_cast<PageNumber>(hotPageCount + 1),
                                                               static_cast<PageNumber>(uniquePages));
            std::uniform_int_distribution<int> probDist(1, 100);
            std::uniform_int_distribution<int> writeDist(1, 10); // 30% writes

            for (size_t i = 0; i < length; ++i) {
                PageNumber p = (probDist(rng) <= 80) ? hotDist(rng) : coldDist(rng);
                AccessType t = (writeDist(rng) <= 3) ? AccessType::WRITE : AccessType::READ;
                refs.emplace_back(p, t);
            }
            break;
        }

        case WorkloadType::CYCLIC_LOOP: {
            // Sequential loop over uniquePages (e.g. 1, 2, ..., N, 1, 2, ..., N)
            std::uniform_int_distribution<int> writeDist(1, 10);
            for (size_t i = 0; i < length; ++i) {
                PageNumber p = static_cast<PageNumber>((i % uniquePages) + 1);
                AccessType t = (writeDist(rng) <= 2) ? AccessType::WRITE : AccessType::READ;
                refs.emplace_back(p, t);
            }
            break;
        }

        case WorkloadType::RANDOM_UNIFORM: {
            std::uniform_int_distribution<PageNumber> dist(1, static_cast<PageNumber>(uniquePages));
            std::uniform_int_distribution<int> writeDist(1, 10);
            for (size_t i = 0; i < length; ++i) {
                PageNumber p = dist(rng);
                AccessType t = (writeDist(rng) <= 3) ? AccessType::WRITE : AccessType::READ;
                refs.emplace_back(p, t);
            }
            break;
        }

        case WorkloadType::BELADY_CANONICAL: {
            auto canonical = BeladyAnomalyDetector::getCanonicalReferenceString();
            while (refs.size() < length) {
                for (const auto& r : canonical) {
                    if (refs.size() >= length) break;
                    refs.push_back(r);
                }
            }
            break;
        }

        default:
            break;
    }

    return refs;
}

BenchmarkSuiteResult BenchmarkRunner::runSuite(const std::string& workloadName,
                                              const std::string& workloadDesc,
                                              const std::vector<MemoryReference>& references,
                                              size_t frameCount) {
    BenchmarkSuiteResult suite;
    suite.workloadName = workloadName;
    suite.workloadDescription = workloadDesc;
    suite.frameCount = frameCount;

    std::vector<std::unique_ptr<PageReplacementAlgorithm>> algorithms;
    algorithms.emplace_back(new FIFOReplacement(frameCount));
    algorithms.emplace_back(new LRUReplacement(frameCount));
    algorithms.emplace_back(new LFUReplacement(frameCount));
    algorithms.emplace_back(new MFUReplacement(frameCount));
    algorithms.emplace_back(new OptimalReplacement(frameCount));
    algorithms.emplace_back(new ClockReplacement(frameCount));

    for (auto& algo : algorithms) {
        algo->runSimulation(references);
        suite.algorithmResults.push_back(algo->getMetrics());
    }

    return suite;
}

bool BenchmarkRunner::exportResultsToCSV(const std::vector<BenchmarkSuiteResult>& suites,
                                         const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << "Workload,Algorithm,Frames,TotalReferences,PageHits,PageFaults,HitRatioPercent,FaultRatioPercent,Evictions,DirtyWritebacks,EAT_Nanoseconds\n";

    for (const auto& suite : suites) {
        for (const auto& m : suite.algorithmResults) {
            out << "\"" << suite.workloadName << "\","
                << "\"" << m.algorithmName << "\","
                << m.frameCount << ","
                << m.totalReferences << ","
                << m.pageHits << ","
                << m.pageFaults << ","
                << std::fixed << std::setprecision(2) << m.hitRatio << ","
                << std::fixed << std::setprecision(2) << m.faultRatio << ","
                << m.evictions << ","
                << m.dirtyWritebacks << ","
                << std::fixed << std::setprecision(2) << m.effectiveAccessTimeNs << "\n";
        }
    }
    return true;
}

bool BenchmarkRunner::exportResultsToJSON(const std::vector<BenchmarkSuiteResult>& suites,
                                          const std::string& filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) return false;

    out << "[\n";
    for (size_t s = 0; s < suites.size(); ++s) {
        const auto& suite = suites[s];
        out << "  {\n";
        out << "    \"workload\": \"" << suite.workloadName << "\",\n";
        out << "    \"description\": \"" << suite.workloadDescription << "\",\n";
        out << "    \"frameCount\": " << suite.frameCount << ",\n";
        out << "    \"results\": [\n";

        for (size_t a = 0; a < suite.algorithmResults.size(); ++a) {
            const auto& m = suite.algorithmResults[a];
            out << "      {\n";
            out << "        \"algorithm\": \"" << m.algorithmName << "\",\n";
            out << "        \"references\": " << m.totalReferences << ",\n";
            out << "        \"hits\": " << m.pageHits << ",\n";
            out << "        \"faults\": " << m.pageFaults << ",\n";
            out << "        \"hitRatio\": " << std::fixed << std::setprecision(2) << m.hitRatio << ",\n";
            out << "        \"faultRatio\": " << std::fixed << std::setprecision(2) << m.faultRatio << ",\n";
            out << "        \"evictions\": " << m.evictions << ",\n";
            out << "        \"dirtyWritebacks\": " << m.dirtyWritebacks << ",\n";
            out << "        \"effectiveAccessTimeNs\": " << std::fixed << std::setprecision(2) << m.effectiveAccessTimeNs << "\n";
            out << "      }" << (a + 1 < suite.algorithmResults.size() ? "," : "") << "\n";
        }

        out << "    ]\n";
        out << "  }" << (s + 1 < suites.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return true;
}

} // namespace vmem
