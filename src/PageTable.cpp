#include "PageTable.hpp"

namespace vmem {

PageTable::PageTable(size_t virtualPageCount) : totalVirtualPages(virtualPageCount) {}

bool PageTable::isPresent(PageNumber vpn) const {
    auto it = table.find(vpn);
    return (it != table.end() && it->second.valid);
}

const PageTableEntry* PageTable::getEntry(PageNumber vpn) const {
    auto it = table.find(vpn);
    if (it != table.end()) {
        return &(it->second);
    }
    return nullptr;
}

PageTableEntry* PageTable::getEntryMutable(PageNumber vpn) {
    return &table[vpn];
}

void PageTable::mapPage(PageNumber vpn, FrameNumber pfn, bool isWrite) {
    PageTableEntry& entry = table[vpn];
    entry.frameNumber = pfn;
    entry.valid = true;
    entry.referenced = true;
    if (isWrite) {
        entry.dirty = true;
    }
    ++entry.accessCount;
}

bool PageTable::unmapPage(PageNumber vpn, bool& outWasDirty) {
    auto it = table.find(vpn);
    if (it != table.end() && it->second.valid) {
        outWasDirty = it->second.dirty;
        it->second.valid = false;
        it->second.dirty = false;
        it->second.referenced = false;
        return true;
    }
    outWasDirty = false;
    return false;
}

void PageTable::markDirty(PageNumber vpn) {
    auto it = table.find(vpn);
    if (it != table.end()) {
        it->second.dirty = true;
        it->second.referenced = true;
    }
}

void PageTable::setReferenced(PageNumber vpn, bool ref) {
    auto it = table.find(vpn);
    if (it != table.end()) {
        it->second.referenced = ref;
    }
}

void PageTable::reset() {
    table.clear();
}

size_t PageTable::getPresentPageCount() const {
    size_t count = 0;
    for (const auto& kv : table) {
        if (kv.second.valid) {
            ++count;
        }
    }
    return count;
}

} // namespace vmem
