#include "MemoryTypes.hpp"
#include "DoublyLinkedList.hpp"
#include "FIFOReplacement.hpp"
#include "LRUReplacement.hpp"
#include "LFUReplacement.hpp"
#include "MFUReplacement.hpp"
#include "OptimalReplacement.hpp"
#include "ClockReplacement.hpp"
#include "VirtualMemoryEngine.hpp"
#include "BeladyAnomalyDetector.hpp"
#include "BenchmarkRunner.hpp"
#include "Visualizer.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cassert>

using namespace vmem;

void runInteractiveSimulator() {
    Visualizer::printHeader("Interactive Step-by-Step Page Replacement Simulator");

    std::cout << "\nSelect Page Replacement Policy:\n";
    std::cout << "  1. FIFO (First-In, First-Out)\n";
    std::cout << "  2. LRU (Least Recently Used) [O(1) DLL + Hash Map]\n";
    std::cout << "  3. LFU (Least Frequently Used) [O(1) Frequency Buckets]\n";
    std::cout << "  4. MFU (Most Frequently Used)\n";
    std::cout << "  5. Optimal (Bélády's Optimal / MIN)\n";
    std::cout << "  6. Clock (Second Chance)\n";
    std::cout << "Enter choice (1-6) [default: 2]: ";

    std::string choiceStr;
    std::getline(std::cin, choiceStr);
    int choice = 2;
    if (!choiceStr.empty()) {
        try { choice = std::stoi(choiceStr); } catch (...) { choice = 2; }
    }

    std::cout << "Enter number of physical frames [default: 3]: ";
    std::string framesStr;
    std::getline(std::cin, framesStr);
    size_t frameCount = 3;
    if (!framesStr.empty()) {
        try { frameCount = std::stoul(framesStr); } catch (...) { frameCount = 3; }
        if (frameCount < 1) frameCount = 1;
    }

    std::cout << "\nSelect Reference String Source:\n";
    std::cout << "  1. OS Textbook Standard: 7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1\n";
    std::cout << "  2. Bélády's Anomaly String: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5\n";
    std::cout << "  3. Locality of Reference Workload (Hot-Cold Access)\n";
    std::cout << "  4. Enter Custom Reference Sequence\n";
    std::cout << "Enter choice (1-4) [default: 1]: ";

    std::string refChoiceStr;
    std::getline(std::cin, refChoiceStr);
    int refChoice = 1;
    if (!refChoiceStr.empty()) {
        try { refChoice = std::stoi(refChoiceStr); } catch (...) { refChoice = 1; }
    }

    std::vector<MemoryReference> refs;

    if (refChoice == 1) {
        std::vector<PageNumber> textbook = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
        for (size_t i = 0; i < textbook.size(); ++i) {
            AccessType t = (i % 4 == 0) ? AccessType::WRITE : AccessType::READ;
            refs.emplace_back(textbook[i], t);
        }
    } else if (refChoice == 2) {
        refs = BeladyAnomalyDetector::getCanonicalReferenceString();
    } else if (refChoice == 3) {
        refs = BenchmarkRunner::generateWorkload(WorkloadType::LOCALITY_OF_REFERENCE, 25, 8);
    } else {
        std::cout << "Enter space or comma separated page numbers (e.g. 1 2 3 4 1 2 5): ";
        std::string customStr;
        std::getline(std::cin, customStr);
        for (char& c : customStr) {
            if (c == ',') c = ' ';
        }
        std::stringstream ss(customStr);
        PageNumber p;
        while (ss >> p) {
            refs.emplace_back(p, AccessType::READ);
        }
        if (refs.empty()) {
            std::cout << "Invalid input. Using default textbook sequence.\n";
            std::vector<PageNumber> textbook = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
            for (auto tp : textbook) refs.emplace_back(tp, AccessType::READ);
        }
    }

    std::unique_ptr<PageReplacementAlgorithm> algo;
    switch (choice) {
        case 1: algo = std::unique_ptr<PageReplacementAlgorithm>(new FIFOReplacement(frameCount)); break;
        case 2: algo = std::unique_ptr<PageReplacementAlgorithm>(new LRUReplacement(frameCount)); break;
        case 3: algo = std::unique_ptr<PageReplacementAlgorithm>(new LFUReplacement(frameCount)); break;
        case 4: algo = std::unique_ptr<PageReplacementAlgorithm>(new MFUReplacement(frameCount)); break;
        case 5: algo = std::unique_ptr<PageReplacementAlgorithm>(new OptimalReplacement(frameCount)); break;
        case 6: algo = std::unique_ptr<PageReplacementAlgorithm>(new ClockReplacement(frameCount)); break;
        default: algo = std::unique_ptr<PageReplacementAlgorithm>(new LRUReplacement(frameCount)); break;
    }

    std::vector<AccessResult> traceLog;
    algo->runSimulation(refs, &traceLog);

    Visualizer::printTraceTable(algo->getName(), refs, traceLog, frameCount);
    Visualizer::printSummaryBox(algo->getMetrics());
}

