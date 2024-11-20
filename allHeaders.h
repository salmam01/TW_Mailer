#ifndef ALL_HEADERS_H
#define ALL_HEADERS_H

//  for standard input/output streams
#include <iostream>
//  std::stringstream and string manipulations
#include <sstream>
//  for c-style strings
#include <cstring>
//  for vector operations
#include <vector>
//  socket, bind, listen, accept
#include <sys/socket.h> 
//  internet address family (struct sockaddr_in) 
#include <netinet/in.h>
//  close function
#include <unistd.h>
//  for receiving signals in the main process
#include <signal.h>
//  std::filesystem for file and directory operations
#include <filesystem>
//  for file reading and writing with std::fstream
#include <fstream>
//  for creating threads and concurrency
#include <thread>
//  for creating mutex and managing thread synchronization
#include <mutex>
//  for blacklist and timeout
#include <chrono>
//  try-catch block and exception handling
#include <exception>
//  for LDAP client library functions
#include <ldap.h>
// for manipulating Internet addresses
#include <arpa/inet.h>
//provides errno-variables and predefined ERR
#include <cerrno>
// to capitalize the client-side input commands
#include <algorithm> // for transform
#include <ctype.h>   // for toupper
// for terminal i/o - for hidden password
#include <termios.h>

#endif // ALL_HEADERS_H