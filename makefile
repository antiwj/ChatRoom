# Makefile for ChatRoom
# Compiler
CXX = g++

# Compiler thread argc
CXXFLAGS = -pthread

# Library
LIB = -lncurses

# Targets
TARGETS = client server

# Build all targets
all: $(TARGETS)

# Build client
client: client.cpp
	$(CXX) client.cpp -o client $(CXXFLAGS) $(LIBS)

# Build server
server: server.cpp
	$(CXX) server.cpp -o server $(CXXFLAGS)

clean:
	rm -f $(TARGETS)
