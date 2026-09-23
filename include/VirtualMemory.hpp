#ifndef VIRTUAL_MEMORY_HPP
#define VIRTUAL_MEMORY_HPP

#include "PageReplacement.hpp"
#include <vector>
#include <unordered_map>
#include <string>

namespace vmem {

using VirtualAddress = uint32_t;
using PhysicalAddress = uint32_t;

struct PageTableEntry {
    FrameNumber frameNumber = 0;
    bool valid = false;
    bool dirty = false;
    bool referenced = false;
};

struct TLBEntry {
    PageNumber vpn = 0;
    FrameNumber pfn = 0;
    bool valid = false;
    bool dirty = false;
    uint64_t lastUsed = 0;
};

struct TranslationTrace {
    VirtualAddress vaddr;
    PageNumber vpn;
    uint32_t offset;
    PhysicalAddress paddr;
    FrameNumber pfn;
    AccessType type;
    bool tlbHit;
    bool pageFault;
    bool victimEvicted;
    PageNumber victimVpn;
    bool dirtyWriteback;
    double latencyNs;
};

class PageTable {
private:
    std::unordered_map<PageNumber, PageTableEntry> table;
public:
    bool isPresent(PageNumber vpn) const;
    const PageTableEntry* getEntry(PageNumber vpn) const;
    void mapPage(PageNumber vpn, FrameNumber pfn, bool isWrite);
    bool unmapPage(PageNumber vpn, bool& wasDirty);
    void markDirty(PageNumber vpn);
    void reset();
};

class TLB {
private:
    size_t capacity;
    std::vector<TLBEntry> entries;
    uint64_t timer;
public:
    explicit TLB(size_t size = 4);
    bool lookup(PageNumber vpn, FrameNumber& outPfn);
    void insert(PageNumber vpn, FrameNumber pfn, bool isDirty);
    void invalidate(PageNumber vpn);
    void markDirty(PageNumber vpn);
    void flush();
};

class VirtualMemoryEngine {
private:
    uint32_t pageSize; // 4096 bytes (4 KB)
    size_t frameCount;
    std::unique_ptr<PageReplacementAlgorithm> policy;
    PageTable pageTable;
    TLB tlb;
    std::vector<TranslationTrace> history;
    size_t hits;
    size_t faults;

public:
    VirtualMemoryEngine(size_t frames, std::unique_ptr<PageReplacementAlgorithm> algo, uint32_t pgSize = 4096);
    TranslationTrace accessMemory(VirtualAddress vaddr, AccessType type = AccessType::READ);
    const std::vector<TranslationTrace>& getHistory() const { return history; }
    void reset();
};

struct BeladyResult {
    std::vector<std::pair<size_t, size_t>> fifoFaults; // {frames, faults}
    std::vector<std::pair<size_t, size_t>> lruFaults;
    std::vector<std::pair<size_t, size_t>> optFaults;
    bool anomalyDetected = false;
    size_t anomalyFromFrames = 0;
    size_t anomalyToFrames = 0;
    size_t anomalyFromFaults = 0;
    size_t anomalyToFaults = 0;
};

class BeladyAnomalyDetector {
public:
    static BeladyResult verify(const std::vector<MemoryReference>& refs, size_t minF = 2, size_t maxF = 6);
    static std::vector<MemoryReference> getCanonicalString();
};

} // namespace vmem

#endif // VIRTUAL_MEMORY_HPP
