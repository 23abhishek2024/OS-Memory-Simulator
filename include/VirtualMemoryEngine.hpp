#ifndef VIRTUAL_MEMORY_ENGINE_HPP
#define VIRTUAL_MEMORY_ENGINE_HPP

#include "MemoryTypes.hpp"
#include "PageTable.hpp"
#include "TLB.hpp"
#include "PageReplacementAlgorithm.hpp"
#include <memory>
#include <vector>
#include <string>

namespace vmem {

struct TranslationTrace {
    VirtualAddress virtualAddress;
    PageNumber vpn;
    uint32_t offset;
    PhysicalAddress physicalAddress;
    FrameNumber pfn;
    AccessType accessType;
    bool tlbHit;
    bool pageTableHit;
    bool pageFault;
    bool victimEvicted;
    PageNumber victimVpn;
    bool dirtyWriteback;
    double accessTimeNs;
};

class VirtualMemoryEngine {
private:
    uint32_t pageSize; // Default 4096 bytes (4 KB)
    size_t physicalFrameCount;
    std::unique_ptr<PageReplacementAlgorithm> replacementPolicy;
    PageTable pageTable;
    TLB tlb;
    SimulationMetrics metrics;
    std::vector<TranslationTrace> history;

public:
    VirtualMemoryEngine(size_t frameCount,
                        std::unique_ptr<PageReplacementAlgorithm> algorithm,
                        uint32_t pageSizeBytes = 4096,
                        size_t tlbSize = 8);

    TranslationTrace accessMemory(VirtualAddress vaddr, AccessType type = AccessType::READ);

    void setAlgorithm(std::unique_ptr<PageReplacementAlgorithm> algorithm);
    void reset();

    const SimulationMetrics& getMetrics() const { return metrics; }
    const PageTable& getPageTable() const { return pageTable; }
    const TLB& getTLB() const { return tlb; }
    const std::vector<TranslationTrace>& getHistory() const { return history; }
    uint32_t getPageSize() const { return pageSize; }
    size_t getFrameCount() const { return physicalFrameCount; }
    std::vector<int> getCurrentFrames() const;
};

} // namespace vmem

#endif // VIRTUAL_MEMORY_ENGINE_HPP
