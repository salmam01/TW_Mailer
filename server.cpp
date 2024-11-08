#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <thread>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <ldap.h>

#define BUFFER_SIZE 1024
#define MAIL_SPOOL_DIR "/mail_spool"

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
      vector<string> body = parser(clientSocket);
      if(body.empty())
      {
        return false;
      }
      commandHandler(clientSocket, body);
      return true;
    }

    vector<string> parser(int clientSocket)
    {
      vector<string> body;
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
            body.clear();
            return body;
        }

        buffer[bytesRead] = '\0';
        receivedData << buffer;
      }
      
      // Split by newline
      string line;
      int count = 0;
      // Extract each line (header) until ".\n" (getline does so automatically)
      while (getline(receivedData, line)) 
      {
          if (line == ".") 
          {
            break;
          }

          //  If count is equal to 4, save the message into one index
          if(count == 4)
          {
            continue;
          }
           // Add each line as a header
          body.push_back(line);
          count++;
      }
      
      body.push_back(receivedData.str());
      return body;
    }

    void commandHandler(int clientSocket, vector<string> body)
    {
      string command = body[0];

      if(command == "SEND")
      {
        sendHandler(clientSocket, body);
      }
      else if(command == "LIST")
      {
        listHandler(clientSocket, body);
      }
      else if(command == "READ")
      {
        readHandler(clientSocket, body);
      }
      else if(command == "DEL")
      {
        delHandler(clientSocket, body);
      }
      else if(command == "QUIT")
      {
        cout << "Closing Server..." << endl;
        close(clientSocket);
      }
      else
      {
        cerr << "Invalid Command!" << endl;
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
    bool sendHandler(int clientSocket, vector<string> body)
    {
      if(!createMailSpool())
      {
        return false;
      }
      string sender = "if23b281";
      string receiver = body[1];
      string subject = body[2];
      string message = body[3];



    }

    /*
    LIST\n 
    <Username>\n
    */
    void listHandler(int clientSocket, vector<string> body)
    {
      string username = body[1];

    }

    /*
    READ\n 
    <Username>\n 
    <Message-Number>\n 
    */
    void readHandler(int clientSocket, vector<string> body)
    {
      
    }

    /*
    DEL\n 
    <Username>\n 
    <Message-Number>\n 
    */
    void delHandler(int clientSocket, vector<string> body)
    {
      
    }

    bool createMailSpool()
    {
      
      ofstream mailSpool(this->mailSpoolDir + ".txt");
      if(!mailSpool)
      {
        cerr << "An error occured while trying to create Mail-Spool: " << this->mailSpoolDir << endl;
        return false;
      }

      mailSpool.close();
      return true;
    }

    string sendResponse(bool state)
    {
      return state ? "OK\n" : "ERR\n";
    }

};

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