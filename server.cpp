#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
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
#include <mutex>

#include <iostream>
#include "server.h"

#ifndef OK
#define OK 0
#endif

#ifndef ARG_ERROR
#define ARG_ERROR 1
#endif

#ifndef TIMEOUT
#define TIMEOUT 15
#endif

const unsigned short CHUNK = 1024;

server::server() : filedir(nullptr), port(nullptr), listen_fd(0) {}

server::server(int argc, char* argv[]) : listen_fd(0)
{
	// This Program excepts 2 and only two arguments
	if (argc != 3) {
		std::cerr << "ERROR: Incorrect number of arguments." << std::endl;
		usage();
		exit(ARG_ERROR);
	}

	// Get our port number
	port = getArg(argv[1]);
	port = checkPortNo(port);
	if ( port.empty() ) {
		std::cerr << "ERROR: invalid port number" << std::endl;
		exit(ARG_ERROR);
	}

	// and the directory to safe files.
	filedir = getArg(argv[2]);
	if ( filedir.empty() ) {
		std::cerr << "ERROR: Unable to get FILE-DIR" << std::endl;
		exit(ARG_ERROR);
	}

	::signal(SIGTERM, server::sigHandler);
  ::signal(SIGQUIT, server::sigHandler);
}

server::~server()
{
	close(listen_fd);
}


void server::sigHandler(int signum)
{
	switch (signum) {
    // Fallthrough
    case SIGTERM: {
      std::cout << "Termination signal (" << signum << ") received.\n";
    }
    case SIGQUIT: {
      std::cout << "Quit program signal (" << signum << ") received.\n";
      exit(EXIT_SUCCESS);
      break;
    }
  }
}

std::string server::getArg(const char* arg)
{
	return arg != nullptr ? (const char*)arg : nullptr;
}

std::string server::checkPortNo(std::string arg)
{
	return stoi(arg) > 1023 ? arg : nullptr;
}

void server::usage()
{
	std::cerr << "Usage: ./server <PORT> <FILE-DIR>\n";
  	std::cerr << "  <PORT>      port number to listen on connections.\n";
  	std::cerr << "  <FILE-DIR>  directory name where to save the received files\n";
}

void server::setupHints(struct addrinfo& hints) 
{
  hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP connection only
  hints.ai_flags = AI_PASSIVE;  // For wildcard IP address
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;
}

struct addrinfo* server::getAddrInfo(struct addrinfo& hints)
{ 
  int status;
  struct addrinfo* res;
  if ((status = getaddrinfo(nullptr, (const char*) port.c_str(), &hints, &res)) != 0) {
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }
  return res;
}

int server::createSocketBindToAddress(struct addrinfo* results)
{
  struct addrinfo* rp;
  int fd;

  // make a socket on first available address, either IPv4 or IPv6
  for (rp = results; rp != nullptr; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd == -1)
      continue;

    if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      int yes = 1;  // allow the port to be reused then move on.
      if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
      }
      break;
    }

    close(fd);
  }

  // Check that a binding occured
  if (rp == nullptr) {
    // TODO: throw exception instead
    std::cerr << "Unable to successfully bind" << std::endl;
    exit(EXIT_FAILURE);
  }

  return fd;
}

void server::initializeNetworkSettings()
{
	// Stuff for getting address information
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  setupHints(hints);
  struct addrinfo* results = getAddrInfo(hints);
  listen_fd = createSocketBindToAddress(results);
  freeaddrinfo(results); // we've bound, so we're done with this

  if (listen(listen_fd, 1) == -1) {
    perror("listen");
    exit(3);
  }
}

int server::getListener() {	return listen_fd;	}

std::string server::nextFilename()
{
  static unsigned short filenum = 0;
  std::string prefix = "./";
  const char* postfix = ".file";
  return (prefix + filedir + std::to_string(++filenum) + postfix);
}

void server::handleConnection(int clientfd, std::string file)
{
   
  std::cerr << "file = " << file << std::endl;
  File of(file, WRITE_ONLY);

  // Keep track of progress
  int bytesRead = 0;
  int bytesWritten = 0;
  int newCount = 0;
  int totalBytesRead = 0;
  // int totalBytesWritten = 0;

  while (true) {
    char buf[CHUNK+1];
    bzero(buf, CHUNK+1);

    bytesRead = recv(clientfd, buf, CHUNK, MSG_DONTWAIT); // non-blocking 

    if (errno == EWOULDBLOCK) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      newCount++;
      errno = 0;

      if( newCount == TIMEOUT ) {
        errno = ETIMEDOUT;
        perror("ERROR");
        if (totalBytesRead > 0) {
          of.truncate(KEEP_OPEN).write("ERROR", strlen("ERROR"));
        } else {
          std::cerr << "ERROR: No data sent from client.\n";
        } // end if totalBytesRead
        break;
      } // end if TIMEOUT
    } else {
      errno = 0;    // reset so we don't misinterpret later
      newCount = 0; 

      if ( bytesRead > 0 ) {
        totalBytesRead += bytesRead;
        of.write(buf, bytesRead);
      } else if (bytesRead == 0) {
        break;  // eof reached and client closed cxn
      } else {
        perror("ERROR");
        exit(-1);
      }// end else
    } // end else
  } // end while
  of.close();
  close(clientfd);
}

void server::pollFileDescriptors(struct PollingInfo info)
{
  int select_status = select(info.nfds, info.readfds, info.writefds, info.exceptfds, info.timeout);
  if (select_status == -1) {
    perror("Select");
    exit(-10);
  } else if (select_status == 0) {
    // TODO: update this with correct handling
    // We should never get here, but if we do, let console know.
    std::cerr << "ERROR: TIMEOUT occured on main thread" << std::endl;
    // close (listen_fd);
    exit(-1);
  }
}

int server::acceptClient(int socket)
{
  struct sockaddr_in clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  int clientfd = accept(socket, (struct sockaddr*)&clientAddr, &clientAddrSize);
  if (clientfd == -1) {
    perror("ERROR");
    // TODO update this with correct handling
    exit(-1);
  }

  return clientfd;
}

bool desiredFDIsSet(int testfd, int requiredFD, fd_set* fdset)
{
  return (FD_ISSET(testfd, fdset) && (testfd == requiredFD));
}

void server::run()
{
  // open connection and listen
  initializeNetworkSettings();

  // Get ready to work. 
  fd_set master;
  FD_ZERO(&master);
  int listen_fd = getListener();
  FD_SET(listen_fd, &master);

  // Main accept/dispatch loop
  for (;;) {
    // Always make a copy. Select alters readfd
    fd_set readfd;
    FD_ZERO(&readfd);
    readfd = master;
    
    struct PollingInfo fdinfo(listen_fd+1, &readfd, nullptr, nullptr, nullptr);
    pollFileDescriptors(fdinfo);

    // Loop through and find ready sockets
    for(int i = 0; i < listen_fd+1; i++) {
      if( desiredFDIsSet(i, listen_fd, &readfd) ) {
        int clientfd = acceptClient(listen_fd);
        std::string file = nextFilename();
        std::thread t;
        t = std::thread(handleConnection, clientfd, file);
        t.detach();
      }
    }
  }
}

int
main(int argc, char* argv[])
{
  server s = server(argc, argv);

  s.run();

  exit(0);
} // End main
