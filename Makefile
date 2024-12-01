# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Executables
EXECUTABLES = client serverM serverA serverR serverD

# Default target
all: $(EXECUTABLES)

# Rules to build each executable
client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

serverM: serverM.cpp
	$(CXX) $(CXXFLAGS) -o serverM serverM.cpp

serverA: serverA.cpp
	$(CXX) $(CXXFLAGS) -o serverA serverA.cpp

serverR: serverR.cpp
	$(CXX) $(CXXFLAGS) -o serverR serverR.cpp

serverD: serverD.cpp
	$(CXX) $(CXXFLAGS) -o serverD serverD.cpp

# Clean up generated files
clean:
	rm -f $(EXECUTABLES) *.o
