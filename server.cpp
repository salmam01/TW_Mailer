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
        thread t(clientHandler, clientSocket);
        t.detach();
      }
    }

    void clientHandler(int clientSocket)
    {
      char buffer[BUFFER_SIZE];
      memset(buffer, 0, BUFFER_SIZE);
      ssize_t bytesRead = 0;
      string command;
      while(bytesRead < BUFFER_SIZE)
      {
        	bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
          if(bytesRead <= 0)
          {
            cerr << "Error while reading data." << endl;
            close(clientSocket);
            return;
          }
          //  idk wtf im doing here yet, ignore pls
          if(bytesRead == '/n')
          {
            command = bytesRead;
          }
      }
      commandHandler(clientSocket, buffer);
    }

    void commandHandler(int clientSocket, string command)
    {
      if(command == "SEND")
      {

      }
      else if(command == "LIST")
      {
        
      }
      else if(command == "READ")
      {
        
      }
      else if(command == "DEL")
      {
        
      }
      else if(command == "QUIT")
      {
        
      }
      else
      {
        cerr << "Invalid Method!" << endl;
      }
    }

    void sendHandler()
    {

    }

    void listHandler()
    {
      
    }

    void readHandler()
    {
      
    }

    void delHandler()
    {
      
    }

    void printUsage()
    {
      cout << "** SERVER USAGE **" << endl;
      cout << "./twmailer-server <port> <mail-spool-directoryname>" << endl;
      cout << "<port>: must be INT value" << endl;
      cout << "<mail-spool-directoryname>: must be PATH value" << endl;
    }
};