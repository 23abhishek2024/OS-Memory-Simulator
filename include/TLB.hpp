#ifndef TLB_HPP
#define TLB_HPP

#include "MemoryTypes.hpp"
#include <vector>
#include <cstddef>

namespace vmem {

class TLB {
private:
    size_t capacity;
    std::vector<TLBEntry> entries;
    uint64_t timer;
    size_t hitCount;
    size_t missCount;

public:
    explicit TLB(size_t tlbSize = 8);

    bool lookup(PageNumber vpn, FrameNumber& outPfn);
    void insert(PageNumber vpn, FrameNumber pfn, bool isDirty = false);
    void invalidate(PageNumber vpn);
    void markDirty(PageNumber vpn);
    void flush();

    size_t getHitCount() const { return hitCount; }
    size_t getMissCount() const { return missCount; }
    double getHitRate() const {
        size_t total = hitCount + missCount;
        return (total > 0) ? (static_cast<double>(hitCount) / total) * 100.0 : 0.0;
    }
    const std::vector<TLBEntry>& getEntries() const { return entries; }
};

} // namespace vmem

#endif // TLB_HPP
