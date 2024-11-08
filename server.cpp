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
      string command = parser(clientSocket);
      if(command.empty())
      {
        cerr << "Command cannot be empty." << endl;
        return false;
      }

      //  Pass the command to the commandHandler if it's not empty
      commandHandler(clientSocket, command);
      return true;
    }

    string parser(int clientSocket)
    {
      //  Buffer reads the data, bytesRead determines the actual number of bytes read
      char buffer[BUFFER_SIZE];
      ssize_t bytesRead = 0;
      string line;

      //  Read the data and save it into the receivedData string
      while(receivedData.str().find("\n") == string::npos)
      {
        //  Clear the buffer and read up to buffer_size - 1 bytes
        memset(buffer, 0, BUFFER_SIZE);
        //  recv reads the data from clientSocket and stores it into buffer (up to buffer_size -1)
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) 
        {
            cerr << "Error while reading message body." << endl;
            close(clientSocket);
            return "";
        }

        buffer[bytesRead] = '\0';
        line += buffer;
      }

      return line;
    }

    void commandHandler(int clientSocket, string command)
    {
      if(command == "LOGIN")
      {
        loginHandler(clientSocket);
      }
      else if(command == "SEND")
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
        cerr << "Invalid Command!" << endl;
      }
    }

    bool loginHandler(int clientSocket)
    {

    }

    /*
    SEND\n 
    <Receiver>\n 
    <Subject (max. 80 chars)>\n 
    <message (multi-line; no length restrictions)\n> 
    .\n  
    */
    //  Ignore the fact that this is a bool for now, it will come in handy later
    bool sendHandler(int clientSocket)
    {
      vector<string> body = sendParser(clientSocket);
      if(body.empty() || body.size() < 3)
      {
        sendResponse(clientSocket, false);
        cerr << "Invalid command syntax." << endl;
        return false;
      }

      //  get the actual sender at some point, this just for testing
      string sender = "if23b281";
      string receiver = body[0];
      string subject = body[1];
      string message = body[2];

      fstream fout;
      string mailSpoolName = sender + ".csv";

      //  Open or create a new file with the name
      fout.open(mailSpoolName, ios::out | ios::app);
      //  Check if the file was opened successfully
      if(!fout.is_open())
      {
        cerr << "Error occurred while opening Mail-Spool file." << endl;
        return false;
      }
      
      fout  << receiver << ";"
            << subject << ";"
            << '"' 
            << message 
            << '"'
            << "\n"; 

      if(fout.fail())
      {
        fout.close();
        cerr << "Error writing to Mail-Spool file." << endl;
        sendResponse(clientSocket, false);
        return false;
      }

      fout.close();

      sendResponse(clientSocket, true);
      return true;
    }

    //  ??? hopefully this shit works now
    vector<string> sendParser(int clientSocket)
    {
      vector<string> body;
      string line;

      while(1)
      {
        //  Use the parser function to extract each line
        line = parser(clientSocket);

        if (line == ".") 
        {
          break;
        }

        if(body.size() < 3)
        {
          // Save receiver, subject
          body.push_back(line);
        }
        //  If body size is greater than 3, save the message into one index
        else
        {
          //  Initialize the vector index, then append the message lines
          if(body.size() == 3)
          {
            body.push_back("");
          }
          body[3] += line + "\n";
        }
      }
      
      return body;
    }

    /*
    LIST\n 
    */
    //  this function should list everything inside the csv file for the specified user
    void listHandler(int clientSocket)
    {
      //  temporary
      string sender = "if23b281";
      fstream fin;
      string mailSpoolName = sender + ".csv";

      fin.open(mailSpoolName, ios::in);
      if(!fin.is_open())
      {
        sendResponse(clientSocket, false);
        cerr << "File does not exist." << endl;
        return;
      }

      string line;
      vector<string> subjects;
      //  Loop until the end of the file
      while(getline(fin, line))
      {
        stringstream ss(line);
        string receiver, subject, message;

        getline(ss, receiver, ';');
        getline(ss, subject, ';');
        getline(ss, message);

        subjects.push_back(subject);
      }
      fin.close();
      
      string response = "Message Count: " + to_string(subjects.size()) + "\n";

      for(int i = 0; i < subjects.size(); i++)
      {
        response += subjects[i] + "\n";
      }

      send(clientSocket, response.c_str(), response.size(), 0);
    }

    /*
    READ\n 
    <Message-Number>\n 
    */
    //  Could be improved further, same as one above
    void readHandler(int clientSocket)
    {
      int messageNr;
      try
      {
        messageNr = stoi(parser(clientSocket));
      }
      catch(const exception& e)
      {
        sendResponse(clientSocket, false);
        cerr << "Message number has to be of integer type." << endl;
        return;
      }

      string sender = "if23b281";
      fstream fin;
      string mailSpoolName = sender + ".csv";

      fin.open(mailSpoolName, ios::in);
      if(!fin.is_open())
      {
        sendResponse(clientSocket, false);
        cerr << "File does not exist." << endl;
        return;
      }

      string line;
      string response;
      int count = 1;
      while(getline(fin, line))
      {
        stringstream ss(line);
        string receiver, subject, message;

        getline(ss, receiver, ';');
        getline(ss, subject, ';');
        getline(ss, message);

        if(count == messageNr)
        {
          response = message; 
          break;
        }
        count++;
      }
      fin.close();

      if(response.empty())
      {
        sendResponse(clientSocket, false);
        cerr << "Message does not exist." << endl;
      }
      else
      {
        send(clientSocket, response.c_str(), response.size(), 0);
      }
    }

    /*
    DEL\n 
    <Message-Number>\n 
    */
    void delHandler(int clientSocket)
    {
      string messageNr = parser(clientSocket);

    }

    //  Not even sure what this is for atp??
    /*bool createMailSpool(string username)
    {
      ofstream mailSpool(username + ".txt");
      if(!mailSpool)
      {
        cerr << "An error occured while trying to create Mail-Spool: " << this->mailSpoolDir << endl;
        return false;
      }

      mailSpool.close();
      return true;
    }*/

    void sendResponse(int clientSocket, bool state)
    {
      if(state)
      {
        send(clientSocket, "OK\n", 3, 0);
      }
      else
      {
        send(clientSocket, "ERR\n", 4, 0);
      }
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