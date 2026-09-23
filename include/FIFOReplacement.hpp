#ifndef FIFO_REPLACEMENT_HPP
#define FIFO_REPLACEMENT_HPP

#include "PageReplacementAlgorithm.hpp"
#include "DoublyLinkedList.hpp"
#include <unordered_map>
#include <vector>

namespace vmem {

class FIFOReplacement : public PageReplacementAlgorithm {
private:
    struct FIFOEntry {
        PageNumber page;
        FrameNumber frame;
        bool dirty;
    };

    DoublyLinkedList<FIFOEntry> orderList;
    std::unordered_map<PageNumber, DoublyLinkedList<FIFOEntry>::Node*> pageMap;
    std::vector<int> frames; // Index represents frame number, value is pageNumber (-1 if empty)
    std::vector<bool> frameDirty;

public:
    explicit FIFOReplacement(size_t frameCount);
    ~FIFOReplacement() override = default;

    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "FIFO"; }
    std::string getDescription() const override {
        return "First-In, First-Out replaces the page that has been in memory the longest.";
    }
    std::vector<int> getFramesState() const override { return frames; }
};

} // namespace vmem

#endif // FIFO_REPLACEMENT_HPP