void runBenchmarkSuite() {
    Visualizer::printHeader("Cross-Algorithm Memory Benchmark Engine");
    std::cout << "Benchmarking FIFO, LRU, LFU, MFU, Optimal (OPT), and Clock...\n\n";

    std::vector<BenchmarkSuiteResult> suites;

    // 1. Locality of Reference
    auto localityRefs = BenchmarkRunner::generateWorkload(WorkloadType::LOCALITY_OF_REFERENCE, 100, 15);
    suites.push_back(BenchmarkRunner::runSuite(
        "Locality of Reference (80/20)",
        "80% references to 20% hot pages, simulating real application memory loops",
        localityRefs, 4));

    // 2. Cyclic Sequential Loop (Thrashing scenario)
    auto cyclicRefs = BenchmarkRunner::generateWorkload(WorkloadType::CYCLIC_LOOP, 50, 5);
    suites.push_back(BenchmarkRunner::runSuite(
        "Cyclic Loop / Thrashing",
        "Cyclic traversal over 5 pages with 4 physical frames (causes frequent evictions)",
        cyclicRefs, 4));

    // 3. Random Uniform Access
    auto randomRefs = BenchmarkRunner::generateWorkload(WorkloadType::RANDOM_UNIFORM, 100, 20);
    suites.push_back(BenchmarkRunner::runSuite(
        "Random Uniform Access",
        "Uniformly distributed random references across 20 distinct pages",
        randomRefs, 5));

    // 4. Bélády's Canonical Sequence
    auto beladyRefs = BeladyAnomalyDetector::getCanonicalReferenceString();
    suites.push_back(BenchmarkRunner::runSuite(
        "Bélády's Anomaly Workload",
        "Standard reference string: [1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5] with 3 frames",
        beladyRefs, 3));

    for (const auto& suite : suites) {
        Visualizer::printBenchmarkTable(suite);
    }

    // Export logs
    BenchmarkRunner::exportResultsToCSV(suites, "logs/benchmark_results.csv");
    BenchmarkRunner::exportResultsToJSON(suites, "logs/benchmark_results.json");
    std::cout << "\n[OK] Benchmark results successfully exported to:\n";
    std::cout << "     - logs/benchmark_results.csv\n";
    std::cout << "     - logs/benchmark_results.json\n";
}

void runBeladyVerification() {
    Visualizer::printHeader("Bélády's Anomaly Verification & Comparative Analysis");

    auto canonical = BeladyAnomalyDetector::getCanonicalReferenceString();
    auto fifoResult = BeladyAnomalyDetector::analyze(canonical, "FIFO", 2, 6);
    auto lruResult = BeladyAnomalyDetector::analyze(canonical, "LRU", 2, 6);
    auto optResult = BeladyAnomalyDetector::analyze(canonical, "Optimal", 2, 6);

    Visualizer::printBeladyAnalysis(fifoResult, lruResult, optResult);

    // Alternative Anomaly string
    std::cout << "\nAnalyzing Secondary Anomaly Sequence (3, 2, 1, 0, 3, 2, 4, 3, 2, 1, 0, 4):\n";
    auto alt = BeladyAnomalyDetector::getAlternativeReferenceString();
    auto altFifo = BeladyAnomalyDetector::analyze(alt, "FIFO", 2, 5);
    auto altLru = BeladyAnomalyDetector::analyze(alt, "LRU", 2, 5);
    auto altOpt = BeladyAnomalyDetector::analyze(alt, "Optimal", 2, 5);
    Visualizer::printBeladyAnalysis(altFifo, altLru, altOpt);
}

