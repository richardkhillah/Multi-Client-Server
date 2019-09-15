#ifndef _SERVER
#define _SERVER

#include <string>
#include "FileManager.h"

struct PollingInfo {
	int nfds;
	fd_set* readfds;
	fd_set* writefds;
	fd_set* exceptfds;
	struct timeval* timeout;

	PollingInfo(int n, 
							fd_set* rfds, 
							fd_set* wfds, 
							fd_set* efds, 
							struct timeval* to) 
							: 
							nfds(n), 
							readfds(rfds), 
							writefds(wfds), 
							exceptfds(efds), 
							timeout(to) {}
};

class server
{
private:
	std::string filedir;
	std::string port;
	int listen_fd;

protected:
	static void sigHandler(int signum);
	std::string getArg(const char* arg);
	std::string checkPortNo(std::string arg);

	void setupHints(struct addrinfo& hints);
	struct addrinfo* getAddrInfo(struct addrinfo& hints);
	int createSocketBindToAddress(struct addrinfo* results);
	void initializeNetworkSettings();

	int getListener();
	void pollFileDescriptors(struct PollingInfo info);
	int acceptClient(int socket);
	std::string nextFilename();
	bool timedOut(ushort secondsAsleep);
	static void handleConnection(int clientfd, std::string file);

public:
	server();
	server(int argc, char* argv[]);
	~server();
	void run();
	void usage();

};

#endif

