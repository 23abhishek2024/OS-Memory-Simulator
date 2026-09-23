#ifndef PAGE_REPLACEMENT_ALGORITHM_HPP
#define PAGE_REPLACEMENT_ALGORITHM_HPP

#include "MemoryTypes.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vmem {

class PageReplacementAlgorithm {
protected:
    size_t frameCapacity;
    SimulationMetrics metrics;
    std::vector<MemoryReference> futureReferences; // For Optimal lookahead
    size_t currentReferenceIndex;

public:
    explicit PageReplacementAlgorithm(size_t frames, const std::string& name)
        : frameCapacity(frames), currentReferenceIndex(0) {
        metrics.algorithmName = name;
        metrics.frameCount = frames;
    }

    virtual ~PageReplacementAlgorithm() = default;

    virtual AccessResult access(PageNumber page, AccessType type = AccessType::READ) = 0;
    virtual void reset() = 0;
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::vector<int> getFramesState() const = 0;

    size_t getFrameCapacity() const { return frameCapacity; }

    void setFrameCapacity(size_t frames) {
        frameCapacity = frames;
        metrics.frameCount = frames;
        reset();
    }

    void setFutureReferences(const std::vector<MemoryReference>& refs) {
        futureReferences = refs;
        currentReferenceIndex = 0;
    }

    const SimulationMetrics& getMetrics() const {
        return metrics;
    }

    SimulationMetrics& getMetricsMutable() {
        return metrics;
    }

    virtual void runSimulation(const std::vector<MemoryReference>& references,
                               std::vector<AccessResult>* traceLog = nullptr) {
        reset();
        setFutureReferences(references);

        for (size_t i = 0; i < references.size(); ++i) {
            currentReferenceIndex = i;
            AccessResult res = access(references[i].page, references[i].type);
            if (traceLog) {
                traceLog->push_back(res);
            }
        }
        metrics.calculate();
    }
};

} // namespace vmem

#endif // PAGE_REPLACEMENT_ALGORITHM_HPP