void runVirtualMemoryDemo() {
    Visualizer::printHeader("Virtual Memory & Demand Paging MMU Demonstration");
    std::cout << "Simulating MMU Address Translation (VPN + Offset -> PFN + Offset)\n";
    std::cout << "Page Size: 4096 bytes (4 KB) | Physical Frames: 3 | TLB Size: 4\n\n";

    auto lru = std::unique_ptr<PageReplacementAlgorithm>(new LRUReplacement(3));
    VirtualMemoryEngine engine(3, std::move(lru), 4096, 4);

    // Stream of virtual addresses:
    // Page 0: 0x0100 (vaddr 256)
    // Page 1: 0x1200 (vaddr 4608)
    // Page 2: 0x2400 (vaddr 9216)
    // Page 0: 0x0500 (vaddr 1280) -> Hit in TLB!
    // Page 3: 0x3800 (vaddr 14336) -> Fault! Memory full, evicts LRU victim (Page 1)
    // Page 0: 0x0800 (WRITE)
    // Page 4: 0x4100 (vaddr 16640) -> Fault! Evicts Page 2
    // Page 0: 0x0900 (vaddr 2304) -> Hit!
    std::vector<std::pair<VirtualAddress, AccessType>> accesses = {
        {0x0100, AccessType::READ},  // VPN 0
        {0x1200, AccessType::READ},  // VPN 1
        {0x2400, AccessType::WRITE}, // VPN 2 (Dirty)
        {0x0500, AccessType::READ},  // VPN 0 (TLB Hit)
        {0x3800, AccessType::READ},  // VPN 3 (Page Fault, evicts VPN 1)
        {0x0800, AccessType::WRITE}, // VPN 0 (TLB Hit, Write)
        {0x4100, AccessType::READ},  // VPN 4 (Page Fault, evicts VPN 2 which is Dirty -> Writeback!)
        {0x0900, AccessType::READ}   // VPN 0 (TLB Hit)
    };

    for (const auto& acc : accesses) {
        engine.accessMemory(acc.first, acc.second);
    }

    Visualizer::printTranslationTraceTable(engine.getHistory());
    Visualizer::printSummaryBox(engine.getMetrics());
}

