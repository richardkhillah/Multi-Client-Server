

#ifndef _client
#define _client

class client
{
private:
	const char* hostname;
	const char* port;
	const char* filename;
	FILE* fstream;

protected:
	int sockfd;
	int status;

	const char* getArg(const char* arg);
	const char* checkPortNo(const char* arg);

public:
	client();			// done
	client(int argc, char* argv[]);
	~client();
	

	void usage();		// done
	int connect();		
	FILE* open_file();
	int write(const void* buf, size_t nbyte);
};

#endif

