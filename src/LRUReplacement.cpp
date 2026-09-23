#include "LRUReplacement.hpp"

namespace vmem {

LRUReplacement::LRUReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "LRU"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void LRUReplacement::reset() {
    orderList.clear();
    pageMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = SimulationMetrics();
    metrics.algorithmName = "LRU";
    metrics.frameCount = frameCapacity;
}

AccessResult LRUReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult result;
    result.page = page;
    result.type = type;

    auto it = pageMap.find(page);
    if (it != pageMap.end()) {
        // Page Hit: move accessed page to MRU (front of list) in O(1)
        ++metrics.pageHits;
        result.hit = true;
        result.pageFault = false;

        auto* node = it->second;
        orderList.move_to_front(node);

        FrameNumber frame = node->data.frame;
        result.allocatedFrame = frame;

        if (type == AccessType::WRITE) {
            node->data.dirty = true;
            frameDirty[frame] = true;
        }
    } else {
        // Page Fault
        ++metrics.pageFaults;
        result.hit = false;
        result.pageFault = true;

        FrameNumber targetFrame = 0;

        if (orderList.size() < frameCapacity) {
            // Find free frame
            for (size_t f = 0; f < frameCapacity; ++f) {
                if (frames[f] == -1) {
                    targetFrame = static_cast<FrameNumber>(f);
                    break;
                }
            }
        } else {
            // Evict Least Recently Used page (tail of list) in O(1)
            auto* victimNode = orderList.getTail();
            LRUEntry victim = victimNode->data;
            result.victimEvicted = true;
            result.victimPage = victim.page;
            targetFrame = victim.frame;
            ++metrics.evictions;

            if (victim.dirty || frameDirty[targetFrame]) {
                result.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }

            pageMap.erase(victim.page);
            orderList.pop_back();
        }

        // Insert newly referenced page at MRU (front) in O(1)
        bool isDirty = (type == AccessType::WRITE);
        auto* newNode = orderList.push_front({page, targetFrame, isDirty});
        pageMap[page] = newNode;
        frames[targetFrame] = static_cast<int>(page);
        frameDirty[targetFrame] = isDirty;
        result.allocatedFrame = targetFrame;
    }

    result.currentFrames = frames;
    return result;
}

} // namespace vmem
