# Compiler and flags
CXX = g++
CXXFLAGS = -g -Wall -std=c++23  # -g for debug symbols, -Wall for all warnings

# Default target
all: milk_server milk_client

# Build milk_server
milk_server: milk_server.cpp milk_server.h
	$(CXX) $(CXXFLAGS) -o milk_server milk_server.cpp

# Build milk_client
milk_client: milk_client.cpp
	$(CXX) $(CXXFLAGS) -o milk_client milk_client.cpp

# Clean rule to remove executables
clean:
	rm -f milk_server milk_client

.PHONY: all clean