#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>
#include <sstream>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <vector>

#define BUFFER_SIZE 1024

using namespace std;
using namespace std::filesystem;

class Server
{
  private:
    int port;
    string mailSpoolDir;
    int serverSocket = -1;
    int reuseValue = 1;
    int abortRequested = 0;

  public:
    Server(int port, string mailSpoolDir)
    {
      this->port = port;
      this->mailSpoolDir = mailSpoolDir;
    }

    bool start()
    {
      //  Create a new socket
      if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
      {
        cerr << "Error while creating socket" << endl;
        return false;
      }

      if (setsockopt(serverSocket,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &reuseValue,
                      sizeof(reuseValue)) == -1)
      {
          cerr << "Set socket options - reuseAddr" << endl;
          return false;
      }

      if (setsockopt(serverSocket,
                      SOL_SOCKET,
                      SO_REUSEPORT,
                      &reuseValue,
                      sizeof(reuseValue)) == -1)
      {
          cerr << "Set socket options - reusePort" << endl;
          return false;
      }

      //  AF_INET = IPv4
      sockaddr_in serverAddress;
      memset(&serverAddress, 0, sizeof(serverAddress));
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
        return false;
      }

      //  Listen for incoming clients
      if(listen(serverSocket, 5) < 0)
      {
        cerr << "Error while listening for clients." << endl;
        close(serverSocket);
        return false;
      }

      //  Accept client
      if(!acceptClients(serverSocket))
      {
        return false;
      }
      close(serverSocket);
      return true;
    }

    bool acceptClients(int serverSocket)
    {
      while(!abortRequested)
      {
        cout << "Listening for client connections..." << endl;
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) 
        {
          cerr << "Error accepting client connection." << endl;
          return false;
        }

        //  Need to look into threads for concurrent server
        //thread t(clientHandler, clientSocket);
        //t.detach();
        clientHandler(clientSocket);
      }
        return true;
    }

    bool clientHandler(int clientSocket)
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
            sendResponse(false);
            close(clientSocket);
            return false;
          }

          command += buffer;
          
          //  New line means command has been read
          if(buffer == "\n")
          {
            break;
          }
      }
      commandHandler(clientSocket, command);
      return true;
    }

    void commandHandler(int clientSocket, string command)
    {
      if(command == "SEND")
      {
        sendHandler(clientSocket);
      }
      else if(command == "LIST")
      {
        listHandler(clientSocket);
      }
      else if(command == "READ")
      {
        readHandler(clientSocket);
      }
      else if(command == "DEL")
      {
        delHandler(clientSocket);
      }
      else if(command == "QUIT")
      {
        cout << "Closing Server..." << endl;
        close(clientSocket);
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
    void sendHandler(int clientSocket)
    {
      vector<string> headers = parser(clientSocket);
      string sender = headers[0];
      string receiver = headers[1];
      string subject = headers[2];
    }

    void listHandler(int clientSocket)
    {
      vector<string> headers = parser(clientSocket);
      string username = headers[0];

    }

    void readHandler(int clientSocket)
    {
      parser(clientSocket);
    }

    void delHandler(int clientSocket)
    {
      parser(clientSocket);
    }

    vector<string> parser(int clientSocket)
    {
      vector<string> headers;
      //  Buffer reads the data, bytesRead determines the actual number of bytes read
      char buffer[BUFFER_SIZE];
      ssize_t bytesRead = 0;
      stringstream receivedData;

      //  Read the data and save it into the receivedData string
      while(receivedData.str().find(".\n") == string::npos)
      {
        //  Clear the buffer and read up to buffer_size - 1 bytes
        memset(buffer, 0, BUFFER_SIZE);
        //  recv reads the data from clientSocket and stores it into buffer (up to buffer_size -1)
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) 
        {
            cerr << "Error while reading message body." << endl;
            close(clientSocket);
            headers.clear();
            return headers;
        }

        buffer[bytesRead] = '\0';
        receivedData << buffer;
      }
      
      // Split by newline
      string line;
      // Extract each line (header) until ".\n" (getline does so automatically)
      while (getline(receivedData, line)) 
      {
          if (line == ".") 
          {
            break;  // Stop at end of headers
          }

          headers.push_back(line);  // Add each line as a header
      }

      return headers;
    }

    string sendResponse(bool state)
    {
      return state ? "OK\n" : "ERR\n";
    }

    void printUsage()
    {
      cout << "** SERVER USAGE **" << endl;
      cout << "./server <port> <mail-spool-directoryname>" << endl;
      cout << "<port>: must be a NUMBER" << endl;
      cout << "<mail-spool-directoryname>: must be a PATH" << endl;
    }
};

int main(int argc, char *argv[])
{
  //  Check for missing arguments
  if(argc != 3)
  {
    cerr << "Invalid Input!" << endl;
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