#ifndef SERVER_HEADERS_H
#define SERVER_HEADERS_H

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

//  std::filesystem for file and directory operations
#include <filesystem>
//  for file reading and writing with std::fstream
#include <fstream>

//  for creating threads and concurrency
#include <thread>

//  try-catch block and exception handling
#include <exception>

//  for LDAP client library functions
#include <ldap.h>


#endif // SERVER_HEADERS_H