#ifndef LRU_REPLACEMENT_HPP
#define LRU_REPLACEMENT_HPP

#include "PageReplacementAlgorithm.hpp"
#include "DoublyLinkedList.hpp"
#include <unordered_map>
#include <vector>

namespace vmem {

class LRUReplacement : public PageReplacementAlgorithm {
private:
    struct LRUEntry {
        PageNumber page;
        FrameNumber frame;
        bool dirty;
    };

    DoublyLinkedList<LRUEntry> orderList;
    std::unordered_map<PageNumber, DoublyLinkedList<LRUEntry>::Node*> pageMap;
    std::vector<int> frames; // index = frame, value = page (-1 if free)
    std::vector<bool> frameDirty;

public:
    explicit LRUReplacement(size_t frameCount);
    ~LRUReplacement() override = default;

    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "LRU"; }
    std::string getDescription() const override {
        return "Least Recently Used replaces the page that has not been referenced for the longest time.";
    }
    std::vector<int> getFramesState() const override { return frames; }
};

} // namespace vmem

#endif // LRU_REPLACEMENT_HPP
