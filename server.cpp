#include <iostream>
#include "serverClass.h"

using namespace std;

//  Global server pointer to be able to pass a function if a signal is called
Server * serverPtr;

void printUsage()
{
  cout << "** SERVER USAGE **" << endl;
  cout << "./server <port> <mail-spool-directoryname>" << endl;
  cout << "<port>: must be a NUMBER" << endl;
  cout << "<mail-spool-directoryname>: must be a PATH" << endl;
}

void signalHandler(int sig)
{
  if(serverPtr != nullptr)
  {
    cout << "Shutting down server..." << endl;
    serverPtr->shutdown();
  }
}

int main(int argc, char *argv[])
{
  //  Check for missing arguments
  if(argc != 3)
  {
    cerr << "Invalid Input!" << endl;
    printUsage();
    return EXIT_FAILURE;
  }
 
  int port;
  istringstream ss(argv[1]);
  if(!(ss >> port) || port <= 0)
  {
    cerr << "Invalid port number. Please enter a number." << endl;
    return EXIT_FAILURE;
  }

  Server server(port, argv[2]);
  serverPtr = &server;

  if(!(server.start()))
  {
    cerr << "Server failed to start" << endl;
    return EXIT_FAILURE;
  }

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  return EXIT_SUCCESS;
}