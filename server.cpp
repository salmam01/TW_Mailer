#include <iostream>
#include "serverClass.h"


using namespace std;

void printUsage()
{
  cout << "** SERVER USAGE **" << endl;
  cout << "./server <port> <mail-spool-directoryname>" << endl;
  cout << "<port>: must be a NUMBER" << endl;
  cout << "<mail-spool-directoryname>: must be a PATH" << endl;
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

  ////////////////////////////////////////////////////////////////////////////
  // SIGNAL HANDLER
  // SIGINT (Interrupt: ctrl+c)
  // https://man7.org/linux/man-pages/man2/signal.2.html
  /*if (signal(SIGINT, signalHandler) == SIG_ERR)
  {
    perror("signal can not be registered");
    return EXIT_FAILURE;
  }*/

  Server server(port, argv[2]);
  if(!(server.start()))
  {
    cerr << "Server failed to start" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}