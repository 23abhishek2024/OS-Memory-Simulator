CXX = g++
CXXFLAGS = -std=c++14 -O2 -Iinclude

SRCS = src/PageReplacement.cpp src/VirtualMemory.cpp src/main.cpp
TARGET = vmem_simulator.exe

all: $(TARGET)

$(TARGET): $(SRCS)
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
