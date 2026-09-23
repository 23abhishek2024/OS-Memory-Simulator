#include "DoublyLinkedList.hpp"
#include "PageReplacement.hpp"
#include "VirtualMemory.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>

using namespace vmem;

void printBanner(const std::string& title) {
    std::cout << "\n========================================================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================================================\n";
}

void runStepByStep() {
    printBanner("Interactive Step-by-Step Page Replacement Simulation");

    std::cout << "\nSelect Algorithm:\n";
    std::cout << "  1. FIFO\n";
    std::cout << "  2. LRU [O(1) Custom Doubly Linked List + Hash Map]\n";
    std::cout << "  3. LFU [O(1) Frequency Buckets]\n";
    std::cout << "  4. MFU\n";
    std::cout << "  5. Optimal (Bélády's OPT)\n";
    std::cout << "Choice (1-5) [default: 2]: ";

    std::string choiceStr; std::getline(std::cin, choiceStr);
    int c = 2; if (!choiceStr.empty()) { try { c = std::stoi(choiceStr); } catch (...) { c = 2; } }

    std::cout << "Enter number of frames [default: 3]: ";
    std::string fStr; std::getline(std::cin, fStr);
    size_t frames = 3; if (!fStr.empty()) { try { frames = std::stoul(fStr); } catch (...) { frames = 3; } }

    std::cout << "Enter page reference sequence (e.g. 7 0 1 2 0 3 0 4) or press Enter for default: ";
    std::string seqStr; std::getline(std::cin, seqStr);

    std::vector<MemoryReference> refs;
    if (seqStr.empty()) {
        std::vector<PageNumber> textbook = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
        for (auto p : textbook) refs.emplace_back(p, AccessType::READ);
    } else {
        for (char& ch : seqStr) if (ch == ',') ch = ' ';
        std::stringstream ss(seqStr);
        PageNumber p; while (ss >> p) refs.emplace_back(p, AccessType::READ);
    }

    std::unique_ptr<PageReplacementAlgorithm> algo;
    if (c == 1) algo.reset(new FIFOReplacement(frames));
    else if (c == 2) algo.reset(new LRUReplacement(frames));
    else if (c == 3) algo.reset(new LFUReplacement(frames));
    else if (c == 4) algo.reset(new MFUReplacement(frames));
    else algo.reset(new OptimalReplacement(frames));

    std::vector<AccessResult> logs;
    algo->runSimulation(refs, &logs);

    std::cout << "\nStep  Page   Status     Victim     Frames State\n";
    std::cout << "------------------------------------------------------\n";
    for (size_t i = 0; i < logs.size(); ++i) {
        std::string status = logs[i].hit ? "[HIT]  " : "[FAULT]";
        std::string vic = logs[i].victimEvicted ? ("Page " + std::to_string(logs[i].victimPage)) : "-";
        std::cout << std::left << std::setw(6) << (i + 1)
                  << std::setw(7) << logs[i].page
                  << std::setw(11) << status
                  << std::setw(11) << vic << "[ ";
        for (size_t f = 0; f < frames; ++f) {
            if (f < logs[i].currentFrames.size() && logs[i].currentFrames[f] != -1)
                std::cout << logs[i].currentFrames[f] << " ";
            else
                std::cout << "- ";
        }
        std::cout << "]\n";
    }

    auto m = algo->getMetrics();
    std::cout << "======================================================\n";
    std::cout << "Algorithm: " << m.algorithmName << " | Frames: " << m.frameCount << "\n";
    std::cout << "References: " << m.totalReferences << " | Hits: " << m.pageHits << " (" << std::fixed << std::setprecision(1) << m.hitRatio << "%)\n";
    std::cout << "Faults: " << m.pageFaults << " (" << m.faultRatio << "%) | Evictions: " << m.evictions << "\n";
    std::cout << "======================================================\n";
}

