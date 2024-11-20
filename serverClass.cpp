#include "allHeaders.h"
#include "serverClass.h"

using namespace std;
using namespace filesystem;

//  Server contructor initializes the addres struct and specified mail_spool
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

//  Method to gracefully shut the server down when SIGINT is caught
void Server::shutDown()
{
  cerr << "Shutting down server..." << endl;
  this->abortRequested = true;
  cleanUpThreads();
  shutdown(this->serverSocket, SHUT_RDWR);
  close(this->serverSocket);
}

//  Method that handles the server start 
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

//  Method that accepts clients until the server is shut down
void Server::acceptClients()
{
  while(!abortRequested)
  {
    cout << "Listening for client connections on port " << ntohs(this->serverAddress.sin_port) << endl;
    if(activeThreads.size() < MAX_CLIENTS)
    {
      int clientSocket = accept(this->serverSocket, nullptr, nullptr);
      if (clientSocket >= 0) 
      {
        //  Create a thread for each client connection and pass the function and parameter
        cout << "Client accepted." << endl;
        thread clientThread(&Server::clientHandler, this, clientSocket);

        lock_guard<mutex> lockMutex(this->threadsMutex);
        //  Move threads into the vector
        this->activeThreads.push_back(move(clientThread));
      }
      else 
      {
        cerr << "Error accepting client connection: " << strerror(errno) << endl;
        continue;
      }
    }
    else
    {
      cerr << "Server capacity is at the maximum." << endl;
    }
  }

  cleanUpThreads();
}

//  Helper method for acceptclient that joins all finished threads 
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

//  Method that handles the client input until QUIT is called 
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
    
    while(1)
    {
      cout << "Awaiting client request ..." << endl;
      string command = parser(clientSocket);
      if(!command.empty())
      {
        if(command == "QUIT")
        {
          cout << "Client has closed the connection." << endl;
          close(clientSocket);
          return;
        }
        commandHandler(clientSocket, command);
      }
      else
      {
        cerr << "Command cannot be empty." << endl;
        continue;
      }
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
}

//  Method to parse client input 
string Server::parser(int clientSocket)
{
  // Buffer reads the data, bytesRead determines the actual number of bytes read
  char buffer[BUFFER_SIZE];
  ssize_t bytesRead = 0;
  string line;

  // Read the data and save it into the buffer
  while (1)
  {
    // Clear the buffer 
    memset(buffer, 0, BUFFER_SIZE);
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead == 0)
    {
      cerr << "Client closed the connection." << endl;
      return "";
    }
    if (bytesRead < 0)
    {
      cerr << "Error while reading message body." << endl;
      return "";
    }
    
    buffer[bytesRead] = '\0';
    stringstream lineStream(buffer);

    //cout << "Received client input: " << buffer << endl;
    
    if(getline(lineStream, line, '\n'))
    {
      return line;
    }
  }
}

//  Method that handles each command 
void Server::commandHandler(int clientSocket, string command)
{
  cout << "Received command: " << command << endl;

  if(!isLoggedIn)
  {
    if(command == "LOGIN")
    {
      //  ADD IP ADDRESS PLEASE
      if(!isBlackListed("IP ADDRESS HERE"))
      {
        loginHandler(clientSocket);
      }
      else
      {
        cerr << "Client is currently blacklisted." << endl;
        sendResponse(clientSocket, false);
        return;
      }
    }
    else
    {
      cerr << "Client cannot access commands without being logged in." << endl;
      sendResponse(clientSocket, false);
    }
    return;
  }

  if(isLoggedIn)
  {
    if (command == "SEND")
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
    else
    {
      // Invalid command
      sendResponse(clientSocket, false);
      cerr << "Invalid Command." << endl;
    }
  }
}

//  Method that takes the client credentials and tries to log them in
bool Server::loginHandler(int clientSocket)
{
    // Parse the username from the client
    string username = parser(clientSocket);
    if (username.empty())
    {
        cerr << "Username cannot be empty." << endl;
        sendResponse(clientSocket, false);
        return false;
    }
    cout << "Username received: " << username << endl;

    // Parse the password from the client
    string password = parser(clientSocket);
    if (password.empty())
    {
        cerr << "Password cannot be empty." << endl;
        sendResponse(clientSocket, false);
        return false;
    }
    cout << "Password received." << endl;

    // Lock to ensure thread safety if multiple threads handle requests
    lock_guard<mutex> lock(this->threadsMutex);

    // Authenticate via LDAP
    if (establishLDAPConnection(username, password))
    {
        cout << "LDAP authentication successful for user: " << username << endl;
        this->username = username; // Save the username for session tracking
        isLoggedIn = true;         // Update login status
        sendResponse(clientSocket, true); // Send success response to client
        return true;
    }
    else
    {
        cerr << "LDAP authentication failed for user: " << username << endl;
        sendResponse(clientSocket, false); // Send failure response to client
        return false;
    }
}

