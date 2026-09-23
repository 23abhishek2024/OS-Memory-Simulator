#ifndef OPTIMAL_REPLACEMENT_HPP
#define OPTIMAL_REPLACEMENT_HPP

#include "PageReplacementAlgorithm.hpp"
#include <unordered_map>
#include <vector>

namespace vmem {

class OptimalReplacement : public PageReplacementAlgorithm {
private:
    std::unordered_map<PageNumber, FrameNumber> residentMap;
    std::vector<int> frames;
    std::vector<bool> frameDirty;

    // Helper to find next occurrence index of a page in futureReferences starting from currentIndex + 1
    size_t findNextUse(PageNumber page, size_t fromIndex) const;

public:
    explicit OptimalReplacement(size_t frameCount);
    ~OptimalReplacement() override = default;

    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "Optimal (OPT)"; }
    std::string getDescription() const override {
        return "Bélády's Optimal replaces the page that will not be used for the longest period in the future.";
    }
    std::vector<int> getFramesState() const override { return frames; }
};

} // namespace vmem

#endif // OPTIMAL_REPLACEMENT_HPP