void runBenchmark() {
    printBanner("Multi-Algorithm Benchmark Comparison");

    std::vector<PageNumber> textbook = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
    std::vector<MemoryReference> refs;
    for (auto p : textbook) refs.emplace_back(p, AccessType::READ);

    size_t frames = 3;
    std::cout << "Reference String: 7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1\n";
    std::cout << "Physical Frames Allocation: " << frames << "\n\n";

    std::vector<std::unique_ptr<PageReplacementAlgorithm>> algos;
    algos.emplace_back(new OptimalReplacement(frames));
    algos.emplace_back(new LRUReplacement(frames));
    algos.emplace_back(new LFUReplacement(frames));
    algos.emplace_back(new FIFOReplacement(frames));
    algos.emplace_back(new MFUReplacement(frames));

    std::cout << std::left
              << std::setw(18) << "Algorithm"
              << std::setw(8)  << "Refs"
              << std::setw(8)  << "Hits"
              << std::setw(8)  << "Faults"
              << std::setw(10) << "Hit %"
              << std::setw(10) << "Fault %"
              << std::setw(12) << "Evictions"
              << "\n";
    std::cout << std::string(74, '-') << "\n";

    for (auto& a : algos) {
        a->runSimulation(refs);
        auto m = a->getMetrics();
        std::cout << std::left
                  << std::setw(18) << m.algorithmName
                  << std::setw(8)  << m.totalReferences
                  << std::setw(8)  << m.pageHits
                  << std::setw(8)  << m.pageFaults
                  << std::setw(10) << (std::to_string(static_cast<int>(m.hitRatio)) + "%")
                  << std::setw(10) << (std::to_string(static_cast<int>(m.faultRatio)) + "%")
                  << std::setw(12) << m.evictions
                  << "\n";
    }
    std::cout << std::string(74, '=') << "\n";
}

void runBelady() {
    printBanner("Bélády's Anomaly Verification & Analysis");

    auto canonical = BeladyAnomalyDetector::getCanonicalString();
    std::cout << "Testing Sequence: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5 across 2 to 6 frames:\n\n";

    BeladyResult res = BeladyAnomalyDetector::verify(canonical, 2, 6);

    std::cout << std::left
              << std::setw(10) << "Frames"
              << std::setw(14) << "FIFO Faults"
              << std::setw(14) << "LRU Faults"
              << std::setw(14) << "OPT Faults"
              << "Anomaly Status\n";
    std::cout << std::string(72, '-') << "\n";

    for (size_t i = 0; i < res.fifoFaults.size(); ++i) {
        size_t f = res.fifoFaults[i].first;
        size_t fifoF = res.fifoFaults[i].second;
        size_t lruF = res.lruFaults[i].second;
        size_t optF = res.optFaults[i].second;

        std::string status = "";
        if (i > 0 && fifoF > res.fifoFaults[i - 1].second) {
            status = "<-- * ANOMALY DETECTED (+1 fault)! *";
        }

        std::cout << std::left
                  << std::setw(10) << f
                  << std::setw(14) << fifoF
                  << std::setw(14) << lruF
                  << std::setw(14) << optF
                  << status << "\n";
    }
    std::cout << std::string(72, '=') << "\n";

    if (res.anomalyDetected) {
        std::cout << "\n[CONFIRMED] Bélády's Anomaly occurred in FIFO:\n";
        std::cout << "  Increasing frames from " << res.anomalyFromFrames << " to " << res.anomalyToFrames
                  << " caused page faults to INCREASE from " << res.anomalyFromFaults << " to " << res.anomalyToFaults << "!\n";
        std::cout << "[STACK ALGORITHM PROOF] LRU & Optimal faults never increase, confirming Stack Inclusion.\n";
    }
}

void runVMemDemo() {
    printBanner("Virtual Memory Demand Paging & Address Translation Demo");
    std::cout << "Simulating MMU Virtual Address (VPN + Offset) -> Physical Address (PFN + Offset)\n\n";

    auto lru = std::unique_ptr<PageReplacementAlgorithm>(new LRUReplacement(3));
    VirtualMemoryEngine engine(3, std::move(lru), 4096);

    std::vector<std::pair<VirtualAddress, AccessType>> stream = {
        {0x0100, AccessType::READ},  // VPN 0 -> Page Fault -> Frame 0
        {0x1200, AccessType::READ},  // VPN 1 -> Page Fault -> Frame 1
        {0x2400, AccessType::WRITE}, // VPN 2 -> Page Fault -> Frame 2 (Dirty)
        {0x0500, AccessType::READ},  // VPN 0 -> TLB HIT!
        {0x3800, AccessType::READ},  // VPN 3 -> Page Fault -> Memory full, evicts VPN 1
        {0x4100, AccessType::READ}   // VPN 4 -> Page Fault -> Evicts VPN 2 (Dirty writeback!)
    };

    std::cout << "Step  Virtual Addr  VPN    Offset  TLB     Page Table    PFN    Physical Addr\n";
    std::cout << "----------------------------------------------------------------------------\n";
    int step = 1;
    for (const auto& item : stream) {
        auto t = engine.accessMemory(item.first, item.second);
        std::stringstream ssVa, ssPa;
        ssVa << "0x" << std::hex << std::uppercase << t.vaddr;
        ssPa << "0x" << std::hex << std::uppercase << t.paddr;

        std::string tlb = t.tlbHit ? "HIT " : "MISS";
        std::string pt = t.pageFault ? "PAGE FAULT" : "HIT       ";

        std::cout << std::left << std::setw(6) << step++
                  << std::setw(14) << ssVa.str()
                  << std::setw(7) << t.vpn
                  << std::setw(8) << t.offset
                  << std::setw(8) << tlb
                  << std::setw(14) << pt
                  << std::setw(7) << t.pfn
                  << ssPa.str();
        if (t.dirtyWriteback) std::cout << " (Dirty WB)";
        std::cout << "\n";
    }
    std::cout << "============================================================================\n";
}

