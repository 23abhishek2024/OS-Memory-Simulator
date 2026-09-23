#ifndef PAGE_TABLE_HPP
#define PAGE_TABLE_HPP

#include "MemoryTypes.hpp"
#include <unordered_map>
#include <vector>
#include <iostream>

namespace vmem {

class PageTable {
private:
    size_t totalVirtualPages;
    std::unordered_map<PageNumber, PageTableEntry> table;

public:
    explicit PageTable(size_t virtualPageCount = 1024);

    bool isPresent(PageNumber vpn) const;
    const PageTableEntry* getEntry(PageNumber vpn) const;
    PageTableEntry* getEntryMutable(PageNumber vpn);

    void mapPage(PageNumber vpn, FrameNumber pfn, bool isWrite = false);
    bool unmapPage(PageNumber vpn, bool& outWasDirty);

    void markDirty(PageNumber vpn);
    void setReferenced(PageNumber vpn, bool ref = true);

    void reset();

    size_t getPresentPageCount() const;
    const std::unordered_map<PageNumber, PageTableEntry>& getEntries() const {
        return table;
    }
};

} // namespace vmem

#endif // PAGE_TABLE_HPP
