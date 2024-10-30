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
      //  Buffer reads the data, bytesRead determines the actual number of bytes read
      char buffer[BUFFER_SIZE];
      //  Clear the buffer and read up to buffer_size - 1 bytes
      memset(buffer, 0, BUFFER_SIZE);
      ssize_t bytesRead = 0;
      string command;

      //  recv reads the data from clientSocket and stores it into buffer (up to buffer_size -1)
      while((bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0)) != 0)
      {   
          //  Error occured while reading data      
          if(bytesRead < 0)
          {
            cerr << "Error while reading data." << endl;
            close(clientSocket);
            return;
          }

          command += buffer;
          
          //  New line means command has been read
          if(command == "/n")
          {
            break;
          }
      }
      commandHandler(clientSocket, command);
    }

    void commandHandler(int clientSocket, string command)
    {
      if(command == "SEND")
      {
        sendHandler();
      }
      else if(command == "LIST")
      {
        listHandler();
      }
      else if(command == "READ")
      {
        readHandler();
      }
      else if(command == "DEL")
      {
        delHandler();
      }
      else if(command == "QUIT")
      {
        //  probably doesn't need its own function
      }
      else
      {
        cerr << "Invalid Method!" << endl;
      }
    }

    /*
    SEND\n 
    <Sender>\n 
    <Receiver>\n 
    <Subject (max. 80 chars)>\n 
    <message (multi-line; no length restrictions)\n> 
    .\n  
    */
    void sendHandler()
    {
      int bytesRead;
      //while()
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