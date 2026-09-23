#include "VirtualMemory.hpp"
#include <limits>

namespace vmem {

// -------------------------------------------------------------
// PageTable Implementation
// -------------------------------------------------------------
bool PageTable::isPresent(PageNumber vpn) const {
    auto it = table.find(vpn);
    return (it != table.end() && it->second.valid);
}

const PageTableEntry* PageTable::getEntry(PageNumber vpn) const {
    auto it = table.find(vpn);
    return (it != table.end()) ? &it->second : nullptr;
}

void PageTable::mapPage(PageNumber vpn, FrameNumber pfn, bool isWrite) {
    auto& entry = table[vpn];
    entry.frameNumber = pfn;
    entry.valid = true;
    entry.referenced = true;
    if (isWrite) entry.dirty = true;
}

bool PageTable::unmapPage(PageNumber vpn, bool& wasDirty) {
    auto it = table.find(vpn);
    if (it != table.end() && it->second.valid) {
        wasDirty = it->second.dirty;
        it->second.valid = false;
        it->second.dirty = false;
        return true;
    }
    wasDirty = false;
    return false;
}

void PageTable::markDirty(PageNumber vpn) {
    auto it = table.find(vpn);
    if (it != table.end()) it->second.dirty = true;
}

void PageTable::reset() {
    table.clear();
}

// -------------------------------------------------------------
// TLB Implementation
// -------------------------------------------------------------
TLB::TLB(size_t size) : capacity(size), entries(size), timer(0) {
    flush();
}

bool TLB::lookup(PageNumber vpn, FrameNumber& outPfn) {
    ++timer;
    for (auto& e : entries) {
        if (e.valid && e.vpn == vpn) {
            e.lastUsed = timer;
            outPfn = e.pfn;
            return true;
        }
    }
    return false;
}

void TLB::insert(PageNumber vpn, FrameNumber pfn, bool isDirty) {
    ++timer;
    for (auto& e : entries) {
        if (e.valid && e.vpn == vpn) {
            e.pfn = pfn; e.dirty = e.dirty || isDirty; e.lastUsed = timer;
            return;
        }
    }
    for (auto& e : entries) {
        if (!e.valid) {
            e.vpn = vpn; e.pfn = pfn; e.valid = true; e.dirty = isDirty; e.lastUsed = timer;
            return;
        }
    }
    // Evict LRU entry in TLB
    size_t lruIdx = 0; uint64_t oldest = std::numeric_limits<uint64_t>::max();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].lastUsed < oldest) { oldest = entries[i].lastUsed; lruIdx = i; }
    }
    entries[lruIdx] = {vpn, pfn, true, isDirty, timer};
}

void TLB::invalidate(PageNumber vpn) {
    for (auto& e : entries) {
        if (e.valid && e.vpn == vpn) { e.valid = false; return; }
    }
}

void TLB::markDirty(PageNumber vpn) {
    for (auto& e : entries) {
        if (e.valid && e.vpn == vpn) { e.dirty = true; return; }
    }
}

void TLB::flush() {
    for (auto& e : entries) { e.valid = false; e.lastUsed = 0; }
    timer = 0;
}

// -------------------------------------------------------------
// VirtualMemoryEngine Implementation
// -------------------------------------------------------------
VirtualMemoryEngine::VirtualMemoryEngine(size_t frames,
                                         std::unique_ptr<PageReplacementAlgorithm> algo,
                                         uint32_t pgSize)
    : pageSize(pgSize), frameCount(frames), policy(std::move(algo)), tlb(4), hits(0), faults(0) {}

void VirtualMemoryEngine::reset() {
    if (policy) policy->reset();
    pageTable.reset();
    tlb.flush();
    history.clear();
    hits = faults = 0;
}

