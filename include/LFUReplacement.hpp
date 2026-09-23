#ifndef LFU_REPLACEMENT_HPP
#define LFU_REPLACEMENT_HPP

#include "PageReplacementAlgorithm.hpp"
#include "DoublyLinkedList.hpp"
#include <unordered_map>
#include <vector>

namespace vmem {

class LFUReplacement : public PageReplacementAlgorithm {
private:
    struct LFUEntry {
        PageNumber page;
        FrameNumber frame;
        uint32_t freq;
        bool dirty;
    };

    // Node pointer in frequency list
    using PageNode = DoublyLinkedList<LFUEntry>::Node*;

    std::unordered_map<PageNumber, PageNode> pageMap;
    // Map frequency to DoublyLinkedList of entries (each list preserves LRU order among same frequency)
    std::unordered_map<uint32_t, DoublyLinkedList<LFUEntry>> freqBuckets;
    uint32_t minFrequency;

    std::vector<int> frames;
    std::vector<bool> frameDirty;

public:
    explicit LFUReplacement(size_t frameCount);
    ~LFUReplacement() override = default;

    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "LFU"; }
    std::string getDescription() const override {
        return "Least Frequently Used replaces the page with the lowest reference frequency count (LRU tie-break).";
    }
    std::vector<int> getFramesState() const override { return frames; }
};

} // namespace vmem

#endif // LFU_REPLACEMENT_HPP
