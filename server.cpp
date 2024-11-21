#include <iostream>
#include "serverClass.h"

using namespace std;

//  Global server pointer to be able to pass a function if a signal is called
Server * serverPtr = nullptr;

//  Function to print the usage of the server
void printUsage()
{
  cout << "** SERVER USAGE **" << endl;
  cout << "./server <port> <mail-spool-directoryname>" << endl;
  cout << "<port>: must be a NUMBER" << endl;
  cout << "<mail-spool-directoryname>: must be a PATH" << endl;
}

//  Function that handles CTRL + C on server side
void signalHandler(int sig)
{
  if(sig == SIGINT || sig == SIGHUP)
  {
    if(serverPtr != nullptr)
    {
      cerr << "Shutdown Requested ..." << endl;
      serverPtr->shutDown();
    }
    exit(0);
  }
}

//  Initiates the server instance and starts the server
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
    cerr << "Server failed to start." << endl;
    return EXIT_FAILURE;
  }

  if(signal(SIGINT, signalHandler) == SIG_ERR)
  {
    cerr << "Error registering signal." << endl;
  }
  if(signal(SIGHUP, signalHandler) == SIG_ERR)
  {
    cerr << "Error registering signal." << endl;
  }

  return EXIT_SUCCESS;
}