#ifndef BENCHMARK_RUNNER_HPP
#define BENCHMARK_RUNNER_HPP

#include "MemoryTypes.hpp"
#include "PageReplacementAlgorithm.hpp"
#include <vector>
#include <string>
#include <memory>

namespace vmem {

enum class WorkloadType {
    LOCALITY_OF_REFERENCE,
    CYCLIC_LOOP,
    RANDOM_UNIFORM,
    BELADY_CANONICAL,
    CUSTOM
};

struct BenchmarkSuiteResult {
    std::string workloadName;
    std::string workloadDescription;
    size_t frameCount;
    std::vector<SimulationMetrics> algorithmResults;
};

class BenchmarkRunner {
public:
    static std::vector<MemoryReference> generateWorkload(WorkloadType type,
                                                        size_t length = 100,
                                                        size_t uniquePages = 15,
                                                        unsigned int seed = 12345);

    static BenchmarkSuiteResult runSuite(const std::string& workloadName,
                                        const std::string& workloadDesc,
                                        const std::vector<MemoryReference>& references,
                                        size_t frameCount);

    static bool exportResultsToCSV(const std::vector<BenchmarkSuiteResult>& suites,
                                   const std::string& filePath);

    static bool exportResultsToJSON(const std::vector<BenchmarkSuiteResult>& suites,
                                    const std::string& filePath);
};

} // namespace vmem

#endif // BENCHMARK_RUNNER_HPP
