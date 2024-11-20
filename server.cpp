#include <iostream>
#include "serverClass.cpp"

using namespace std;

//  Global server pointer to be able to pass a function if a signal is called
Server * serverPtr = nullptr;

void printUsage()
{
  cout << "** SERVER USAGE **" << endl;
  cout << "./server <port> <mail-spool-directoryname>" << endl;
  cout << "<port>: must be a NUMBER" << endl;
  cout << "<mail-spool-directoryname>: must be a PATH" << endl;
}

void signalHandler(int sig)
{
  if(sig == SIGINT)
  {
    if(serverPtr != nullptr)
    {
      cerr << "Shutdown Requested" << endl;
      serverPtr->shutDown();
    }
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

  if(signal(SIGINT, signalHandler) == SIG_ERR)
  {
    cerr << "Error registering signal." << endl;
  }

  return EXIT_SUCCESS;
}