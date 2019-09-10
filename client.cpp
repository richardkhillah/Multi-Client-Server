#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <thread>
#include <chrono>

#include <iostream>
#include "client.h"

#ifndef ARG_ERROR
#define ARG_ERROR 1
#endif

#ifndef CXN_ERROR
#define CXN_ERROR 2
#endif

#ifndef IOERROR
#define IOERROR 3
#endif

#define TIMEOUT 15 // seconds

client::client() : hostname(nullptr), port(nullptr), filename(nullptr) {}

client::client(int argc, char* argv[])
{
	// We should have exactly 4 arguments
  if (argc != 4) {
    std::cerr << "ERROR: Incorrect number of arguments." << std::endl;
    usage();
    exit(ARG_ERROR);
  }

  hostname = getArg(argv[1]);
  if (hostname.empty()) {
    std::cerr << "ERROR: Unable to get HOSTNAME-OR-IP" << std::endl;
    exit(ARG_ERROR);
  }

  port = getArg(argv[2]);
  port = checkPortNo(port);
  if (port.empty()) {
    std::cerr << "ERROR: invalid PORT number" << std::endl;
    exit(ARG_ERROR);
  }

  // Then we move onto getting the dirName
  filename = getArg(argv[3]);
  if (filename.empty()) {
    std::cerr << "ERROR: Unable to get FILENAME" << std::endl;
    exit(ARG_ERROR);
  }
}

client::~client()
{
  fclose(fstream);
  close(sockfd);
}

std::string client::getArg(const char* arg)
{
	return arg != nullptr ? (const char*)arg : nullptr;
}

std::string client::checkPortNo(std::string arg)
{
	return stoi(arg) > 1023 ? arg : nullptr;
}

int client::getSockFd() {return sockfd;}

void client::usage()
{
  std::cerr << "Usage: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\n";
  std::cerr << "  <HOSTNAME-OR-IP> hostname or IP address of the server to connect with.\n";
  std::cerr << "  <PORT>           port number of the server to connect with.\n";
  std::cerr << "  <FILENAME>       name of the file to transfer to the server\n";
}

void client::setupHints(struct addrinfo& hints)
{
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;
}

struct addrinfo* client::getAddrInfo(struct addrinfo& hints)
{
  int status;
  struct addrinfo* res;
  if ((status = getaddrinfo(hostname.c_str(), (const char*)port.c_str(), &hints, &res)) != 0) {
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }
  return res;
}

ushort client::sleepForOneSecond()
{
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return 1;
}

bool client::timedOut(ushort secondsAsleep) 
{
  return TIMEOUT <= secondsAsleep;
}

int client::createSocketAndConnect(struct addrinfo* results)
{
  struct addrinfo* rp;
  int fd;
  int status = -1;
  int secondsAsleep = 0;
  while( !timedOut(secondsAsleep) && (status == -1) ) {
    for( rp = results; rp != nullptr; rp = rp->ai_next ) {
      fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if( fd == -1)
        continue;

      status = ::connect(fd, rp->ai_addr, rp->ai_addrlen);
      if( status != -1 )
        break;
      else 
        close( fd );
    }// end for

    if( status == -1 )
      secondsAsleep += sleepForOneSecond();
  }

  if( timedOut(secondsAsleep) ) {
    errno = ETIMEDOUT;
    perror("ERROR");
    exit(TIMEOUT);
  }

  // check errno for other errors before reseting
  errno = 0;
  return fd;
}
void client::initializeNetworkSettings()
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  setupHints(hints);
  struct addrinfo* results = getAddrInfo(hints);
  sockfd = createSocketAndConnect(results);
  freeaddrinfo(results);
}

FILE* client::openFile()
{
  fstream = fopen(filename.c_str(), "r");
  if (!fstream) {
    std::cerr << "ERROR: Unable to open file.\n";
    exit(2);
  }
  return fstream;
}

const unsigned short CHUNK = 1024;

int client::readBytesFromFileToBuffer(FILE* file, char* buf, unsigned long nbyte)
{
  int bytesRead = fread(buf, sizeof(char), nbyte-1, file);
  if ( bytesRead == -1 ) {
    perror("ERROR");
    // fclose(file);
    exit(IOERROR);
  } 
  return bytesRead;
}

int client::writeBytesFromBufferToSocket(char* buf, unsigned long nbyte, int socket)
{
  int bytesWritten = ::write(socket, buf, nbyte);
  if (bytesWritten == -1) {
    perror("ERROR");
    exit(IOERROR);
  }
  return bytesWritten;
}

void client::sendFileOverNetworkSocket(int socket)
{
  FILE* file = openFile();

  int bytesRead; 
  int bytesWritten;

  while (true) {
    char buf[CHUNK];
    bzero(buf, CHUNK);
    
    bytesRead = readBytesFromFileToBuffer(file, buf, CHUNK-1);
    if( bytesRead == 0 ) {
      break;
    } else {
      bytesWritten = writeBytesFromBufferToSocket(buf, CHUNK-1, socket);
      if( bytesWritten > 0 )
        continue;
      else {
        std::cerr << "Error writing bytes\n";
        exit(-1);
      }
    }
  }
}

void client::run()
{
  initializeNetworkSettings();
  sendFileOverNetworkSocket(getSockFd());
}

int
main(int argc, char* argv[])
{

  client c = client(argc, argv);

  c.run();

  exit(EXIT_SUCCESS);
}