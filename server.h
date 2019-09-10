#ifndef _SERVER
#define _SERVER

#include <string>

class server
{
private:
	const char* filedir;
	const char* port;
	int listen_fd;
	int fileno;

protected:
	static void sigHandler(int signum);
	const char* getArg(const char* arg);
	const char* checkPortNo(const char* arg);

public:
	server();
	server(int argc, char* argv[]);
	~server();
	
	void usage();
	int start();

	int getListener();
	std::string nextFilename();
	int write(const void* buf, size_t nbyte);
	static void handleConnection(int clientfd, std::string file);
};

#endif

