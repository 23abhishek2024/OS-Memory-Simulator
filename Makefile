CXX = g++
CXXFLAGS = -std=c++14 -O2 -Wall -Wextra -Iinclude

SRCS = src/FIFOReplacement.cpp \
       src/LRUReplacement.cpp \
       src/LFUReplacement.cpp \
       src/MFUReplacement.cpp \
       src/OptimalReplacement.cpp \
       src/ClockReplacement.cpp \
       src/PageTable.cpp \
       src/TLB.cpp \
       src/VirtualMemoryEngine.cpp \
       src/BeladyAnomalyDetector.cpp \
       src/BenchmarkRunner.cpp \
       src/Visualizer.cpp \
       src/main.cpp

TARGET = vmem_simulator.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p logs
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

test: $(TARGET)
	./$(TARGET) --test

benchmark: $(TARGET)
	./$(TARGET) --benchmark

belady: $(TARGET)
	./$(TARGET) --belady

demo: $(TARGET)
	./$(TARGET) --demo

clean:
	rm -f $(TARGET)
