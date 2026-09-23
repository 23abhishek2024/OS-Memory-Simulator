#include "TLB.hpp"
#include <limits>

namespace vmem {

TLB::TLB(size_t tlbSize)
    : capacity(tlbSize),
      entries(tlbSize),
      timer(0),
      hitCount(0),
      missCount(0) {
    flush();
}

bool TLB::lookup(PageNumber vpn, FrameNumber& outPfn) {
    ++timer;
    for (auto& entry : entries) {
        if (entry.valid && entry.pageNumber == vpn) {
            entry.lastUsed = timer;
            outPfn = entry.frameNumber;
            ++hitCount;
            return true;
        }
    }
    ++missCount;
    return false;
}

void TLB::insert(PageNumber vpn, FrameNumber pfn, bool isDirty) {
    ++timer;

    // Check if entry already exists (update it)
    for (auto& entry : entries) {
        if (entry.valid && entry.pageNumber == vpn) {
            entry.frameNumber = pfn;
            entry.dirty = (entry.dirty || isDirty);
            entry.lastUsed = timer;
            return;
        }
    }

    // Check for an unused/invalid slot
    for (auto& entry : entries) {
        if (!entry.valid) {
            entry.pageNumber = vpn;
            entry.frameNumber = pfn;
            entry.valid = true;
            entry.dirty = isDirty;
            entry.lastUsed = timer;
            return;
        }
    }

    // TLB Full: Evict Least Recently Used TLB entry
    size_t lruIndex = 0;
    uint64_t oldest = std::numeric_limits<uint64_t>::max();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].lastUsed < oldest) {
            oldest = entries[i].lastUsed;
            lruIndex = i;
        }
    }

    entries[lruIndex].pageNumber = vpn;
    entries[lruIndex].frameNumber = pfn;
    entries[lruIndex].valid = true;
    entries[lruIndex].dirty = isDirty;
    entries[lruIndex].lastUsed = timer;
}

void TLB::invalidate(PageNumber vpn) {
    for (auto& entry : entries) {
        if (entry.valid && entry.pageNumber == vpn) {
            entry.valid = false;
            entry.pageNumber = 0;
            entry.frameNumber = 0;
            entry.dirty = false;
            return;
        }
    }
}

void TLB::markDirty(PageNumber vpn) {
    for (auto& entry : entries) {
        if (entry.valid && entry.pageNumber == vpn) {
            entry.dirty = true;
            return;
        }
    }
}

void TLB::flush() {
    for (auto& entry : entries) {
        entry.pageNumber = 0;
        entry.frameNumber = 0;
        entry.valid = false;
        entry.dirty = false;
        entry.lastUsed = 0;
    }
    timer = 0;
    hitCount = 0;
    missCount = 0;
}

} // namespace vmem
