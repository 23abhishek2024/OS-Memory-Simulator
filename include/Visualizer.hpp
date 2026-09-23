#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "MemoryTypes.hpp"
#include "BeladyAnomalyDetector.hpp"
#include "BenchmarkRunner.hpp"
#include "VirtualMemoryEngine.hpp"
#include <vector>
#include <string>

namespace vmem {

class Visualizer {
public:
    static void printHeader(const std::string& title);
    static void printSubHeader(const std::string& subTitle);

    static void printTraceTable(const std::string& algoName,
                                const std::vector<MemoryReference>& references,
                                const std::vector<AccessResult>& traceLog,
                                size_t frameCount);

    static void printBenchmarkTable(const BenchmarkSuiteResult& suite);

    static void printBeladyAnalysis(const AnomalyResult& fifoResult,
                                    const AnomalyResult& lruResult,
                                    const AnomalyResult& optResult);

    static void printTranslationTraceTable(const std::vector<TranslationTrace>& traces);

    static void printSummaryBox(const SimulationMetrics& metrics);
};

} // namespace vmem

#endif // VISUALIZER_HPP
