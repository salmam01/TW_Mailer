#include "serverClass.h"
#include "serverHeaders.h"

using namespace std;
using namespace filesystem;


Server::Server(int port, string mailSpoolDirName)
{
  memset(&this->serverAddress, 0, sizeof(this->serverAddress));
  this->serverAddress.sin_family = AF_INET;
  this->serverAddress.sin_port = htons(port);
  this->serverAddress.sin_addr.s_addr = INADDR_ANY;

  //  Create the Mail Spool Directory
  path currentPath = current_path();
  this->mailSpoolDir.name = mailSpoolDirName;
  this->mailSpoolDir.path = currentPath /= this->mailSpoolDir.name;
  if(!exists(this->mailSpoolDir.path))
  {
    create_directory(this->mailSpoolDir.path);
  }
} 

void Server::shutDown()
{
  cerr << "Shutting down server..." << endl;
  this->abortRequested = true;
  shutdown(this->serverSocket, SHUT_RDWR);
  close(this->serverSocket);
}

bool Server::start()
{
  //  Create a new socket
  if((this->serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    cerr << "Error while creating socket" << endl;
    return false;
  }

  //  Set socket options 
  if (setsockopt( this->serverSocket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
  {
    cerr << "Set socket options - reuseAddr" << endl;
    return false;
  }

  if (setsockopt( this->serverSocket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
  {
    cerr << "Set socket options - reusePort" << endl;
    return false;
  }
  
  cout << "Server start was successful." << endl;
  cout << "Port: " << ntohs(this->serverAddress.sin_port) << endl;
  cout << "Mail-Spool Directory Name: " << this->mailSpoolDir.name << endl;
  cout << "Mail-Spool Directory Path: " << this->mailSpoolDir.path << endl;

  //  Check binding socket for errors
  if(bind(this->serverSocket, (struct sockaddr*)&this->serverAddress, sizeof(this->serverAddress)) < 0)
  {
    cerr << "Error while binding socket." << endl;
    close(this->serverSocket);
    return false;
  }

  //  Check listen for incoming clients and errors
  if(listen(this->serverSocket, 5) < 0)
  {
    cerr << "Error while listening for clients." << endl;
    close(this->serverSocket);
    return false;
  }

  //  Accept clients
  acceptClients();

  close(this->serverSocket);
  return true;
}

//  Function that accepts clients until the server is shut down
void Server::acceptClients()
{
  while(!abortRequested)
  {
    cout << "Listening for client connections on port " << ntohs(this->serverAddress.sin_port) << endl;
    int clientSocket = accept(this->serverSocket, nullptr, nullptr);
    if (clientSocket >= 0) 
    {
      //  Create a thread for each client connection and pass the function and parameter
      cout << "Client accepted." << endl;
      thread clientThread(&Server::clientHandler, this, clientSocket);

      this->threadsMutex.lock();
      //  Move threads into the vector
      this->activeThreads.push_back(move(clientThread));
      this->threadsMutex.unlock();
    }
    else 
    {
      cerr << "Error accepting client connection: " << strerror(errno) << endl;
      continue;
    }

    cleanUpThreads();
  }

  cleanUpThreads();
}

//  Helper function for acceptclient that joins all finished threads 
void Server::cleanUpThreads()
{
  //  Critical section
  lock_guard<mutex> lockMutex(this->threadsMutex);

  for(auto &thread : this->activeThreads)
  {
    if(thread.joinable())
    {
      thread.join();
    }
  }
  this->activeThreads.clear();
}

void Server::clientHandler(int clientSocket)
{
  try
  {
    string welcomeMessage = "Welcome to our Mail Server! Please login with LOGIN.\n";
    if (send(clientSocket, welcomeMessage.c_str(), welcomeMessage.size(), 0) == -1)
    {
      cerr << "Send failed." << endl;
      return;
    }

    string command = parser(clientSocket);
    cout << "Do u ever exit?" << endl;
    if(!command.empty())
    {
      commandHandler(clientSocket, command);
    }
    else
    {
      cerr << "Command cannot be empty." << endl;
      return;
    }
  }
  catch(const exception &e)
  {
    cerr << "Exception in clientHandler: " << e.what() << endl;
    sendResponse(clientSocket, false);
    close(clientSocket);
    return;
  }
  catch(...)
  {
    cerr << "An unknown error occurred. Connection will terminate." << endl;
    sendResponse(clientSocket, false);
    close(clientSocket);
    return;
  }
  close(clientSocket);
}

string Server::parser(int clientSocket)
{
  // Buffer reads the data, bytesRead determines the actual number of bytes read
  char buffer[BUFFER_SIZE];
  ssize_t bytesRead = 0;
  string line;

  // Read the data and save it into the buffer
  while (true)
  {
    // Clear the buffer
    memset(buffer, 0, BUFFER_SIZE);
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

    if (bytesRead == 0)
    {
      cerr << "Client closed the connection." << endl;
      return line;
    }
    if (bytesRead < 0)
    {
      cerr << "Error while reading message body." << endl;
      return "";
    }

    buffer[bytesRead] = '\0';
    line += buffer;

    // Debug output
    cout << "Received client input: " << buffer << endl;

    // Check if the accumulated line contains a newline
    size_t newlinePos = line.find('\n');
    if (newlinePos != string::npos)
    {
        // Trim the newline character and return the result
        line = line.substr(0, newlinePos);
        break;
    }
  }
  return line;
}

//  Function that handles each command 
void Server::commandHandler(int clientSocket, string command)
{
  cout << "Received command: " << command << endl;
  
  if (command == "LOGIN")
  {
    cout << "Handling LOGIN command." << endl;
    if (loginHandler(clientSocket)) 
    {
      sendResponse(clientSocket, true);
    } else 
    {
      sendResponse(clientSocket, false);
    }
  }
  else if (command == "SEND")
  {
    cout << "Handling SEND command." << endl;
    sendHandler(clientSocket); 
  }
  else if (command == "LIST")
  {
    cout << "Handling LIST command." << endl;
    listHandler(clientSocket);
  }
  else if (command == "READ")
  {
    cout << "Handling READ command." << endl;
    readHandler(clientSocket);
  }
  else if (command == "DEL")
  {
    cout << "Handling DEL command." << endl;
    delHandler(clientSocket);
  }
  else if (command == "QUIT")
  {
    cout << "Handling QUIT command." << endl;
    sendResponse(clientSocket, true);
    close(clientSocket);
  }
  else
  {
    // Invalid command
    sendResponse(clientSocket, false);
    cerr << "Invalid Command." << endl;
  }
}

bool Server::loginHandler(int clientSocket)
{
  char buffer[1024];
  ssize_t bytesRead;

  // Read the username sent by the client
  memset(buffer, 0, sizeof(buffer)); // Clear the buffer
  bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
  
  if (bytesRead <= 0)
  {
    cerr << "Error reading username from client." << endl;
    return false;
  }

  // Null-terminate the string received from the client
  buffer[bytesRead] = '\0';
  string username(buffer);
  cout << "User entered: " << username << endl; // Log the username

  // Read the password sent by the client
  memset(buffer, 0, sizeof(buffer)); // Clear the buffer
  bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
  
  if (bytesRead <= 0)
  {
    cerr << "Error reading password from client." << endl;
    return false;
  }

  // Null-terminate the string received from the client
  buffer[bytesRead] = '\0';
  string password(buffer);
  cout << "Password entered: " << password << endl; // Log the password

  // For testing
  return true;
}

bool Server::establishLDAPConnection(const string& bindPassword)
{
    LDAP* ldapHandle;
    int result;

    // Initialize LDAP connection using the class member ldapServer
    result = ldap_initialize(&ldapHandle, ldapServer);
    if (result != LDAP_SUCCESS)
    {
        cerr << "Failed to initialize LDAP connection: " << ldap_err2string(result) << endl;
        return false;
    }
    cout << "LDAP connection initialized successfully." << endl;

    // Set LDAP options (optional but recommended)
    int protocolVersion = LDAP_VERSION3;
    result = ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &protocolVersion);
    if (result != LDAP_SUCCESS)
    {
        cerr << "Failed to set LDAP protocol version: " << ldap_err2string(result) << endl;
        ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
        return false;
    }
    cout << "LDAP protocol version set to 3." << endl;

    // Bind to the LDAP server using the class member ldapBind
    BerValue bindCredentials;
    bindCredentials.bv_val = const_cast<char*>(bindPassword.c_str());
    bindCredentials.bv_len = bindPassword.length();

    BerValue* serverCredentials = nullptr;
    result = ldap_sasl_bind_s(ldapHandle, ldapBind, LDAP_SASL_SIMPLE, &bindCredentials, nullptr, nullptr, &serverCredentials);

    if (result != LDAP_SUCCESS)
    {
        cerr << "LDAP bind failed: " << ldap_err2string(result) << endl;
        ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
        return false;
    }
    cout << "LDAP bind successful." << endl;

    // Connection is established successfully
    ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
    return true;
}

//  Function to save a sent message inside a csv file
void Server::sendHandler(int clientSocket)
{
  vector<string> body = sendParser(clientSocket);
  if(body.empty() || body.size() < 3)
  {
    sendResponse(clientSocket, false);
    cerr << "Invalid SEND command syntax." << endl;
    return;
  }

  //  get the actual sender at some point, this just for testing
  string sender = "if23b281";
  string receiver = body[0];
  string subject = body[1];
  string message = body[2];

  fstream fout;
  //  Save the new file name as sender.csv inside the Mail Spool Directory location
  string mailSpoolFile = (mailSpoolDir.path / (sender + ".csv")).string();

  //  Open or create a new file with the name (if none exists yet)
  fout.open(mailSpoolFile, ios::out | ios::app);
  //  Check if the file was opened successfully
  if(!fout.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "Error occurred while opening Mail-Spool file:" << mailSpoolFile << endl;
    return;
  }
  
  //  Write the information and message into the file
  fout  << receiver << ";"
        << subject << ";"
        << '"' 
        << message 
        << '"'
        << "\n"; 

  if(fout.fail())
  {
    fout.close();
    sendResponse(clientSocket, false);
    cerr << "Error writing to Mail-Spool file: " << mailSpoolFile << endl;
    return;
  }

  fout.close();
  sendResponse(clientSocket, true);
}

//  Helper function of sendHandler to parse the request body 
//  Important: Add a limit on how many characters a client can send 
vector<string> Server::sendParser(int clientSocket)
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

//  Function that lists everything inside the csv file for the specified user
void Server::listHandler(int clientSocket)
{
  //  temporary
  string sender = "if23b281";
  fstream fin;
  string mailSpoolFile = (mailSpoolDir.path / (sender + ".csv")).string();

  fin.open(mailSpoolFile, ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "File does not exist: " << mailSpoolFile << endl;
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

  for (const auto& subject : subjects) 
  {
      response += subject + "\n";
  }
  send(clientSocket, response.c_str(), response.size(), 0);
}

//  Function to read a specific message
void Server::readHandler(int clientSocket)
{
  int messageNr;
  try
  {
    messageNr = stoi(parser(clientSocket));
  }
  catch(const exception& e)
  {
    cerr << "Message number has to be of integer type." << endl;
    sendResponse(clientSocket, false);
    return;
  }

  string sender = "if23b281";
  fstream fin;
  string mailSpoolFile = (mailSpoolDir.path / (sender + ".csv")).string();

  fin.open(mailSpoolFile, ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "File does not exist: " << mailSpoolFile << endl;
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

// Function to delete a specific message
void Server::delHandler(int clientSocket)
{
  string sender = "if23b281";
  int messageNr;
  try
  {
    messageNr = stoi(parser(clientSocket));
  }
  catch(const exception& e)
  {
    sendResponse(clientSocket, false);
    cerr << "Message number has to be of integer type." << endl;
    cerr << "Error:" << e.what() << endl;
    return;
  }

  string mailSpoolFile = (mailSpoolDir.path / (sender + ".csv")).string();
  string tempFilePath = (mailSpoolDir.path / (sender + "Temp.csv")).string();

  fstream fin(mailSpoolFile, ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "File does not exist: " << mailSpoolFile << endl;
    return;
  }

  fstream tempFile(tempFilePath, ios::out);
  if(!tempFile.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "Error while opening temporary file for writing: " << tempFilePath << endl;
    fin.close();
    return;
  }

  string line;
  int count = 1;
  bool fileFound = false;

  // Copy the file contents without the specified line into a temporary file and replace the original file
  while(getline(fin, line))
  {
    //  Skip the line if it's found 
    if(count == messageNr)
    {
      fileFound = true;
    }
    //  Write all other lines into the temporary file
    else
    {
      tempFile << line << "\n";
    }
    count++;
  }

  fin.close();
  tempFile.close();

  if(fileFound)
  {
    if (remove(mailSpoolFile.c_str()) != 0 || rename(tempFilePath.c_str(), mailSpoolFile.c_str()) != 0) {
      sendResponse(clientSocket, false);
      cerr << "Error updating the mail file: " << mailSpoolFile << endl;
      return;
    }
    sendResponse(clientSocket, true);
  }
  else
  {
    remove(tempFilePath.c_str());
    sendResponse(clientSocket, false);
    cerr << "Message number not found: " << messageNr << endl;
  }
}

//  Function that handles simple responses to the client
void Server::sendResponse(int clientSocket, bool state)
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