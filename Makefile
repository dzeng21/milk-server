# Compiler and flags
CXX = g++
CXXFLAGS = -g -Wall -std=c++23  # -g for debug symbols, -Wall for all warnings

# Default target
all: milk_server milk_client matrix_test

# Build milk_server
milk_server: milk_server.cpp milk_server.h milk_matrix.h
	$(CXX) $(CXXFLAGS) -o milk_server milk_server.cpp -lssl -lcrypto

# Build milk_client
milk_client: milk_client.cpp
	$(CXX) $(CXXFLAGS) -o milk_client milk_client.cpp

# Build matrix tests
matrix_test: milk_matrix_test.cpp
	$(CXX) $(CXXFLAGS) -o milk_matrix_test milk_matrix_test.cpp


# Clean rule to remove executables
clean:
	rm -f milk_server milk_client

.PHONY: all clean