void runTestSuite() {
    Visualizer::printHeader("Running Virtual Memory Engine Unit Test Suite");

    // Test 1: DoublyLinkedList correctness
    {
        std::cout << "[TEST 1] Testing DoublyLinkedList operations... ";
        DoublyLinkedList<int> list;
        assert(list.empty());
        assert(list.size() == 0);

        auto* n1 = list.push_back(10);
        auto* n2 = list.push_back(20);
        auto* n3 = list.push_front(5);
        assert(list.size() == 3);
        assert(list.getHead()->data == 5);
        assert(list.getTail()->data == 20);

        list.move_to_front(n2);
        assert(list.getHead()->data == 20);

        list.remove(n1);
        assert(list.size() == 2);
        std::cout << "PASSED\n";
    }

    // Test 2: Standard Textbook Page Fault Count Verification
    // String: 7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1 with 3 frames
    {
        std::cout << "[TEST 2] Verifying standard textbook reference string faults (3 frames)... ";
        std::vector<PageNumber> textbook = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
        std::vector<MemoryReference> refs;
        for (auto p : textbook) refs.emplace_back(p, AccessType::READ);

        FIFOReplacement fifo(3);
        fifo.runSimulation(refs);
        assert(fifo.getMetrics().pageFaults == 15);

        LRUReplacement lru(3);
        lru.runSimulation(refs);
        assert(lru.getMetrics().pageFaults == 12);

        OptimalReplacement opt(3);
        opt.runSimulation(refs);
        assert(opt.getMetrics().pageFaults == 9);

        std::cout << "PASSED (FIFO: 15, LRU: 12, OPT: 9 faults)\n";
    }

    // Test 3: Bélády's Anomaly Verification
    // String: 1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5
    // 3 frames: 9 faults, 4 frames: 10 faults in FIFO!
    {
        std::cout << "[TEST 3] Verifying Bélády's Anomaly in FIFO (3 vs 4 frames)... ";
        auto canonical = BeladyAnomalyDetector::getCanonicalReferenceString();

        FIFOReplacement fifo3(3);
        fifo3.runSimulation(canonical);
        assert(fifo3.getMetrics().pageFaults == 9);

        FIFOReplacement fifo4(4);
        fifo4.runSimulation(canonical);
        assert(fifo4.getMetrics().pageFaults == 10);

        LRUReplacement lru3(3);
        lru3.runSimulation(canonical);
        assert(lru3.getMetrics().pageFaults == 10);

        LRUReplacement lru4(4);
        lru4.runSimulation(canonical);
        assert(lru4.getMetrics().pageFaults == 8);

        std::cout << "PASSED (FIFO: 9 -> 10 [Anomaly!], LRU: 10 -> 8 [Stack Property])\n";
    }

    // Test 4: Address Translation & TLB Test
    {
        std::cout << "[TEST 4] Testing MMU Virtual-to-Physical translation and TLB... ";
        auto lru = std::unique_ptr<PageReplacementAlgorithm>(new LRUReplacement(4));
        VirtualMemoryEngine engine(4, std::move(lru), 4096, 4);

        // Access 0x1050 (VPN 1, offset 0x50)
        auto t1 = engine.accessMemory(0x1050, AccessType::READ);
        assert(t1.pageFault == true);
        assert(t1.vpn == 1);
        assert(t1.offset == 0x50);

        // Access 0x1080 (Same page, VPN 1) -> Must be TLB Hit!
        auto t2 = engine.accessMemory(0x1080, AccessType::READ);
        assert(t2.tlbHit == true);
        assert(t2.pageFault == false);
        assert(t2.pfn == t1.pfn);

        std::cout << "PASSED\n";
    }

    std::cout << "\n>>> ALL UNIT TESTS PASSED SUCCESSFULLY! <<<\n";
    std::cout << "========================================================================================\n";
}

int main(int argc, char* argv[]) {
    // Non-interactive CLI flag mode
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--test" || arg == "-t") {
            runTestSuite();
            return 0;
        } else if (arg == "--belady" || arg == "-b") {
            runBeladyVerification();
            return 0;
        } else if (arg == "--benchmark" || arg == "-m") {
            runBenchmarkSuite();
            return 0;
        } else if (arg == "--vmem" || arg == "-v") {
            runVirtualMemoryDemo();
            return 0;
        } else if (arg == "--demo" || arg == "-d") {
            runTestSuite();
            runBeladyVerification();
            runVirtualMemoryDemo();
            runBenchmarkSuite();
            return 0;
        }
    }

    // Interactive menu mode
    while (true) {
        Visualizer::printHeader("Virtual Memory & Page Replacement Engine");
        std::cout << "  [1] Interactive Step-by-Step Simulator (Trace Frames in Real Time)\n";
        std::cout << "  [2] Comprehensive Multi-Algorithm Benchmark Suite (FIFO, LRU, LFU, MFU, OPT, Clock)\n";
        std::cout << "  [3] Bélády's Anomaly Verification & Comparative Visualizer\n";
        std::cout << "  [4] Virtual Memory & Demand Paging MMU Address Translation Demo\n";
        std::cout << "  [5] Run Built-in Unit Test Suite\n";
        std::cout << "  [6] Exit\n";
        std::cout << "\nEnter selection [1-6]: ";

        std::string input;
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        int choice = 0;
        try { choice = std::stoi(input); } catch (...) { choice = 0; }

        switch (choice) {
            case 1: runInteractiveSimulator(); break;
            case 2: runBenchmarkSuite(); break;
            case 3: runBeladyVerification(); break;
            case 4: runVirtualMemoryDemo(); break;
            case 5: runTestSuite(); break;
            case 6:
                std::cout << "Exiting Virtual Memory Engine. Goodbye!\n";
                return 0;
            default:
                std::cout << "Invalid choice. Please select from 1 to 6.\n";
                break;
        }

        std::cout << "\nPress Enter to return to main menu...";
        std::string pause;
        std::getline(std::cin, pause);
    }

    return 0;
}
