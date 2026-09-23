#include "LFUReplacement.hpp"

namespace vmem {

LFUReplacement::LFUReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "LFU"),
      minFrequency(0),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void LFUReplacement::reset() {
    pageMap.clear();
    freqBuckets.clear();
    minFrequency = 0;
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = SimulationMetrics();
    metrics.algorithmName = "LFU";
    metrics.frameCount = frameCapacity;
}

AccessResult LFUReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult result;
    result.page = page;
    result.type = type;

    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        // Page Hit: Increment frequency and promote in O(1)
        ++metrics.pageHits;
        result.hit = true;
        result.pageFault = false;

        PageNode oldNode = it->second;
        LFUEntry entry = oldNode->data;
        uint32_t oldFreq = entry.freq;
        FrameNumber frame = entry.frame;
        result.allocatedFrame = frame;

        if (type == AccessType::WRITE) {
            entry.dirty = true;
            frameDirty[frame] = true;
        }

        // Remove from old frequency bucket
        freqBuckets[oldFreq].remove(oldNode);
        if (freqBuckets[oldFreq].empty()) {
            freqBuckets.erase(oldFreq);
            if (minFrequency == oldFreq) {
                minFrequency = oldFreq + 1;
            }
        }

        // Add to new frequency bucket
        entry.freq = oldFreq + 1;
        PageNode newNode = freqBuckets[entry.freq].push_front(entry);
        pageMap[page] = newNode;
    } else {
        // Page Fault
        ++metrics.pageFaults;
        result.hit = false;
        result.pageFault = true;

        FrameNumber targetFrame = 0;

        if (pageMap.size() < frameCapacity) {
            // Find free frame
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) {
                    targetFrame = static_cast<FrameNumber>(f);
                    break;
                }
            }
        } else {
            // Evict victim with lowest frequency (and LRU within that bucket) in O(1)
            auto& bucket = freqBuckets[minFrequency];
            auto* victimNode = bucket.getTail();
            LFUEntry victim = victimNode->data;
            result.victimEvicted = true;
            result.victimPage = victim.page;
            targetFrame = victim.frame;
            ++metrics.evictions;

            if (victim.dirty || frameDirty[targetFrame]) {
                result.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }

            pageMap.erase(victim.page);
            bucket.pop_back();

            if (bucket.empty()) {
                freqBuckets.erase(minFrequency);
            }
        }

        // Insert new page with frequency = 1
        bool isDirty = (type == AccessType::WRITE);
        LFUEntry newEntry{page, targetFrame, 1, isDirty};
        minFrequency = 1;
        PageNode newNode = freqBuckets[1].push_front(newEntry);
        pageMap[page] = newNode;
        frames[targetFrame] = static_cast<int>(page);
        frameDirty[targetFrame] = isDirty;
        result.allocatedFrame = targetFrame;
    }

    result.currentFrames = frames;
    return result;
}

} // namespace vmem
