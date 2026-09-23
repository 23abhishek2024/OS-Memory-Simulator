#ifndef MEMORY_TYPES_HPP
#define MEMORY_TYPES_HPP

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace vmem {

// Distinct types for virtual and physical addressing
using PageNumber = uint32_t;
using FrameNumber = uint32_t;
using VirtualAddress = uint32_t;
using PhysicalAddress = uint32_t;

// Access operation types
enum class AccessType {
    READ,
    WRITE
};

inline std::string accessTypeToString(AccessType type) {
    return (type == AccessType::WRITE) ? "WRITE" : "READ";
}

// Memory reference event for tracing
struct MemoryReference {
    PageNumber page;
    AccessType type;
    VirtualAddress vaddr;

    MemoryReference(PageNumber p, AccessType t = AccessType::READ, VirtualAddress va = 0)
        : page(p), type(t), vaddr(va) {}
};

// Page Table Entry (PTE)
struct PageTableEntry {
    FrameNumber frameNumber = 0;
    bool valid = false;         // Present in physical memory
    bool dirty = false;         // Modified while in memory (needs swap writeback)
    bool referenced = false;    // Referenced recently (used by Clock/Second-chance)
    bool readOnly = false;      // Access protection flag
    uint64_t lastAccessTime = 0;
    uint32_t accessCount = 0;

    void reset() {
        frameNumber = 0;
        valid = false;
        dirty = false;
        referenced = false;
        readOnly = false;
        lastAccessTime = 0;
        accessCount = 0;
    }
};

// TLB Entry
struct TLBEntry {
    PageNumber pageNumber = 0;
    FrameNumber frameNumber = 0;
    bool valid = false;
    bool dirty = false;
    uint64_t lastUsed = 0;
};

// Step result for page access
struct AccessResult {
    PageNumber page;
    AccessType type;
    bool hit = false;
    bool pageFault = false;
    bool victimEvicted = false;
    PageNumber victimPage = 0;
    FrameNumber allocatedFrame = 0;
    bool dirtyWriteback = false;
    std::vector<int> currentFrames; // Frame state snapshot for visualizer
};

// High-level simulation benchmark metrics
struct SimulationMetrics {
    std::string algorithmName;
    size_t frameCount = 0;
    size_t totalReferences = 0;
    size_t pageHits = 0;
    size_t pageFaults = 0;
    size_t evictions = 0;
    size_t dirtyWritebacks = 0;
    size_t tlbHits = 0;
    size_t tlbMisses = 0;
    double hitRatio = 0.0;
    double faultRatio = 0.0;
    double effectiveAccessTimeNs = 0.0; // EAT in nanoseconds

    void calculate() {
        if (totalReferences > 0) {
            hitRatio = (static_cast<double>(pageHits) / totalReferences) * 100.0;
            faultRatio = (static_cast<double>(pageFaults) / totalReferences) * 100.0;
        } else {
            hitRatio = 0.0;
            faultRatio = 0.0;
        }

        // Standard OS Effective Access Time (EAT) calculation:
        // EAT = (1 - p) * MemoryAccessTime + p * (PageFaultOverhead + (dirty ? WritebackOverhead : 0))
        // Where:
        // Memory Access Time = 100 ns
        // TLB Hit Time = 10 ns
        // Page Fault Service Time = 8,000,000 ns (8 ms)
        // Dirty Page Writeback Overhead = 12,000,000 ns (12 ms)
        const double MEM_ACCESS_NS = 100.0;
        const double TLB_HIT_NS = 10.0;
        const double PAGE_FAULT_NS = 8000000.0;
        const double WRITEBACK_NS = 12000000.0;

        if (totalReferences > 0) {
            double cleanFaults = static_cast<double>(pageFaults - dirtyWritebacks);
            double dirtyFaults = static_cast<double>(dirtyWritebacks);
            double hitCost = static_cast<double>(pageHits) * (TLB_HIT_NS + MEM_ACCESS_NS);
            double cleanFaultCost = cleanFaults * (TLB_HIT_NS + MEM_ACCESS_NS + PAGE_FAULT_NS);
            double dirtyFaultCost = dirtyFaults * (TLB_HIT_NS + MEM_ACCESS_NS + PAGE_FAULT_NS + WRITEBACK_NS);

            effectiveAccessTimeNs = (hitCost + cleanFaultCost + dirtyFaultCost) / totalReferences;
        }
    }
};

} // namespace vmem

#endif // MEMORY_TYPES_HPP
