CXX=g++
CXXOPTIMIZE=-O2
CXXFLAGS=-g -Wall -Wextra -std=c++11 $(CXXOPTIMIZE)
EXT=cpp
UID=604853262

all: server client

server: 
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp FileManager.cpp

client: 
	$(CXX) $(CXXFLAGS) -o $@ $@.cpp 

clean:
	rm -rf server client *.dSYM *.tar.gz ./savedir/ test* FileManager

test: FileManager
	mkdir ./savedir

dist: clean
	tar -cvzf $(UID).tar.gz server.* client.* Makefile README.txt
# 	TODO: add report.pdf to dist

FileManager:
	$(CXX) $(CXXFLAGS) -DTEST -o $@ $@.cpp 