//  Method to connect to the LDAP server and check if the clients credentials are valid
bool Server::establishLDAPConnection(const string& username, const string& password)
{
  LDAP* ldapHandle;
  int result;

  // Initialize LDAP connection
  result = ldap_initialize(&ldapHandle, ldapServer);
  if (result != LDAP_SUCCESS)
  {
    cerr << "Failed to initialize LDAP connection: " << ldap_err2string(result) << endl;
    return false;
  }
  cout << "LDAP connection initialized successfully." << endl;

  // Set the LDAP protocol version to 3 (recommended)
  int protocolVersion = LDAP_VERSION3;
  result = ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &protocolVersion);
  if (result != LDAP_SUCCESS)
  {
    cerr << "Failed to set LDAP protocol version: " << ldap_err2string(result) << endl;
    ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
    return false;
  }
  cout << "LDAP protocol version set to 3." << endl;

  // Construct the DN (Distinguished Name) for binding
  string bindDN = "uid=" + username + "," + ldapBind;  // Construct the DN based on the username and base DN
  BerValue bindCredentials;
  bindCredentials.bv_val = const_cast<char*>(password.c_str());
  bindCredentials.bv_len = password.length();

  // Attempt to bind with the provided credentials
  result = ldap_sasl_bind_s(ldapHandle, bindDN.c_str(), LDAP_SASL_SIMPLE, &bindCredentials, nullptr, nullptr, nullptr);

  if (result != LDAP_SUCCESS)
  {
    cerr << "LDAP bind failed: " << ldap_err2string(result) << endl;
    ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
    checkLoginAttempts();
    return false;
  }

  cout << "LDAP bind successful." << endl;

  // Unbind from the server after authentication
  ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
  return true;
}

//  Method that keeps track of the clients login attempts and blacklists them if needed
void Server::checkLoginAttempts()
{
  lock_guard<mutex> lock(this->threadsMutex);
  if(loginAttempts == maxLoginAttempts)
  {
    //  IP!!!
    blackList.push_back("CLIENT IP");
    this_thread::sleep_for(chrono::minutes(1));
    
    /*for(auto &ipAddr : this->blackList)
    {
      
      if(ipAddr == ip)
      {
        return blackList.erase(ip);
        blackList.resize();
      }
    }*/
  }
  else
  {
    loginAttempts++;
  }
}

//  Method that checks if the client is black listed
bool Server::isBlackListed(string ip)
{
  lock_guard<mutex> lock(this->threadsMutex);
  for(auto &ipAddr : this->blackList)
  {
    if(ipAddr == ip)
    {
      return true;
    }
  }
  return false;
}

//  Method to save a sent message inside a csv file
void Server::sendHandler(int clientSocket)
{
  vector<string> body = sendParser(clientSocket);
  if(body.empty() || body.size() < 3)
  {
    sendResponse(clientSocket, false);
    cerr << "Invalid SEND command syntax." << endl;
    return;
  }

  string receiver = body[0];
  string subject = body[1];
  string message = body[2];

  cout << "Receiver: " << receiver << endl;
  cout << "Subject: " << subject << endl;
  cout << "Message: " << message << endl;

  fstream fout;
  //  Save the new file name as sender.csv inside the Mail Spool Directory location
  string mailSpoolFile = getMailSpoolFile();

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

//  Helper Method of sendHandler to parse the request body 
//  Important: Add a limit on how many characters a client can send 
vector<string> Server::sendParser(int clientSocket)
{
  vector<string> body;
  string line;

  while(1)
  {
    //  Use the parser function to extract each line
    line = parser(clientSocket);
    if(line.empty())
    {
      body.clear();
      return body;
    }

    if (line == ".") 
    {
      break;
    }

    if(body.size() < 3)
    {
      // Save receiver & subject
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

//  Method that lists the subjects inside the csv file for the specified user
void Server::listHandler(int clientSocket)
{
  fstream fin;
  string mailSpoolFile = getMailSpoolFile();

  fin.open(mailSpoolFile, ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "File does not exist: " << mailSpoolFile << endl;
    return;
  }

  string line;
  vector<string> subjects;
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
  
  if(subjects.empty())
  {
    cerr << "No messages found." << endl;
    sendResponse(clientSocket, false);
    return;
  }

  string response = "Message Count: " + to_string(subjects.size()) + "\n";

  for (const auto& subject : subjects) 
  {
    response += subject + "\n";
  }
  if((send(clientSocket, response.c_str(), response.size(), 0)) == -1)
  {
    cerr << "Error while sending LIST data to client." << endl;
    sendResponse(clientSocket, false);
    return;
  }

  cout << "LIST data sent to client succesfully." << endl;
}

//  Method to read a message specified by the client
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

  fstream fin;
  string mailSpoolFile = getMailSpoolFile();

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
    return;
  }
  
  sendResponse(clientSocket, true);
  if((send(clientSocket, response.c_str(), response.size(), 0)) == -1)
  {
    cerr << "Error while sending READ data to client." << endl;
    sendResponse(clientSocket, false);
    return;
  }
  cout << "LIST data sent to client successfully." << endl;
}

// Method to delete a message specificied by the client
void Server::delHandler(int clientSocket)
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
    cerr << "Error:" << e.what() << endl;
    return;
  }

  string mailSpoolFile = getMailSpoolFile();
  string tempFilePath = (mailSpoolDir.path / (this->username + "Temp.csv")).string();

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
    cout << "Message" << messageNr << "deleted successfully." << endl;
  }
  else
  {
    remove(tempFilePath.c_str());
    sendResponse(clientSocket, false);
    cerr << "Message number not found: " << messageNr << endl;
  }
}

//  Method that returns the users Mail-Spool file
string Server::getMailSpoolFile()
{
  //  Critical section
  lock_guard<mutex> lockMutex(this->threadsMutex);
  return (mailSpoolDir.path / (this->username + ".csv")).string();
}

//  Method that handles simple responses to the client
void Server::sendResponse(int clientSocket, bool state)
{
  if(state)
  {
    if((send(clientSocket, "OK\n", 3, 0)) == -1)
    {
      cerr << "Error occured while sending OK response to client." << endl;
    }
    else
    {
      cerr << "OK reponse sent to client successfully." << endl;
    }
  }
  else
  {
    if((send(clientSocket, "ERR\n", 4, 0)) == -1)
    {
      cerr << "Error occured while sending ERR response to client." << endl;
    }
    else
    {
      cerr << "ERR reponse sent to client successfully." << endl;
    }
  }
}