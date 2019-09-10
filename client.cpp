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

// int main(int argc, char const *argv[])
// {
// 	std::cout << "Hello, world from client.\n";
// 	return 0;
// }

// Default constructor
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
  if (hostname == nullptr) {
    std::cerr << "ERROR: Unable to get HOSTNAME-OR-IP" << std::endl;
    exit(ARG_ERROR);
  }

  port = getArg(argv[2]);
  port = checkPortNo(port);
  if (port == nullptr) {
    std::cerr << "ERROR: invalid PORT number" << std::endl;
    exit(ARG_ERROR);
  }

  // Then we move onto getting the dirName
  filename = getArg(argv[3]);
  if (filename == nullptr) {
    std::cerr << "ERROR: Unable to get FILENAME" << std::endl;
    exit(ARG_ERROR);
  }
}

client::~client()
{
  fclose(fstream);
  close(sockfd);
}

//////////////////////////////
//
// Protected Helper Functions 
//
//////////////////////////////
const char* client::getArg(const char* arg)
{
	return arg != nullptr ? (const char*)arg : nullptr;
}

const char* client::checkPortNo(const char* arg)
{
	return atoi(arg) > 1023 ? arg : nullptr;
}

//////////////////////////////
//
// Public Functions
//
//////////////////////////////


void client::usage()
{
  std::cerr << "Usage: ./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\n";
  std::cerr << "  <HOSTNAME-OR-IP> hostname or IP address of the server to connect with.\n";
  std::cerr << "  <PORT>           port number of the server to connect with.\n";
  std::cerr << "  <FILENAME>       name of the file to transfer to the server\n";
}

int client::connect()
{
  struct addrinfo hints, *serverAddr;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  int result;
  if ((result = getaddrinfo(hostname, port, &hints, &serverAddr)) != 0) {
    fprintf(stderr, "ERROR: %s\n", gai_strerror(result));
    exit(CXN_ERROR);
  }

  int secondsAsleep = 0;
  struct addrinfo *rp;

  int status = -1;

  // Keep trying to connect until we've hit our timeout
  while (secondsAsleep < TIMEOUT && status == -1) {
    // Loop through our results from getaddrinfo
    for (rp = serverAddr; rp != nullptr; rp = rp->ai_next) {
      // create a socket using TCP IP
      if ((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        break;
    
      // If we can connect, break out of this loop
      if ((status = ::connect(sockfd, rp->ai_addr, rp->ai_addrlen)) != -1) {
        break;
      }

      close(sockfd);
    } // end for

    // We only want to sleep if we couldn't connect on any IPAddr
    if (status == -1) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      secondsAsleep++;
    }
  }

  // If we hit TIMEOUT, we're done.
  if (secondsAsleep == TIMEOUT) {
    errno = ETIMEDOUT;
    perror("ERROR");
    exit(TIMEOUT);
  }

  // This is redundant... 
  // If we didn't TIMEOUT, ensure we have a connection
  if (rp == nullptr) {
    std::cerr << "ERROR: Unable to connect to server" << std::endl;
    return -1;
  }

  // If we made it this far, reset errno in case it was set above
  if (errno == ECONNREFUSED)
    errno = 0;

  return 0;
}

FILE* client::open_file()
{
  fstream = fopen(filename, "r");
  if (!fstream) {
    std::cerr << "ERROR: Unable to open file.\n";
    exit(2);
  }
  return fstream;
}

int client::write(const void* buf, size_t nbyte)
{ 
  return ::write(sockfd, buf, nbyte);
}

void
commandEntered(char* ip, int p, char* f) {
  std::cerr << "Command Entered:" << std::endl;
  std::cerr << "    \"$ ./client " << ip << " " << p << " " << f << std::endl;
}

const unsigned short CHUNK = 1024;

void sleepFor(char* message, int numSec) {
  std::cerr << message << " " << numSec << " seconds.\n";
  std::this_thread::sleep_for(std::chrono::seconds(numSec));
}

int
main(int argc, char* argv[])
{

  client* c = new client(argc, argv);

  c->connect();

  FILE* file = c->open_file();

  int bytesRead;    // This is how we check whether reads
  int bytesWritten; // and write are successful

  while (true) {
    // Get ready to fread() and write
    char buf[CHUNK];
    bzero(buf, CHUNK);
    
    bytesRead = fread(buf, sizeof(char), CHUNK-1, file);

    // See whether there was an error reading
    if ( bytesRead == -1 ) {
      perror("ERROR");
      fclose(file);
      exit(IOERROR);
      break;
    } else if (bytesRead == 0) {
      // std::cerr << "Finished Transmitting. Exiting.\n";
      // break outta here, the file has been sent.
      break;
    } 

    else {
      // Send the information over the line
      bytesWritten = c->write(buf, bytesRead);

      // All is good if we wrote to the socket
      if (bytesWritten > 0) {
        continue;
      } else if (bytesWritten == -1) {
        // There was an error writing to the socket
        perror("ERROR");
        fclose(file);
        exit(IOERROR);
      } else {
        std::cout << "ERROR: Something else happened" << std::endl;
      }
    }
  }

  exit(EXIT_SUCCESS);
}