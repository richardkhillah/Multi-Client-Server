

#ifndef _client
#define _client

class client
{
private:
	std::string hostname;
	std::string port;
	std::string filename;
	FILE* fstream;

protected:
	int sockfd;
	int status;

	std::string getArg(const char* arg);
	std::string checkPortNo(std::string arg);

	void setupHints(struct addrinfo& hints);
	struct addrinfo* getAddrInfo(struct addrinfo& hints);
	ushort sleepForOneSecond();
	bool timedOut(ushort secondsAsleep);
	int createSocketAndConnect(struct addrinfo* results);
	void initializeNetworkSettings();

	int getSockFd();
	int readBytesFromFileToBuffer(FILE* file, char* buf, unsigned long nbyte);
	int writeBytesFromBufferToSocket(char* buf, unsigned long nbyte, int socket);
	FILE* openFile();
	void sendFileOverNetworkSocket(int socket);

public:
	client();			// done
	client(int argc, char* argv[]);
	~client();

	void usage();		// done
	void run();	
};

#endif

