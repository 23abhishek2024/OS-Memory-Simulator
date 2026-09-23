#include "MFUReplacement.hpp"
#include <limits>

namespace vmem {

MFUReplacement::MFUReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "MFU"),
      frames(frameCount, -1),
      frameDirty(frameCount, false),
      accessCounter(0) {}

void MFUReplacement::reset() {
    residentMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    accessCounter = 0;
    metrics = SimulationMetrics();
    metrics.algorithmName = "MFU";
    metrics.frameCount = frameCapacity;
}

AccessResult MFUReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    ++accessCounter;
    AccessResult result;
    result.page = page;
    result.type = type;

    auto it = residentMap.find(page);
    if (it != residentMap.end()) {
        // Page Hit: Increment frequency
        ++metrics.pageHits;
        result.hit = true;
        result.pageFault = false;

        it->second.freq++;
        it->second.lastAccessTime = accessCounter;
        FrameNumber frame = it->second.frame;
        result.allocatedFrame = frame;

        if (type == AccessType::WRITE) {
            it->second.dirty = true;
            frameDirty[frame] = true;
        }
    } else {
        // Page Fault
        ++metrics.pageFaults;
        result.hit = false;
        result.pageFault = true;

        FrameNumber targetFrame = 0;

        if (residentMap.size() < frameCapacity) {
            // Find free frame
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) {
                    targetFrame = static_cast<FrameNumber>(f);
                    break;
                }
            }
        } else {
            // Memory is full: Evict page with MAXIMUM frequency
            PageNumber victimPage = 0;
            uint32_t maxFreq = 0;
            uint64_t oldestAccess = std::numeric_limits<uint64_t>::max();

            for (const auto& kv : residentMap) {
                const auto& entry = kv.second;
                if (entry.freq > maxFreq ||
                    (entry.freq == maxFreq && entry.lastAccessTime < oldestAccess)) {
                    maxFreq = entry.freq;
                    oldestAccess = entry.lastAccessTime;
                    victimPage = entry.page;
                }
            }

            MFUEntry victim = residentMap[victimPage];
            result.victimEvicted = true;
            result.victimPage = victim.page;
            targetFrame = victim.frame;
            ++metrics.evictions;

            if (victim.dirty || frameDirty[targetFrame]) {
                result.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }

            residentMap.erase(victimPage);
        }

        // Insert new page with initial frequency = 1
        bool isDirty = (type == AccessType::WRITE);
        residentMap[page] = MFUEntry{page, targetFrame, 1, accessCounter, isDirty};
        frames[targetFrame] = static_cast<int>(page);
        frameDirty[targetFrame] = isDirty;
        result.allocatedFrame = targetFrame;
    }

    result.currentFrames = frames;
    return result;
}

} // namespace vmem
