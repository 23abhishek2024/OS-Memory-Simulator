#ifndef PAGE_REPLACEMENT_HPP
#define PAGE_REPLACEMENT_HPP

#include "DoublyLinkedList.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <limits>
#include <iomanip>

namespace vmem {

using PageNumber = uint32_t;
using FrameNumber = uint32_t;

enum class AccessType { READ, WRITE };

inline std::string accessTypeToString(AccessType t) {
    return (t == AccessType::WRITE) ? "WRITE" : "READ";
}

struct MemoryReference {
    PageNumber page;
    AccessType type;
    MemoryReference(PageNumber p, AccessType t = AccessType::READ) : page(p), type(t) {}
};

struct AccessResult {
    PageNumber page = 0;
    AccessType type = AccessType::READ;
    bool hit = false;
    bool pageFault = false;
    bool victimEvicted = false;
    PageNumber victimPage = 0;
    FrameNumber allocatedFrame = 0;
    bool dirtyWriteback = false;
    std::vector<int> currentFrames;
};

struct AlgorithmMetrics {
    std::string algorithmName;
    size_t frameCount = 0;
    size_t totalReferences = 0;
    size_t pageHits = 0;
    size_t pageFaults = 0;
    size_t evictions = 0;
    size_t dirtyWritebacks = 0;
    double hitRatio = 0.0;
    double faultRatio = 0.0;
    double effectiveAccessTimeNs = 0.0;

    void calculate() {
        if (totalReferences > 0) {
            hitRatio = (static_cast<double>(pageHits) / totalReferences) * 100.0;
            faultRatio = (static_cast<double>(pageFaults) / totalReferences) * 100.0;
            // EAT: 10ns TLB + 100ns RAM, 8ms clean fault, 20ms dirty fault
            double hitCost = pageHits * 110.0;
            double cleanFaultCost = (pageFaults - dirtyWritebacks) * 8000110.0;
            double dirtyFaultCost = dirtyWritebacks * 20000110.0;
            effectiveAccessTimeNs = (hitCost + cleanFaultCost + dirtyFaultCost) / totalReferences;
        }
    }
};

// Abstract Base Class
class PageReplacementAlgorithm {
protected:
    size_t frameCapacity;
    AlgorithmMetrics metrics;
    std::vector<MemoryReference> futureReferences;
    size_t currentStep;

public:
    PageReplacementAlgorithm(size_t frames, const std::string& name);
    virtual ~PageReplacementAlgorithm() = default;

    virtual AccessResult access(PageNumber page, AccessType type = AccessType::READ) = 0;
    virtual void reset() = 0;
    virtual std::string getName() const = 0;
    virtual std::vector<int> getFramesState() const = 0;

    void setFutureReferences(const std::vector<MemoryReference>& refs) {
        futureReferences = refs;
        currentStep = 0;
    }

    void runSimulation(const std::vector<MemoryReference>& references,
                       std::vector<AccessResult>* traceLog = nullptr);

    const AlgorithmMetrics& getMetrics() const { return metrics; }
    size_t getCapacity() const { return frameCapacity; }
};

// 1. FIFO (First-In, First-Out)
class FIFOReplacement : public PageReplacementAlgorithm {
private:
    struct Entry { PageNumber page; FrameNumber frame; bool dirty; };
    DoublyLinkedList<Entry> arrivalList;
    std::unordered_map<PageNumber, DoublyLinkedList<Entry>::Node*> pageMap;
    std::vector<int> frames;
    std::vector<bool> frameDirty;

public:
    explicit FIFOReplacement(size_t frameCount);
    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "FIFO"; }
    std::vector<int> getFramesState() const override { return frames; }
};

// 2. LRU (Least Recently Used) - O(1) via Doubly Linked List + Hash Map
class LRUReplacement : public PageReplacementAlgorithm {
private:
    struct Entry { PageNumber page; FrameNumber frame; bool dirty; };
    DoublyLinkedList<Entry> lruList; // Head = MRU, Tail = LRU
    std::unordered_map<PageNumber, DoublyLinkedList<Entry>::Node*> pageMap;
    std::vector<int> frames;
    std::vector<bool> frameDirty;

public:
    explicit LRUReplacement(size_t frameCount);
    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "LRU"; }
    std::vector<int> getFramesState() const override { return frames; }
};

// 3. LFU (Least Frequently Used) - O(1) via Frequency Buckets
class LFUReplacement : public PageReplacementAlgorithm {
private:
    struct Entry { PageNumber page; FrameNumber frame; uint32_t freq; bool dirty; };
    using PageNode = DoublyLinkedList<Entry>::Node*;
    std::unordered_map<PageNumber, PageNode> pageMap;
    std::unordered_map<uint32_t, DoublyLinkedList<Entry>> freqBuckets;
    uint32_t minFreq;
    std::vector<int> frames;
    std::vector<bool> frameDirty;

public:
    explicit LFUReplacement(size_t frameCount);
    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "LFU"; }
    std::vector<int> getFramesState() const override { return frames; }
};

// 4. MFU (Most Frequently Used)
class MFUReplacement : public PageReplacementAlgorithm {
private:
    struct Entry { PageNumber page; FrameNumber frame; uint32_t freq; uint64_t lastUsed; bool dirty; };
    std::unordered_map<PageNumber, Entry> resident;
    std::vector<int> frames;
    std::vector<bool> frameDirty;
    uint64_t timer;

public:
    explicit MFUReplacement(size_t frameCount);
    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "MFU"; }
    std::vector<int> getFramesState() const override { return frames; }
};

// 5. Optimal (Bélády's Optimal / MIN)
class OptimalReplacement : public PageReplacementAlgorithm {
private:
    std::unordered_map<PageNumber, FrameNumber> resident;
    std::vector<int> frames;
    std::vector<bool> frameDirty;
    size_t findNextUse(PageNumber page, size_t fromIndex) const;

public:
    explicit OptimalReplacement(size_t frameCount);
    AccessResult access(PageNumber page, AccessType type = AccessType::READ) override;
    void reset() override;
    std::string getName() const override { return "Optimal (OPT)"; }
    std::vector<int> getFramesState() const override { return frames; }
};

} // namespace vmem

#endif // PAGE_REPLACEMENT_HPP
