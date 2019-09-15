#ifndef _file_manager_
#define _file_manager_

#include <iostream>
#include <fstream>
#include <cerrno>
#include <unistd.h>
// #include <cstdio>
#include <cstdlib>
// #include <string>
#include <sys/types.h>

#ifndef READ_ONLY
#define READ_ONLY std::ios::in
#endif
#ifndef WRITE_ONLY
#define WRITE_ONLY std::ios::out
#endif
#ifndef APPEND_ONLY
#define APPEND_ONLY std::ios::app
#endif
#ifndef TRUNC_ONLY
#define TRUNC_ONLY std::ios::trunc
#endif

#ifndef KEEP_OPEN
#define KEEP_OPEN true
#endif

struct File {
	std::string _filename;
	std::fstream _fs;
	std::ios::openmode _mode;

	File(std::string filename, std::ios::openmode mode);

	File& open();
	File& close();
	File& read(char* buf, size_t nbytes);
	File& write(const char* buf, size_t nbytes);
	File& truncate(bool leaveOpen);
};

class FileManager
{
private:
	std::string _filename;
	FILE* _ofile;
public:
	FileManager(std::string filename);
	~FileManager();
	
	int readBytesFromFileToBuffer(char* buf, size_t nbytes);
	int writeBytesFromBufferToFile(const char* buf, size_t nbytes);
	int readBytesFromSocketToBuffer(int socket, char* buf, size_t nbytes);
	int writeBytesFromBufferToSocket(const char* buf, size_t nbytes, int socket);
};
#endif