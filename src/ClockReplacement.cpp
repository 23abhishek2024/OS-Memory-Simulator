#include "ClockReplacement.hpp"

namespace vmem {

ClockReplacement::ClockReplacement(size_t frameCount)
    : PageReplacementAlgorithm(frameCount, "Clock (Second Chance)"),
      clockBuffer(frameCount),
      clockHand(0) {}

void ClockReplacement::reset() {
    for (auto& entry : clockBuffer) {
        entry.page = 0;
        entry.referenceBit = false;
        entry.dirty = false;
        entry.occupied = false;
    }
    clockHand = 0;
    pageToIndex.clear();
    metrics = SimulationMetrics();
    metrics.algorithmName = "Clock (Second Chance)";
    metrics.frameCount = frameCapacity;
}

AccessResult ClockReplacement::access(PageNumber page, AccessType type) {
    ++metrics.totalReferences;
    AccessResult result;
    result.page = page;
    result.type = type;

    auto it = pageToIndex.find(page);
    if (it != pageToIndex.end()) {
        // Page Hit: Set reference bit to 1 (give second chance)
        ++metrics.pageHits;
        result.hit = true;
        result.pageFault = false;

        size_t index = it->second;
        clockBuffer[index].referenceBit = true;
        result.allocatedFrame = static_cast<FrameNumber>(index);

        if (type == AccessType::WRITE) {
            clockBuffer[index].dirty = true;
        }
    } else {
        // Page Fault
        ++metrics.pageFaults;
        result.hit = false;
        result.pageFault = true;

        size_t targetIndex = 0;

        // Sweep clock hand
        while (true) {
            if (!clockBuffer[clockHand].occupied) {
                // Free slot available
                targetIndex = clockHand;
                clockHand = (clockHand + 1) % frameCapacity;
                break;
            }

            if (clockBuffer[clockHand].referenceBit) {
                // Second chance granted: clear reference bit and advance hand
                clockBuffer[clockHand].referenceBit = false;
                clockHand = (clockHand + 1) % frameCapacity;
            } else {
                // Victim found: reference bit was 0
                targetIndex = clockHand;
                auto& victim = clockBuffer[targetIndex];

                result.victimEvicted = true;
                result.victimPage = victim.page;
                ++metrics.evictions;

                if (victim.dirty) {
                    result.dirtyWriteback = true;
                    ++metrics.dirtyWritebacks;
                }

                pageToIndex.erase(victim.page);
                clockHand = (clockHand + 1) % frameCapacity;
                break;
            }
        }

        // Install new page
        bool isDirty = (type == AccessType::WRITE);
        clockBuffer[targetIndex].page = page;
        clockBuffer[targetIndex].referenceBit = true;
        clockBuffer[targetIndex].dirty = isDirty;
        clockBuffer[targetIndex].occupied = true;

        pageToIndex[page] = targetIndex;
        result.allocatedFrame = static_cast<FrameNumber>(targetIndex);
    }

    result.currentFrames = getFramesState();
    return result;
}

std::vector<int> ClockReplacement::getFramesState() const {
    std::vector<int> state(frameCapacity, -1);
    for (size_t i = 0; i < frameCapacity; ++i) {
        if (clockBuffer[i].occupied) {
            state[i] = static_cast<int>(clockBuffer[i].page);
        }
    }
    return state;
}

} // namespace vmem
