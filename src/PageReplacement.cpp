#include "PageReplacement.hpp"
#include <algorithm>

namespace vmem {

PageReplacementAlgorithm::PageReplacementAlgorithm(size_t frames, const std::string& name)
    : frameCapacity(frames), currentStep(0) {
    metrics.algorithmName = name;
    metrics.frameCount = frames;
}

void PageReplacementAlgorithm::runSimulation(const std::vector<MemoryReference>& references,
                                             std::vector<AccessResult>* traceLog) {
    reset();
    setFutureReferences(references);
    for (size_t i = 0; i < references.size(); ++i) {
        currentStep = i;
        AccessResult res = access(references[i].page, references[i].type);
        if (traceLog) traceLog->push_back(res);
    }
    metrics.calculate();
}

// -------------------------------------------------------------
// FIFO Implementation
// -------------------------------------------------------------
FIFOReplacement::FIFOReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "FIFO"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void FIFOReplacement::reset() {
    arrivalList.clear();
    pageMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = AlgorithmMetrics();
    metrics.algorithmName = "FIFO";
    metrics.frameCount = frameCapacity;
}

AccessResult FIFOReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult res; res.page = page; res.type = type;
    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        ++metrics.pageHits;
        res.hit = true;
        res.allocatedFrame = it->second->data.frame;
        if (type == AccessType::WRITE) {
            it->second->data.dirty = true;
            frameDirty[res.allocatedFrame] = true;
        }
    } else {
        ++metrics.pageFaults;
        res.pageFault = true;
        FrameNumber target = 0;
        if (arrivalList.size() < frameCapacity) {
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) { target = f; break; }
            }
        } else {
            auto* victimNode = arrivalList.getHead();
            Entry victim = victimNode->data;
            res.victimEvicted = true; res.victimPage = victim.page;
            target = victim.frame;
            ++metrics.evictions;
            if (victim.dirty || frameDirty[target]) {
                res.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }
            pageMap.erase(victim.page);
            arrivalList.pop_front();
        }
        bool isDirty = (type == AccessType::WRITE);
        auto* newNode = arrivalList.push_back({page, target, isDirty});
        pageMap[page] = newNode;
        frames[target] = page;
        frameDirty[target] = isDirty;
        res.allocatedFrame = target;
    }
    res.currentFrames = frames;
    return res;
}

// -------------------------------------------------------------
// LIFO Implementation (O(1) DLL used as a stack)
// -------------------------------------------------------------
LIFOReplacement::LIFOReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "LIFO"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void LIFOReplacement::reset() {
    loadStack.clear();
    pageMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = AlgorithmMetrics();
    metrics.algorithmName = "LIFO";
    metrics.frameCount = frameCapacity;
}

AccessResult LIFOReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult res; res.page = page; res.type = type;
    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        // Hit: LIFO only cares about load order, so nothing moves
        ++metrics.pageHits;
        res.hit = true;
        res.allocatedFrame = it->second->data.frame;
        if (type == AccessType::WRITE) {
            it->second->data.dirty = true;
            frameDirty[res.allocatedFrame] = true;
        }
    } else {
        ++metrics.pageFaults;
        res.pageFault = true;
        FrameNumber target = 0;
        if (loadStack.size() < frameCapacity) {
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) { target = f; break; }
            }
        } else {
            auto* victimNode = loadStack.getTail(); // O(1): most recently loaded page (top of stack)
            Entry victim = victimNode->data;
            res.victimEvicted = true; res.victimPage = victim.page;
            target = victim.frame;
            ++metrics.evictions;
            if (victim.dirty || frameDirty[target]) {
                res.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }
            pageMap.erase(victim.page);
            loadStack.pop_back();
        }
        bool isDirty = (type == AccessType::WRITE);
        auto* newNode = loadStack.push_back({page, target, isDirty}); // push onto stack
        pageMap[page] = newNode;
        frames[target] = page;
        frameDirty[target] = isDirty;
        res.allocatedFrame = target;
    }
    res.currentFrames = frames;
    return res;
}

