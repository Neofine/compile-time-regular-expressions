CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -Wall
LDFLAGS = -lpcre2-8

# Include paths
INCLUDES = -I/usr/local/include -I..

TARGET = benchmark
SRC = benchmark.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "Building match benchmark..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) $(LDFLAGS) -o $(TARGET)
	@echo "✅ Build complete"

clean:
	rm -f $(TARGET) match_benchmark_results.csv

run: $(TARGET)
	@echo "Running match benchmark..."
	@./$(TARGET)

.PHONY: all clean run
