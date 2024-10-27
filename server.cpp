#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>
#include <thread>
#include <unistd.h>
#include <cstring>

#define BUFFER_SIZE 1024

using namespace std;
using namespace std::filesystem;

class Server
{
  private:
    int port;
    path mailSpoolDir;

  public:
    Server(int port, path mailSpoolDir)
    {
      this->port = port;
      this->mailSpoolDir = mailSpoolDir;
    }

    void start()
    {
      //  Create a new socket
      int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
      
      //  AF_INET = IPv4
      sockaddr_in serverAddress;
      serverAddress.sin_family = AF_INET;
      serverAddress.sin_port = htons(this->port);
      serverAddress.sin_addr.s_addr = INADDR_ANY;

      cout << "Port: " << this->port << endl;
      cout << "Mail-Spool Directory: " << this->mailSpoolDir << endl;

      //  Binding socket
      if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
      {
        cerr << "Error while binding socket." << endl;
        close(serverSocket);
        return;
      }

      //  Listen for incoming clients
      if(listen(serverSocket, 5) < 0)
      {
        cerr << "Error while listening for clients." << endl;
        close(serverSocket);
        return;
      }

      //  Accept client
      acceptClients(serverSocket);
      
      close(serverSocket);
    }

    void acceptClients(int serverSocket)
    {
      while(1)
      {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) 
        {
          cerr << "Error accepting client connection." << endl;
          return;
        }

        //  Need to look into threads for concurrent server
        //thread()
        clientHandler(clientSocket);
      }
    }

    void clientHandler(int clientSocket)
    {
      char buffer[BUFFER_SIZE];


    }

    void printUsage()
    {
      cout << "** SERVER USAGE **" << endl;
      cout << "./twmailer-server <port> <mail-spool-directoryname>" << endl;
      cout << "<port>: must be INT value" << endl;
      cout << "<mail-spool-directoryname>: must be PATH value" << endl;
    }
};

int main(int argc, char *argv[])
{
  //  Check for missing arguments
  if(argc != 3)
  {
    cerr << "Invalid Input!" << endl;
    return -1;
  }

  Server server(stoi(argv[1]), argv[2]);
  server.start();

  return 0;
}