// -------------------------------------------------------------
// LRU Implementation (O(1) DLL + Hash Map)
// -------------------------------------------------------------
LRUReplacement::LRUReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "LRU"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void LRUReplacement::reset() {
    lruList.clear();
    pageMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = AlgorithmMetrics();
    metrics.algorithmName = "LRU";
    metrics.frameCount = frameCapacity;
}

AccessResult LRUReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult res; res.page = page; res.type = type;
    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        ++metrics.pageHits;
        res.hit = true;
        auto* node = it->second;
        lruList.move_to_front(node); // O(1) promotion
        FrameNumber frame = node->data.frame;
        res.allocatedFrame = frame;
        if (type == AccessType::WRITE) {
            node->data.dirty = true;
            frameDirty[frame] = true;
        }
    } else {
        ++metrics.pageFaults;
        res.pageFault = true;
        FrameNumber target = 0;
        if (lruList.size() < frameCapacity) {
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) { target = f; break; }
            }
        } else {
            auto* victimNode = lruList.getTail(); // O(1) LRU victim
            Entry victim = victimNode->data;
            res.victimEvicted = true; res.victimPage = victim.page;
            target = victim.frame;
            ++metrics.evictions;
            if (victim.dirty || frameDirty[target]) {
                res.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }
            pageMap.erase(victim.page);
            lruList.pop_back();
        }
        bool isDirty = (type == AccessType::WRITE);
        auto* newNode = lruList.push_front({page, target, isDirty});
        pageMap[page] = newNode;
        frames[target] = page;
        frameDirty[target] = isDirty;
        res.allocatedFrame = target;
    }
    res.currentFrames = frames;
    return res;
}

// -------------------------------------------------------------
// LFU Implementation (O(1) Frequency Buckets)
// -------------------------------------------------------------
LFUReplacement::LFUReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "LFU"),
      minFreq(0),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void LFUReplacement::reset() {
    pageMap.clear();
    freqBuckets.clear();
    minFreq = 0;
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = AlgorithmMetrics();
    metrics.algorithmName = "LFU";
    metrics.frameCount = frameCapacity;
}

AccessResult LFUReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult res; res.page = page; res.type = type;
    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        ++metrics.pageHits;
        res.hit = true;
        PageNode node = it->second;
        Entry entry = node->data;
        uint32_t oldFreq = entry.freq;
        res.allocatedFrame = entry.frame;
        if (type == AccessType::WRITE) {
            entry.dirty = true;
            frameDirty[entry.frame] = true;
        }
        freqBuckets[oldFreq].remove(node);
        if (freqBuckets[oldFreq].empty()) {
            freqBuckets.erase(oldFreq);
            if (minFreq == oldFreq) minFreq = oldFreq + 1;
        }
        entry.freq = oldFreq + 1;
        pageMap[page] = freqBuckets[entry.freq].push_front(entry);
    } else {
        ++metrics.pageFaults;
        res.pageFault = true;
        FrameNumber target = 0;
        if (pageMap.size() < frameCapacity) {
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) { target = f; break; }
            }
        } else {
            auto& bucket = freqBuckets[minFreq];
            auto* victimNode = bucket.getTail();
            Entry victim = victimNode->data;
            res.victimEvicted = true; res.victimPage = victim.page;
            target = victim.frame;
            ++metrics.evictions;
            if (victim.dirty || frameDirty[target]) {
                res.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }
            pageMap.erase(victim.page);
            bucket.pop_back();
            if (bucket.empty()) freqBuckets.erase(minFreq);
        }
        bool isDirty = (type == AccessType::WRITE);
        minFreq = 1;
        pageMap[page] = freqBuckets[1].push_front({page, target, 1, isDirty});
        frames[target] = page;
        frameDirty[target] = isDirty;
        res.allocatedFrame = target;
    }
    res.currentFrames = frames;
    return res;
}

// -------------------------------------------------------------
// MFU Implementation
// -------------------------------------------------------------
MFUReplacement::MFUReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "MFU"),
      frames(frameCount, -1),
      frameDirty(frameCount, false),
      timer(0) {}

