#include "OptimalReplacement.hpp"
#include <limits>

namespace vmem {

OptimalReplacement::OptimalReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "Optimal (OPT)"),
      frames(frameCount, -1),
      frameDirty(frameCount, false) {}

void OptimalReplacement::reset() {
    residentMap.clear();
    std::fill(frames.begin(), frames.end(), -1);
    std::fill(frameDirty.begin(), frameDirty.end(), false);
    metrics = SimulationMetrics();
    metrics.algorithmName = "Optimal (OPT)";
    metrics.frameCount = frameCapacity;
}

size_t OptimalReplacement::findNextUse(PageNumber page, size_t fromIndex) const {
    for (size_t i = fromIndex; i < futureReferences.size(); ++i) {
        if (futureReferences[i].page == page) {
            return i;
        }
    }
    return std::numeric_limits<size_t>::max();
}

AccessResult OptimalReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult result;
    result.page = page;
    result.type = type;

    auto it = residentMap.find(page);
    if (it != residentMap.end()) {
        // Page Hit
        ++metrics.pageHits;
        result.hit = true;
        result.pageFault = false;
        FrameNumber frame = it->second;
        result.allocatedFrame = frame;

        if (type == AccessType::WRITE) {
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
            // Memory is full: Look ahead into the future to find victim with farthest next use
            PageNumber victimPage = 0;
            size_t farthestIndex = 0;

            for (const auto& kv : residentMap) {
                PageNumber candidatePage = kv.first;
                size_t nextUse = findNextUse(candidatePage, currentReferenceIndex + 1);

                if (nextUse > farthestIndex) {
                    farthestIndex = nextUse;
                    victimPage = candidatePage;
                }

                // If candidate is never used again in the future, it's an optimal victim
                if (nextUse == std::numeric_limits<size_t>::max()) {
                    victimPage = candidatePage;
                    break;
                }
            }

            targetFrame = residentMap[victimPage];
            result.victimEvicted = true;
            result.victimPage = victimPage;
            ++metrics.evictions;

            if (frameDirty[targetFrame]) {
                result.dirtyWriteback = true;
                ++metrics.dirtyWritebacks;
            }

            residentMap.erase(victimPage);
        }

        // Allocate target frame to new page
        bool isDirty = (type == AccessType::WRITE);
        residentMap[page] = targetFrame;
        frames[targetFrame] = static_cast<int>(page);
        frameDirty[targetFrame] = isDirty;
        result.allocatedFrame = targetFrame;
    }

    result.currentFrames = frames;
    return result;
}

} // namespace vmem
