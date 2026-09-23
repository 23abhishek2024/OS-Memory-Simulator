#ifndef MFU_REPLACEMENT_HPP
#define MFU_REPLACEMENT_HPP

#include "PageReplacementAlgorithm.hpp"
#include <unordered_map>
#include <vector>

namespace vmem {

class MFUReplacement : public PageReplacementAlgorithm {
private:
    struct MFUEntry {
        PageNumber page;
        FrameNumber frame;
        uint32_t freq;
        uint64_t lastAccessTime;
        bool dirty;
    };

    std::unordered_map<PageNumber, MFUEntry> residentMap;
    std::vector<int> frames;
    std::vector<bool> frameDirty;
    uint64_t accessCounter;

public:
    explicit MFUReplacement(size_t frameCount);
    ~MFUReplacement() override = default;

    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "MFU"; }
    std::string getDescription() const override {
        return "Most Frequently Used replaces the page with the highest reference frequency count.";
    }
    std::vector<int> getFramesState() const override { return frames; }
};

} // namespace vmem

#endif // MFU_REPLACEMENT_HPP
