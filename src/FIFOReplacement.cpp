#include "FIFOReplacement.hpp"

namespace vmem {

FIFOReplacement::FIFOReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "FIFO"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void FIFOReplacement::reset() {
    orderList.clear();
    pageMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = SimulationMetrics();
    metrics.algorithmName = "FIFO";
    metrics.frameCount = frameCapacity;
}

AccessResult FIFOReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult result;
    result.page = page;
    result.type = type;

    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        // Page Hit
        ++metrics.pageHits;
        result.hit = true;
        result.pageFault = false;
        FrameNumber frame = it->second->data.frame;
        result.allocatedFrame = frame;

        if (type == AccessType::WRITE) {
            it->second->data.dirty = true;
            frameDirty[frame] = true;
        }
    } else {
        // Page Fault
        ++metrics.pageFaults;
        result.hit = false;
        result.pageFault = true;

        FrameNumber targetFrame = 0;

        if (orderList.size() < frameCapacity) {
            // Find first unallocated frame
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) {
                    targetFrame = static_cast<FrameNumber>(f);
                    break;
                }
            }
        } else {
            // Memory is full: Evict earliest arrived page (head of FIFO list)
            auto* victimNode = orderList.getHead();
            FIFOEntry victim = victimNode->data;
            result.victimEvicted = true;
            result.victimPage = victim.page;
            targetFrame = victim.frame;
            ++metrics.evictions;

            if (victim.dirty || frameDirty[targetFrame]) {
                result.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }

            pageMap.erase(victim.page);
            orderList.pop_front();
        }

        // Place new page in target frame
        bool isDirty = (type == AccessType::WRITE);
        auto* newNode = orderList.push_back({page, targetFrame, isDirty});
        pageMap[page] = newNode;
        frames[targetFrame] = static_cast<int>(page);
        frameDirty[targetFrame] = isDirty;
        result.allocatedFrame = targetFrame;
    }

    result.currentFrames = frames;
    return result;
}

} // namespace vmem