void MFUReplacement::reset() {
    resident.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    timer = 0;
    metrics = AlgorithmMetrics();
    metrics.algorithmName = "MFU";
    metrics.frameCount = frameCapacity;
}

AccessResult MFUReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    ++timer;
    AccessResult res; res.page = page; res.type = type;
    auto it = resident.find(page);
    if (it != resident.end()) {
        ++metrics.pageHits;
        res.hit = true;
        it->second.freq++;
        it->second.lastUsed = timer;
        res.allocatedFrame = it->second.frame;
        if (type == AccessType::WRITE) {
            it->second.dirty = true;
            frameDirty[res.allocatedFrame] = true;
        }
    } else {
        ++metrics.pageFaults;
        res.pageFault = true;
        FrameNumber target = 0;
        if (resident.size() < frameCapacity) {
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) { target = f; break; }
            }
        } else {
            PageNumber victimPage = 0;
            uint32_t maxFreq = 0;
            uint64_t oldest = std::numeric_limits<uint64_t>::max();
            for (const auto& kv : resident) {
                if (kv.second.freq > maxFreq || (kv.second.freq == maxFreq && kv.second.lastUsed < oldest)) {
                    maxFreq = kv.second.freq;
                    oldest = kv.second.lastUsed;
                    victimPage = kv.first;
                }
            }
            Entry victim = resident[victimPage];
            res.victimEvicted = true; res.victimPage = victim.page;
            target = victim.frame;
            ++metrics.evictions;
            if (victim.dirty || frameDirty[target]) {
                res.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }
            resident.erase(victimPage);
        }
        bool isDirty = (type == AccessType::WRITE);
        resident[page] = {page, target, 1, timer, isDirty};
        frames[target] = page;
        frameDirty[target] = isDirty;
        res.allocatedFrame = target;
    }
    res.currentFrames = frames;
    return res;
}

// -------------------------------------------------------------
// Optimal Implementation
// -------------------------------------------------------------
OptimalReplacement::OptimalReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "Optimal (OPT)"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void OptimalReplacement::reset() {
    resident.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = AlgorithmMetrics();
    metrics.algorithmName = "Optimal (OPT)";
    metrics.frameCount = frameCapacity;
}

size_t OptimalReplacement::findNextUse(PageNumber page, size_t fromIndex) const {
    for (size_t i = fromIndex; i < futureReferences.size(); ++i) {
        if (futureReferences[i].page == page) return i;
    }
    return std::numeric_limits<size_t>::max();
}

AccessResult OptimalReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult res; res.page = page; res.type = type;
    auto it = resident.find(page);
    if (it != resident.end()) {
        ++metrics.pageHits;
        res.hit = true;
        res.allocatedFrame = it->second;
        if (type == AccessType::WRITE) frameDirty[res.allocatedFrame] = true;
    } else {
        ++metrics.pageFaults;
        res.pageFault = true;
        FrameNumber target = 0;
        if (resident.size() < frameCapacity) {
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) { target = f; break; }
            }
        } else {
            // Scan frames in order so ties (e.g. two pages never used again)
            // always go to the lowest frame -> same result on every compiler
            PageNumber victimPage = static_cast<PageNumber>(frames[0]);
            size_t farthest = 0;
            for (size_t f = 0; f < frameCapacity; ++f) {
                PageNumber candidate = static_cast<PageNumber>(frames[f]);
                size_t nextUse = findNextUse(candidate, currentStep + 1);
                if (nextUse == std::numeric_limits<size_t>::max()) {
                    victimPage = candidate;
                    break;
                }
                if (nextUse > farthest) {
                    farthest = nextUse;
                    victimPage = candidate;
                }
            }
            target = resident[victimPage];
            res.victimEvicted = true; res.victimPage = victimPage;
            ++metrics.evictions;
            if (frameDirty[target]) {
                res.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }
            resident.erase(victimPage);
        }
        bool isDirty = (type == AccessType::WRITE);
        resident[page] = target;
        frames[target] = page;
        frameDirty[target] = isDirty;
        res.allocatedFrame = target;
    }
    res.currentFrames = frames;
    return res;
}

} // namespace vmem
