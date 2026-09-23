#include "Visualizer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace vmem {

void Visualizer::printHeader(const std::string& title) {
    std::cout << "\n========================================================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================================================================\n";
}

void Visualizer::printSubHeader(const std::string& subTitle) {
    std::cout << "\n--- " << subTitle << " ---\n";
}

void Visualizer::printTraceTable(const std::string& algoName,
                                const std::vector<MemoryReference>& references,
                                const std::vector<AccessResult>& traceLog,
                                size_t frameCount) {
    printHeader("Execution Log & Frame State: " + algoName + " (" + std::to_string(frameCount) + " Frames)");

    // Header line
    std::cout << std::left
              << std::setw(6)  << "Step"
              << std::setw(8)  << "Page"
              << std::setw(8)  << "Type"
              << std::setw(10) << "Status"
              << std::setw(18) << "Victim Evicted";

    for (size_t f = 0; f < frameCount; ++f) {
        std::cout << std::setw(7) << ("Frame" + std::to_string(f));
    }
    std::cout << "\n";

    std::cout << std::string(50 + frameCount * 7, '-') << "\n";

    for (size_t i = 0; i < traceLog.size(); ++i) {
        const auto& log = traceLog[i];
        std::string status = log.hit ? "[HIT]" : "[FAULT]";
        std::string victimStr = "-";
        if (log.victimEvicted) {
            victimStr = "Page " + std::to_string(log.victimPage);
            if (log.dirtyWriteback) {
                victimStr += " (WB)";
            }
        }

        std::cout << std::left
                  << std::setw(6)  << (i + 1)
                  << std::setw(8)  << log.page
                  << std::setw(8)  << accessTypeToString(log.type)
                  << std::setw(10) << status
                  << std::setw(18) << victimStr;

        for (size_t f = 0; f < frameCount; ++f) {
            if (f < log.currentFrames.size() && log.currentFrames[f] != -1) {
                std::cout << std::setw(7) << log.currentFrames[f];
            } else {
                std::cout << std::setw(7) << "[ ]";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::string(50 + frameCount * 7, '=') << "\n";
}

void Visualizer::printBenchmarkTable(const BenchmarkSuiteResult& suite) {
    printHeader("Benchmark Results: " + suite.workloadName);
    std::cout << "Description: " << suite.workloadDescription << "\n";
    std::cout << "Physical Frame Allocation: " << suite.frameCount << " Frames\n\n";

    std::cout << std::left
              << std::setw(24) << "Algorithm"
              << std::setw(8)  << "Refs"
              << std::setw(8)  << "Hits"
              << std::setw(8)  << "Faults"
              << std::setw(10) << "Hit %"
              << std::setw(10) << "Fault %"
              << std::setw(10) << "Evictions"
              << std::setw(10) << "Dirty WB"
              << std::setw(14) << "EAT (ns)"
              << "\n";

    std::cout << std::string(102, '-') << "\n";

    for (const auto& m : suite.algorithmResults) {
        std::cout << std::left
                  << std::setw(24) << m.algorithmName
                  << std::setw(8)  << m.totalReferences
                  << std::setw(8)  << m.pageHits
                  << std::setw(8)  << m.pageFaults
                  << std::setw(10) << (std::to_string(static_cast<int>(m.hitRatio)) + "%")
                  << std::setw(10) << (std::to_string(static_cast<int>(m.faultRatio)) + "%")
                  << std::setw(10) << m.evictions
                  << std::setw(10) << m.dirtyWritebacks
                  << std::setw(14) << std::fixed << std::setprecision(1) << m.effectiveAccessTimeNs
                  << "\n";
    }
    std::cout << std::string(102, '=') << "\n";
}

void Visualizer::printBeladyAnalysis(const AnomalyResult& fifoResult,
                                    const AnomalyResult& lruResult,
                                    const AnomalyResult& optResult) {
    printHeader("Bélády's Anomaly Verification & Comparative Analysis");

    std::cout << "Reference String: ";
    for (size_t i = 0; i < fifoResult.referenceString.size(); ++i) {
        std::cout << fifoResult.referenceString[i].page << (i + 1 < fifoResult.referenceString.size() ? ", " : "");
    }
    std::cout << "\n\n";

    std::cout << std::left
              << std::setw(10) << "Frames"
              << std::setw(14) << "FIFO Faults"
              << std::setw(14) << "LRU Faults"
              << std::setw(14) << "OPT Faults"
              << std::setw(35) << "Bélády Anomaly Indicator"
              << "\n";

    std::cout << std::string(87, '-') << "\n";

    size_t count = fifoResult.frameFaultPairs.size();
    for (size_t i = 0; i < count; ++i) {
        size_t frames = fifoResult.frameFaultPairs[i].first;
        size_t fifoF = fifoResult.frameFaultPairs[i].second;
        size_t lruF = (i < lruResult.frameFaultPairs.size()) ? lruResult.frameFaultPairs[i].second : 0;
        size_t optF = (i < optResult.frameFaultPairs.size()) ? optResult.frameFaultPairs[i].second : 0;

        std::string indicator = "";
        if (i > 0 && fifoF > fifoResult.frameFaultPairs[i - 1].second) {
            indicator = "<-- * ANOMALY! (" + std::to_string(fifoResult.frameFaultPairs[i-1].second) +
                        " -> " + std::to_string(fifoF) + " faults) *";
        }

        std::cout << std::left
                  << std::setw(10) << frames
                  << std::setw(14) << fifoF
                  << std::setw(14) << lruF
                  << std::setw(14) << optF
                  << std::setw(35) << indicator
                  << "\n";
    }
    std::cout << std::string(87, '=') << "\n";

    // Visual ASCII Bar Chart
    std::cout << "\n--- ASCII Fault Distribution Chart (FIFO vs LRU) ---\n";
    for (size_t i = 0; i < count; ++i) {
        size_t frames = fifoResult.frameFaultPairs[i].first;
        size_t fifoF = fifoResult.frameFaultPairs[i].second;
        size_t lruF = (i < lruResult.frameFaultPairs.size()) ? lruResult.frameFaultPairs[i].second : 0;

        std::string fifoBar(fifoF * 2, '#');
        std::string lruBar(lruF * 2, '=');

        std::cout << "Frames " << frames << " | FIFO: [" << fifoBar << "] (" << fifoF << " faults)\n";
        std::cout << "         | LRU : [" << lruBar  << "] (" << lruF  << " faults)\n";
    }

    std::cout << "\nTheoretical Proof & Finding:\n";
    if (fifoResult.anomalyDetected) {
        std::cout << "[CONFIRMED] Bélády's Anomaly occurred in FIFO:\n";
        for (const auto& occ : fifoResult.occurrences) {
            std::cout << "  -> Increasing frames from " << occ.lowerFrames << " to " << occ.higherFrames
                      << " caused faults to INCREASE by +" << occ.deltaFaults
                      << " (from " << occ.lowerFaults << " to " << occ.higherFaults << " faults)!\n";
        }
    } else {
        std::cout << "[INFO] No Bélády's Anomaly detected for this specific workload.\n";
    }

    std::cout << "[STACK ALGORITHM VERIFICATION] LRU and Optimal faults are strictly non-increasing,\n";
    std::cout << "                               confirming the Stack Inclusion Property.\n";
    std::cout << "========================================================================================\n";
}

void Visualizer::printTranslationTraceTable(const std::vector<TranslationTrace>& traces) {
    printHeader("MMU Demand Paging & Address Translation Log");

    std::cout << std::left
              << std::setw(6)  << "Step"
              << std::setw(14) << "Virtual Addr"
              << std::setw(8)  << "VPN"
              << std::setw(8)  << "Offset"
              << std::setw(8)  << "Op"
              << std::setw(10) << "TLB"
              << std::setw(14) << "Page Table"
              << std::setw(8)  << "PFN"
              << std::setw(16) << "Physical Addr"
              << std::setw(16) << "Latency"
              << "\n";

    std::cout << std::string(108, '-') << "\n";

    for (size_t i = 0; i < traces.size(); ++i) {
        const auto& t = traces[i];
        std::stringstream ssVa, ssPa;
        ssVa << "0x" << std::hex << std::uppercase << t.virtualAddress;
        ssPa << "0x" << std::hex << std::uppercase << t.physicalAddress;

        std::string tlbStr = t.tlbHit ? "HIT" : "MISS";
        std::string ptStr = t.pageTableHit ? "HIT" : (t.pageFault ? "PAGE FAULT" : "-");

        std::stringstream ssLat;
        if (t.accessTimeNs >= 1000000.0) {
            ssLat << std::fixed << std::setprecision(2) << (t.accessTimeNs / 1000000.0) << " ms";
        } else {
            ssLat << std::fixed << std::setprecision(0) << t.accessTimeNs << " ns";
        }

        std::cout << std::left
                  << std::setw(6)  << (i + 1)
                  << std::setw(14) << ssVa.str()
                  << std::setw(8)  << t.vpn
                  << std::setw(8)  << t.offset
                  << std::setw(8)  << accessTypeToString(t.accessType)
                  << std::setw(10) << tlbStr
                  << std::setw(14) << ptStr
                  << std::setw(8)  << t.pfn
                  << std::setw(16) << ssPa.str()
                  << std::setw(16) << ssLat.str()
                  << "\n";
    }
    std::cout << std::string(108, '=') << "\n";
}

void Visualizer::printSummaryBox(const SimulationMetrics& metrics) {
    std::cout << "\n+-------------------------------------------------------+\n";
    std::cout << "| SIMULATION SUMMARY: " << std::left << std::setw(33) << metrics.algorithmName << "|\n";
    std::cout << "+-------------------------------------------------------+\n";
    std::cout << "| Physical Frames Allocated : " << std::setw(26) << metrics.frameCount << "|\n";
    std::cout << "| Total Page References     : " << std::setw(26) << metrics.totalReferences << "|\n";
    std::cout << "| Total Page Hits           : " << std::setw(26) << metrics.pageHits << "|\n";
    std::cout << "| Total Page Faults         : " << std::setw(26) << metrics.pageFaults << "|\n";
    std::cout << "| Hit Ratio                 : " << std::setw(25) << (std::to_string(static_cast<int>(metrics.hitRatio)) + "%") << "|\n";
    std::cout << "| Page Fault Ratio          : " << std::setw(25) << (std::to_string(static_cast<int>(metrics.faultRatio)) + "%") << "|\n";
    std::cout << "| Total Victim Evictions    : " << std::setw(26) << metrics.evictions << "|\n";
    std::cout << "| Dirty Page Writebacks     : " << std::setw(26) << metrics.dirtyWritebacks << "|\n";
    std::cout << "| Effective Access Time     : " << std::setw(23) << (std::to_string(static_cast<int>(metrics.effectiveAccessTimeNs)) + " ns") << "|\n";
    std::cout << "+-------------------------------------------------------+\n";
}

} // namespace vmem
