# OS Memory Simulator — Virtual Memory & Page Replacement Engine

[![C++14](https://img.shields.io/badge/Language-C%2B%2B14-blue.svg)](https://isocpp.org/)
[![OS-Concepts](https://img.shields.io/badge/Domain-OS%20Memory%20Management-purple.svg)](https://en.wikipedia.org/wiki/Virtual_memory)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

A clean, high-performance **Virtual Memory Simulator & Page Replacement Engine** in modern C++. Designed to demonstrate operating system memory management principles, including **demand paging, address translation (VPN to PFN), TLB caching, page fault handling, and Bélády's anomaly verification**.

---

## 🌟 Key Features

- **Demand Paging & Page Fault Handling**: Simulates the MMU interrupt cycle, page table updates, frame allocation, and simulated swap disk latency.
- **Address Translation Pipeline**: Translates Virtual Address $(\text{VPN} + \text{Offset})$ to Physical Address $(\text{PFN} + \text{Offset})$ with TLB caching.
- **$O(1)$ LRU & LFU Implementations**: Uses a custom **Doubly Linked List** and **Hash Maps** for constant-time $O(1)$ access, promotion, and eviction.
- **Bélády's Anomaly Verification**: Empirically proves that increasing FIFO frames from 3 to 4 causes page faults to **increase from 9 to 10**, while mathematically verifying that Stack Algorithms (**LRU & Optimal**) remain immune.
- **Comprehensive Benchmarks**: Evaluates **FIFO, LRU, LFU, MFU, and Optimal** algorithms across memory workloads.
- **Dual Interface**: Interactive terminal CLI (with live ASCII memory frame state tables) and a clean web dashboard (`web/index.html`).

---

## 🛠️ Tech Stack & Data Structures

- **Language**: Modern C++ (C++14)
- **Data Structures**:
  - `DoublyLinkedList<T>`: Handcrafted templated doubly linked list supporting $O(1)$ node detachment, prepend, append, and move-to-front.
  - `std::unordered_map`: Hash map for $O(1)$ page lookups in LRU, LFU, and Page Table.
  - `Frequency Buckets`: Hash map of doubly linked lists for $O(1)$ LFU frequency updates.
- **OS Concepts**: Virtual Memory, Translation Lookaside Buffer (TLB), Page Table Entries (PTE), Page Fault Interrupts, Dirty Writebacks.

---

## 📊 Algorithmic Complexity

| Algorithm | Access Time | Eviction Time | Space Complexity | Description |
| :--- | :---: | :---: | :---: | :--- |
| **FIFO** | $O(1)$ | $O(1)$ | $O(F)$ | Evicts the earliest arrived page in physical memory. |
| **LRU** | **$O(1)$** | **$O(1)$** | $O(F)$ | Custom Doubly Linked List + Hash Map; evicts least recently accessed page. |
| **LFU** | **$O(1)$** | **$O(1)$** | $O(F)$ | Frequency buckets + Doubly Linked List; evicts least frequently referenced page. |
| **MFU** | $O(F)$ | $O(F)$ | $O(F)$ | Evicts most frequently used page under high memory utilization. |
| **Optimal (OPT)** | $O(1)$ | $O(N \cdot F)$ | $O(F)$ | Bélády's MIN lookahead; establishes the theoretical minimum page faults. |

---

## 🔬 Bélády's Anomaly Verification

Using the canonical sequence: `1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5`

```
Frames    FIFO Faults   LRU Faults    OPT Faults    Bélády Anomaly Status
------------------------------------------------------------------------
2         12            12            9             
3         9             10            7             
4         10            8             6             <-- * ANOMALY DETECTED (+1 fault)! *
5         5             5             5             
6         5             5             5             
```

- **Why it happens**: FIFO does not satisfy the *Stack Inclusion Property* ($S_m \not\subseteq S_{m+1}$).
- **Stack Algorithms**: LRU and Optimal guarantee monotonic non-increasing faults as frame capacity grows.

---

## 📂 Project Structure

```
os_project/
│
├── include/
│   ├── DoublyLinkedList.hpp    # Handcrafted O(1) Doubly Linked List template
│   ├── PageReplacement.hpp     # Base class & algorithms (FIFO, LRU, LFU, MFU, Optimal)
│   └── VirtualMemory.hpp       # Demand Paging, Page Table, TLB & Belady's Anomaly
│
├── src/
│   ├── PageReplacement.cpp     # Implementation of all 5 replacement algorithms
│   ├── VirtualMemory.cpp       # Implementation of Virtual Memory & Belady Anomaly
│   └── main.cpp                # Interactive CLI menu & benchmark comparisons
│
├── web/                        # Web Visualizer Dashboard
│   ├── index.html              # Clean HTML
│   ├── style.css               # Styling & frame animations
│   └── app.js                  # Simulation & canvas chart logic
│
├── build.ps1                   # Build script
├── Makefile                    # Cross-platform build file
├── README.md                   # Project documentation
└── .gitignore
```

---

## 🚀 How to Compile & Run

### 1. Single Command Build
```powershell
# Using PowerShell build script:
.\build.ps1

# Or standard g++ command:
g++ -std=c++14 -O2 -Iinclude src/PageReplacement.cpp src/VirtualMemory.cpp src/main.cpp -o vmem_simulator.exe
```

### 2. Run
```powershell
.\vmem_simulator.exe

# Or run automated tests / verification directly:
.\vmem_simulator.exe --test      # Runs built-in test suite
.\vmem_simulator.exe --belady    # Runs Bélády's anomaly verification
.\vmem_simulator.exe --benchmark # Runs benchmark comparison
.\vmem_simulator.exe --demo      # Runs full tour
```

---

## 📜 License
Distributed under the [MIT License](LICENSE).
