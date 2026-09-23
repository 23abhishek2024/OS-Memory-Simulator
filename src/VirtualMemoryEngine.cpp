#include "VirtualMemoryEngine.hpp"
#include <iomanip>

namespace vmem {

VirtualMemoryEngine::VirtualMemoryEngine(size_t frameCount,
                                         std::unique_ptr<PageReplacementAlgorithm> algorithm,
                                         uint32_t pageSizeBytes,
                                         size_t tlbSize)
    : pageSize(pageSizeBytes),
      physicalFrameCount(frameCount),
      replacementPolicy(std::move(algorithm)),
      pageTable(1024),
      tlb(tlbSize) {
    reset();
}

void VirtualMemoryEngine::setAlgorithm(std::unique_ptr<PageReplacementAlgorithm> algorithm) {
    replacementPolicy = std::move(algorithm);
    reset();
}

void VirtualMemoryEngine::reset() {
    if (replacementPolicy) {
        replacementPolicy->reset();
    }
    pageTable.reset();
    tlb.flush();
    history.clear();
    metrics = SimulationMetrics();
    metrics.frameCount = physicalFrameCount;
    if (replacementPolicy) {
        metrics.algorithmName = replacementPolicy->getName();
    }
}

TranslationTrace VirtualMemoryEngine::accessMemory(VirtualAddress vaddr, AccessType type) {
    TranslationTrace trace;
    trace.virtualAddress = vaddr;
    trace.vpn = vaddr / pageSize;
    trace.offset = vaddr % pageSize;
    trace.accessType = type;
    trace.tlbHit = false;
    trace.pageTableHit = false;
    trace.pageFault = false;
    trace.victimEvicted = false;
    trace.victimVpn = 0;
    trace.dirtyWriteback = false;
    trace.accessTimeNs = 0.0;

    const double TLB_LOOKUP_NS = 10.0;
    const double RAM_ACCESS_NS = 100.0;
    const double PAGE_FAULT_NS = 8000000.0;   // 8 ms
    const double WRITEBACK_NS = 12000000.0;   // 12 ms

    ++metrics.totalReferences;

    FrameNumber pfn = 0;
    trace.accessTimeNs += TLB_LOOKUP_NS;

    // Step 1: Check TLB
    if (tlb.lookup(trace.vpn, pfn)) {
        trace.tlbHit = true;
        trace.pfn = pfn;
        trace.accessTimeNs += RAM_ACCESS_NS;
        ++metrics.tlbHits;
        ++metrics.pageHits;

        // Also inform the replacement algorithm of the page hit (to refresh LRU/Clock etc.)
        if (replacementPolicy) {
            replacementPolicy->access(trace.vpn, type);
        }
    } else {
        // TLB Miss
        ++metrics.tlbMisses;
        trace.tlbHit = false;
        trace.accessTimeNs += RAM_ACCESS_NS; // Page table lookup overhead

        // Step 2: Check Page Table
        if (pageTable.isPresent(trace.vpn)) {
            trace.pageTableHit = true;
            ++metrics.pageHits;

            const auto* pte = pageTable.getEntry(trace.vpn);
            pfn = pte->frameNumber;
            trace.pfn = pfn;

            // Refresh replacement policy and TLB
            if (replacementPolicy) {
                replacementPolicy->access(trace.vpn, type);
            }
            tlb.insert(trace.vpn, pfn, (type == AccessType::WRITE));
            trace.accessTimeNs += RAM_ACCESS_NS;
        } else {
            // Step 3: Page Fault Interrupt!
            trace.pageFault = true;
            ++metrics.pageFaults;
            trace.accessTimeNs += PAGE_FAULT_NS;

            // Delegate frame allocation and replacement to policy
            AccessResult algoResult = replacementPolicy->access(trace.vpn, type);
            pfn = algoResult.allocatedFrame;
            trace.pfn = pfn;

            if (algoResult.victimEvicted) {
                trace.victimEvicted = true;
                trace.victimVpn = algoResult.victimPage;
                ++metrics.evictions;

                // Invalidate victim in TLB & unmap from PageTable
                tlb.invalidate(algoResult.victimPage);
                bool wasDirty = false;
                pageTable.unmapPage(algoResult.victimPage, wasDirty);

                if (algoResult.dirtyWriteback || wasDirty) {
                    trace.dirtyWriteback = true;
                    ++metrics.dirtyWritebacks;
                    trace.accessTimeNs += WRITEBACK_NS;
                }
            }

            // Map new page into Page Table and TLB
            pageTable.mapPage(trace.vpn, pfn, (type == AccessType::WRITE));
            tlb.insert(trace.vpn, pfn, (type == AccessType::WRITE));
        }
    }

    if (type == AccessType::WRITE) {
        pageTable.markDirty(trace.vpn);
        tlb.markDirty(trace.vpn);
    }

    trace.physicalAddress = (pfn * pageSize) + trace.offset;
    history.push_back(trace);
    metrics.calculate();
    return trace;
}

std::vector<int> VirtualMemoryEngine::getCurrentFrames() const {
    if (replacementPolicy) {
        return replacementPolicy->getFramesState();
    }
    return std::vector<int>(physicalFrameCount, -1);
}

} // namespace vmem
