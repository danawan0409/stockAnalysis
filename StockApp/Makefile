#
# Compiler and flags
#
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -Iinclude

#
# Libraries
#
LIBS = -lpqxx -lpq

#
# Source files and build output
#
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = stockapp

#
# Default target
#
all: $(TARGET)

#
# Link all object files to build final executable
#
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

#
# Compile .cpp files to .o files
#
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

#
# Clean build
#
clean:
	rm -f $(TARGET) src/*.o