TranslationTrace VirtualMemoryEngine::accessMemory(VirtualAddress vaddr, AccessType type) {
    TranslationTrace trace;
    trace.vaddr = vaddr;
    trace.vpn = vaddr / pageSize;
    trace.offset = vaddr % pageSize;
    trace.type = type;
    trace.tlbHit = false;
    trace.pageFault = false;
    trace.victimEvicted = false;
    trace.dirtyWriteback = false;
    trace.latencyNs = 10.0; // TLB check

    FrameNumber pfn = 0;
    if (tlb.lookup(trace.vpn, pfn)) {
        trace.tlbHit = true;
        trace.pfn = pfn;
        trace.latencyNs += 100.0; // RAM access
        ++hits;
        if (policy) policy->access(trace.vpn, type);
    } else {
        trace.latencyNs += 100.0; // Page table lookup
        if (pageTable.isPresent(trace.vpn)) {
            const auto* pte = pageTable.getEntry(trace.vpn);
            pfn = pte->frameNumber;
            trace.pfn = pfn;
            trace.latencyNs += 100.0;
            ++hits;
            if (policy) policy->access(trace.vpn, type);
            tlb.insert(trace.vpn, pfn, (type == AccessType::WRITE));
        } else {
            // PAGE FAULT
            trace.pageFault = true;
            trace.latencyNs += 8000000.0; // 8ms disk swap-in
            ++faults;

            AccessResult res = policy->access(trace.vpn, type);
            pfn = res.allocatedFrame;
            trace.pfn = pfn;

            if (res.victimEvicted) {
                trace.victimEvicted = true;
                trace.victimVpn = res.victimPage;
                tlb.invalidate(res.victimPage);
                bool wasDirty = false;
                pageTable.unmapPage(res.victimPage, wasDirty);
                if (res.dirtyWriteback || wasDirty) {
                    trace.dirtyWriteback = true;
                    trace.latencyNs += 12000000.0; // 12ms dirty writeback
                }
            }
            pageTable.mapPage(trace.vpn, pfn, (type == AccessType::WRITE));
            tlb.insert(trace.vpn, pfn, (type == AccessType::WRITE));
        }
    }

    if (type == AccessType::WRITE) {
        pageTable.markDirty(trace.vpn);
        tlb.markDirty(trace.vpn);
    }

    trace.paddr = (pfn * pageSize) + trace.offset;
    history.push_back(trace);
    return trace;
}

// -------------------------------------------------------------
// BeladyAnomalyDetector Implementation
// -------------------------------------------------------------
std::vector<MemoryReference> BeladyAnomalyDetector::getCanonicalString() {
    std::vector<PageNumber> pages = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    std::vector<MemoryReference> refs;
    for (auto p : pages) refs.emplace_back(p, AccessType::READ);
    return refs;
}

BeladyResult BeladyAnomalyDetector::verify(const std::vector<MemoryReference>& refs, size_t minF, size_t maxF) {
    BeladyResult result;
    for (size_t f = minF; f <= maxF; ++f) {
        FIFOReplacement fifo(f);
        fifo.runSimulation(refs);
        result.fifoFaults.push_back({f, fifo.getMetrics().pageFaults});

        LRUReplacement lru(f);
        lru.runSimulation(refs);
        result.lruFaults.push_back({f, lru.getMetrics().pageFaults});

        OptimalReplacement opt(f);
        opt.runSimulation(refs);
        result.optFaults.push_back({f, opt.getMetrics().pageFaults});
    }

    for (size_t i = 1; i < result.fifoFaults.size(); ++i) {
        if (result.fifoFaults[i].second > result.fifoFaults[i - 1].second) {
            result.anomalyDetected = true;
            result.anomalyFromFrames = result.fifoFaults[i - 1].first;
            result.anomalyToFrames = result.fifoFaults[i].first;
            result.anomalyFromFaults = result.fifoFaults[i - 1].second;
            result.anomalyToFaults = result.fifoFaults[i].second;
            break;
        }
    }
    return result;
}

} // namespace vmem
