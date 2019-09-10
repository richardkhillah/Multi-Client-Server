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

server::server() : filedir(nullptr), port(nullptr), listen_fd(0), fileno(0) {}

server::server(int argc, char* argv[]) : listen_fd(0), fileno(0)
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
	if ( port == nullptr ) {
		std::cerr << "ERROR: invalid port number" << std::endl;
		exit(ARG_ERROR);
	}

	// and the directory to safe files.
	filedir = getArg(argv[2]);
	if (filedir == nullptr) {
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

//////////////////////////////
//
// Protected Helper Functions 
//
//////////////////////////////

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

const char* server::getArg(const char* arg)
{
	return arg != nullptr ? (const char*)arg : nullptr;
}

const char* server::checkPortNo(const char* arg)
{
	return atoi(arg) > 1023 ? arg : nullptr;
}

//////////////////////////////
//
// Public Functions
//
//////////////////////////////

void server::usage()
{
	std::cerr << "Usage: ./server <PORT> <FILE-DIR>\n";
  	std::cerr << "  <PORT>      port number to listen on connections.\n";
  	std::cerr << "  <FILE-DIR>  directory name where to save the received files\n";
}

int server::start()
{
	// Stuff for getting address information
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int status;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;  // Allow for IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // TCP connection only
  hints.ai_flags = AI_PASSIVE;  // For wildcard IP address
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  if ((status = getaddrinfo(nullptr, port, &hints, &result)) != 0) {
    perror("getaddrinfo");
    return EXIT_FAILURE;
  }

  // get our list of address structures
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    // Create a TCP socket
    listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listen_fd == -1)
      continue;

    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // allow the port to be reused then move on.
      int yes = 1;
      if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
      }
      break;
    }

    // otherwise close this socket and try again
    close(listen_fd);
  }

  // If we make it this far, see whether we bound the socket to an addr
  if (rp == nullptr) {
    std::cerr << "Unable to successfully bind" << std::endl;
    return EXIT_FAILURE;
  }

  // we've bound, so we're done with this
  freeaddrinfo(result);

  // Turn the socket into a listener
  if (listen(listen_fd, 1) == -1) {
    perror("listen");
    return 3;
  }
  return 0;
}

int server::getListener() {	return listen_fd;	}

int server::write(const void* buf, size_t nbyte)
{
	return -1;
}

std::string server::nextFilename()
{
  const char* postfix = ".file";
  std::string file("./");
  file = file + filedir + std::to_string(++fileno) + postfix;
  return file;
}

void server::handleConnection(int clientfd, std::string file)
{
   
  std::cerr << "file = " << file << std::endl;
  FILE* ofile = fopen(file.c_str(), "w");

  // Keep track of progress
  int bytesRead;
  int bytesWritten;
  int newCount = 0;
  int totalBytesRead = 0;
  int totalBytesWritten = 0;

  while (true) {
    char buf[CHUNK];    // buffer to read into/write from
    bzero(buf, CHUNK);  // clean slate each time

    // read some bytes from the client
    bytesRead = recv(clientfd, buf, CHUNK-1, MSG_DONTWAIT);

    // Since we're doing non-blocking I/O, check to see whether
    // recv would have blocked
    if (errno == EWOULDBLOCK) {
      std::cerr << "Putting thread handling socket " << clientfd << " to sleep for 1 second... ";
      // If it would have, that means nothing was read, so 
      // go to sleep for a second.
      std::this_thread::sleep_for(std::chrono::seconds(1));
      newCount++;
      std::cerr << "Total timeout count: " << newCount << std::endl;
      errno = 0;  // Reset errno for next time.

      // Then we check to see wether that last second we were asleep
      // pushed us to the timeout threshold
      if( newCount == TIMEOUT ) {
        // int old_errno = errno;
        errno = ETIMEDOUT;
        perror("ERROR");

        // Determine wheter client sent bytes
        if (totalBytesRead > 0) {
          // flush contents of file
          if (fclose(ofile) == 0) {
            ofile = fopen(file.c_str(), "w");
            const char *msg = "ERROR";
            fwrite(msg, sizeof(char), sizeof msg, ofile);
            break;
          } else {
            // This shouldn't have happened and so errno is set
            perror("ERROR:");
            exit(EXIT_FAILURE);
          }
        } else {
          std::cerr << "ERROR: No data sent from client.\n";
        } // end if totalBytesRead
        break; // out of transmission loop
      } // end if TIMEOUT

      // skip the rest of this loop since there is nothing in the
      // buffer to write and check the socket again.
      continue;
    } else {
      // std::cerr << "bytesRead: " << bytesRead;
      errno = 0;    // reset so we don't misinterpret later
      newCount = 0; 
    }

    // If there is something to write,
    if ( bytesRead > 0 ) {
      // Keep track of how much we have read
      totalBytesRead += bytesRead;
      // then write bytes to file
      bytesWritten = fwrite(buf, sizeof(char), bytesRead, ofile);
      if (bytesWritten < 0) {
        perror("ERROR");
        break;
      }
      // std::cerr << " bytesWritten: " << bytesWritten << " totalBytesWritten: " << totalBytesWritten << std::endl;
      
      // add the bytes read to totalBytesRead
      totalBytesWritten += bytesWritten;
    } else if (bytesRead == 0) {
      // eof reached and client closed cxn
      break;
    } else {

      // std::cerr << "else bytesRead: " << bytesRead << std::endl;
    }
  }
  
  // Standard clean up
  // std::cerr << "closing " << file << std::endl;
  fclose(ofile);

  close(clientfd);
}


int
main(int argc, char* argv[])
{
  server* s = new server(argc, argv);

	// open connection and listen
	s->start();

	// Get ready to work. 
	fd_set master;
	fd_set readfd;
  int listen_fd = s->getListener();
	int maxfd = -1;
	FD_ZERO(&master);
	FD_ZERO(&readfd);

  // So select() can 
  FD_SET(listen_fd, &master);
  maxfd = listen_fd;

  std::thread t;
  int select_status;
  
  // Main accept/dispatch loop
  for (;;) {
    // Always make a copy. Select alters readfd
    readfd = master;

    // and now we wait
    select_status = select(maxfd+1, &readfd, nullptr, nullptr, nullptr);
    if (select_status == -1) {
      perror("Select");
      return -10;
    } else if (select_status == 0) {
      // We should never get here, but if we do, let console know.
      std::cerr << "ERROR: TIMEOUT occured on main thread" << std::endl;
      // close (listen_fd);
      break;
    }



    // Loop through and find ready sockets
    for(int i = 0; i < maxfd+1; i++) {
      // We are only interested in file descriptors select() SET
      if (FD_ISSET(i, &readfd)) {
        if ( i == listen_fd ) {
          // accept a new connection
          struct sockaddr_in clientAddr;
          socklen_t clientAddrSize = sizeof(clientAddr);
          int clientfd;

          clientfd = accept(listen_fd, (struct sockaddr*)&clientAddr, &clientAddrSize);
       
          if (clientfd == -1) {
            perror("ERROR");
            return 4;
          } else {
            // Send a thread off to handle the client
            // fileno++;
            // Form the name of the file and open it
            std::string file = s->nextFilename();

            t = std::thread(s->handleConnection, clientfd, file);
            t.detach(); // We don't want to wait here...
          }
        }
      }
    }
  }

  exit(0);
} // End main
