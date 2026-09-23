#ifndef CLOCK_REPLACEMENT_HPP
#define CLOCK_REPLACEMENT_HPP

#include "PageReplacementAlgorithm.hpp"
#include <unordered_map>
#include <vector>

namespace vmem {

class ClockReplacement : public PageReplacementAlgorithm {
private:
    struct ClockEntry {
        PageNumber page = 0;
        bool referenceBit = false;
        bool dirty = false;
        bool occupied = false;
    };

    std::vector<ClockEntry> clockBuffer;
    size_t clockHand;
    std::unordered_map<PageNumber, size_t> pageToIndex;

public:
    explicit ClockReplacement(size_t frameCount);
    ~ClockReplacement() override = default;

    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "Clock (Second Chance)"; }
    std::string getDescription() const override {
        return "Clock algorithm approximates LRU using a circular frame buffer with reference bits and a sweeping hand.";
    }
    std::vector<int> getFramesState() const override;
};

} // namespace vmem

#endif // CLOCK_REPLACEMENT_HPP