void runTests() {
    printBanner("Running Unit Test Suite");

    // Test 1: DoublyLinkedList correctness
    {
        DoublyLinkedList<int> dll;
        assert(dll.empty());
        auto* n1 = dll.push_back(10);
        auto* n2 = dll.push_back(20);
        dll.push_front(5);
        assert(dll.size() == 3);
        dll.move_to_front(n2);
        assert(dll.getHead()->data == 20);
        dll.remove(n1);
        assert(dll.size() == 2);
        std::cout << "[PASS] DoublyLinkedList O(1) operations\n";
    }

    // Test 2: Standard Reference String
    {
        std::vector<PageNumber> textbook = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
        std::vector<MemoryReference> refs;
        for (auto p : textbook) refs.emplace_back(p, AccessType::READ);

        FIFOReplacement fifo(3); fifo.runSimulation(refs);
        assert(fifo.getMetrics().pageFaults == 15);

        LRUReplacement lru(3); lru.runSimulation(refs);
        assert(lru.getMetrics().pageFaults == 12);

        OptimalReplacement opt(3); opt.runSimulation(refs);
        assert(opt.getMetrics().pageFaults == 9);

        std::cout << "[PASS] Page fault verification (FIFO: 15, LRU: 12, OPT: 9 faults)\n";
    }

    // Test 3: Bélády's Anomaly Verification
    {
        auto can = BeladyAnomalyDetector::getCanonicalString();
        FIFOReplacement f3(3); f3.runSimulation(can);
        FIFOReplacement f4(4); f4.runSimulation(can);
        assert(f3.getMetrics().pageFaults == 9);
        assert(f4.getMetrics().pageFaults == 10);
        std::cout << "[PASS] Bélády's Anomaly in FIFO (3 frames: 9 -> 4 frames: 10 faults)\n";
    }

    std::cout << "\n>>> ALL TESTS PASSED! <<<\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--test" || arg == "-t") { runTests(); return 0; }
        if (arg == "--belady" || arg == "-b") { runBelady(); return 0; }
        if (arg == "--benchmark" || arg == "-m") { runBenchmark(); return 0; }
        if (arg == "--vmem" || arg == "-v") { runVMemDemo(); return 0; }
        if (arg == "--demo" || arg == "-d") {
            runTests(); runBelady(); runBenchmark(); runVMemDemo(); return 0;
        }
    }

    while (true) {
        printBanner("OS Memory Simulator · Virtual Memory & Page Replacement Engine");
        std::cout << "  1. Step-by-Step Simulation (Live Memory Frames State)\n";
        std::cout << "  2. Benchmark All 5 Algorithms (FIFO, LRU, LFU, MFU, Optimal)\n";
        std::cout << "  3. Verify Bélády's Anomaly (Spike Discovery in FIFO)\n";
        std::cout << "  4. Virtual Memory Address Translation Demo (MMU + TLB)\n";
        std::cout << "  5. Run Automated Unit Tests\n";
        std::cout << "  6. Exit\n";
        std::cout << "\nEnter choice (1-6): ";

        std::string input;
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        int choice = 0; try { choice = std::stoi(input); } catch (...) { choice = 0; }
        if (choice == 1) runStepByStep();
        else if (choice == 2) runBenchmark();
        else if (choice == 3) runBelady();
        else if (choice == 4) runVMemDemo();
        else if (choice == 5) runTests();
        else if (choice == 6) break;
        else std::cout << "Invalid choice.\n";

        std::cout << "\nPress Enter to return to menu...";
        std::string dummy; std::getline(std::cin, dummy);
    }
    return 0;
}
