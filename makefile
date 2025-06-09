# Compiler and base flags
CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -pthread -lnuma

# Sources and objects
SRCS := Benchmark.cpp Connection.cpp main.cpp
OBJS := $(SRCS:.cpp=.o)

# Output name
TARGET := program

# Default build (optimized)
all: CXXFLAGS += -O3
all: $(TARGET)

run: debug
	./program r &
	./program w

# Debug build (with -g and no optimization)
debug: CXXFLAGS += -g -O0
debug: $(TARGET)

# Linking rule
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilation rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)