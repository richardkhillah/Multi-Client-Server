CXX=g++
CXXOPTIMIZE=-O2
CXXFLAGS=-g -Wall -Wextra -std=c++11 $(CXXOPTIMIZE)
EXT=cpp
UID=604853262

all: server client

server: 
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp

client: 
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp 

clean:
	rm -rf server client *.dSYM *.tar.gz

dist: clean
	tar -cvzf $(UID).tar.gz server.* client.* Makefile README.txt
# 	TODO: add report.pdf to dist