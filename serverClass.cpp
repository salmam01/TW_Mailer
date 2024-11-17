#include "serverClass.h"
#include "serverHeaders.h"

Server::Server(int port, std::string mailSpoolDir)
{
  //  AF_INET = IPv4
  memset(&this->serverAddress, 0, sizeof(this->serverAddress));
  this->serverAddress.sin_family = AF_INET;
  this->serverAddress.sin_port = htons(port);
  this->serverAddress.sin_addr.s_addr = INADDR_ANY;

  this->mailSpoolDir = mailSpoolDir;
} 

bool Server::start()
{
  //  Create a new socket
  if((this->serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    std::cerr << "Error while creating socket" << std::endl;
    return false;
  }

  if (setsockopt(this->serverSocket,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &reuseValue,
                      sizeof(reuseValue)) == -1)
  {
    std::cerr << "Set socket options - reuseAddr" << std::endl;
    return false;
  }

  if (setsockopt(this->serverSocket,
                      SOL_SOCKET,
                      SO_REUSEPORT,
                      &reuseValue,
                      sizeof(reuseValue)) == -1)
      {
      std::cerr << "Set socket options - reusePort" << std::endl;
      return false;
  }

  std::cout << "Port: " << this->serverAddress.sin_port << std::endl;
  std::cout << "Mail-Spool Directory: " << this->mailSpoolDir << std::endl;

  //  Binding socket
  if(bind(this->serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
  {
    std::cerr << "Error while binding socket." << std::endl;
    close(this->serverSocket);
    return false;
  }

  //  Listen for incoming clients
  if(listen(this->serverSocket, 5) < 0)
  {
    std::cerr << "Error while listening for clients." << std::endl;
    close(this->serverSocket);
    return false;
  }

  //  Accept client
  if(!acceptClients())
  {
    return false;
  }

  close(this->serverSocket);
  return true;
}

bool Server::acceptClients()
{
  while(!abortRequested)
  {
    std::cout << "Listening for client connections on port " << this->serverAddress.sin_port << std::endl;
    int clientSocket = accept(this->serverSocket, nullptr, nullptr);
    if (clientSocket >= 0) 
    {
      //  Create a thread for each client connection and pass the function and parameter
      std::cout << "Client accepted." << std::endl;
      std::thread clientThread(&Server::clientHandler, this, clientSocket);
      //  Main thread continues executing without waiting for the clientThread
      clientThread.detach();
    }
    else 
    {
      std::cerr << "Error accepting client connection." << std::endl;
      return false;
    }
  }
  return true;
}

bool Server::clientHandler(int clientSocket)
{
  try
  {
    std::string welcomeMessage = "Welcome to our Mail Server! Please login with LOGIN.\n";
    if (send(clientSocket, welcomeMessage.c_str(), strlen(welcomeMessage), 0) == -1)
    {
      // Close the socket on failure
      std::cerr << "Send failed." << std::endl;
      return false;
    }

    std::string command = parser(clientSocket);
    if(command.empty())
    {
      std::cerr << "Command cannot be empty." << std::endl;
      return false;
    }

    //  Pass the command to the commandHandler if it's not empty
    commandHandler(clientSocket, command);
    return true;

  }
  catch(const std::exception &e)
  {
    std::cerr << "Exception in clientHandler: " << e.what() << std::endl;
    sendResponse(clientSocket, false);
    close(clientSocket);
  }
  catch(...)
  {
    std::cerr << "An unknown error occurred. Connection will terminate." << std::endl;
    sendResponse(clientSocket, false);
    close(clientSocket);
  }
}

std::string Server::parser(int clientSocket)
{
  // Buffer reads the data, bytesRead determines the actual number of bytes read
  char buffer[BUFFER_SIZE];
  ssize_t bytesRead = 0;
  std::string line;

  // Read the data and save it into the receivedData string
  while (true)
  {
      // Clear the buffer and read up to buffer_size - 1 bytes
      memset(buffer, 0, BUFFER_SIZE);
      bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

      if (bytesRead <= 0)
      {
          std::cerr << "Error while reading message body." << std::endl;
          close(clientSocket);
          return "";
      }

      buffer[bytesRead] = '\0'; // Null terminate the buffer
      line += buffer; // Append buffer content to the line

      // Debug output
      std::cout << "Received client input: " << buffer << std::endl;

      // If a newline character is found, we have the full line
      if (line.find("\n") != std::string::npos)
      {
          break;
      }
  }

  // Strip the newline character at the end, if present
  size_t pos = line.find("\n");
  if (pos != std::string::npos)
  {
      line = line.substr(0, pos);  // Remove the newline
  }

  return line;
}

void Server::commandHandler(int clientSocket, std::string command)
{
  std::cout << "Received command: " << command << std::endl;
  
  if (command == "LOGIN")
  {
    std::cout << "Handling LOGIN command" << std::endl;
    if (loginHandler(clientSocket)) {
        sendResponse(clientSocket, true);  // OK LOGIN SUCCESSFUL
    } else {
        sendResponse(clientSocket, false); // ERR LOGIN FAILED
    }
  }
  else if (command == "SEND")
  {
    std::cout << "Handling SEND command" << std::endl;
    if (sendHandler(clientSocket)) {
        sendResponse(clientSocket, true);  // OK MESSAGE SENT
    } else {
        sendResponse(clientSocket, false); // ERR MESSAGE SEND FAILED
    }
  }
  else if (command == "LIST")
  {
    std::cout << "Handling LIST command" << std::endl;
    listHandler(clientSocket);
  }
  else if (command == "READ")
  {
    std::cout << "Handling READ command" << std::endl;
    readHandler(clientSocket);
  }
  else if (command == "DEL")
  {
    std::cout << "Handling DEL command" << std::endl;
    delHandler(clientSocket);
  }
  else if (command == "QUIT")
  {
    std::cout << "Handling QUIT command" << std::endl;
    sendResponse(clientSocket, true);  // OK CONNECTION CLOSED
    close(clientSocket);  // Close the connection after QUIT
  }
  else
  {
    // Invalid command
    sendResponse(clientSocket, false); // ERR INVALID COMMAND
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
    std::cerr << "Error reading username from client." << std::endl;
    return false;
  }

  // Null-terminate the string received from the client
  buffer[bytesRead] = '\0';
  std::string username(buffer);
  std::cout << "User entered: " << username << std::endl; // Log the username

  // Read the password sent by the client
  memset(buffer, 0, sizeof(buffer)); // Clear the buffer
  bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
  
  if (bytesRead <= 0)
  {
    std::cerr << "Error reading password from client." << std::endl;
    return false;
  }

  // Null-terminate the string received from the client
  buffer[bytesRead] = '\0';
  std::string password(buffer);
  std::cout << "Password entered: " << password << std::endl; // Log the password

  // For testing
  return true;
}


/*
SEND\n 
<Receiver>\n 
<Subject (max. 80 chars)>\n 
<message (multi-line; no length restrictions)\n> 
.\n  
*/
// -------------------------------------->MISSING PATH SPECIFICATION FOR ALL HANDLER FUNCTIONS<-----------------------------------------------
//  Ignore the fact that this is a bool for now, it will come in handy later
bool Server::sendHandler(int clientSocket)
{
  std::vector<std::string> body = sendParser(clientSocket);
  if(body.empty() || body.size() < 3)
  {
    sendResponse(clientSocket, false);
    std::cerr << "Invalid command syntax." << std::endl;
    return false;
  }

  //  get the actual sender at some point, this just for testing
  std::string sender = "if23b281";
  std::string receiver = body[0];
  std::string subject = body[1];
  std::string message = body[2];

  std::fstream fout;
  std::string mailSpoolName = sender + ".csv";

  //  Open or create a new file with the name
  fout.open(mailSpoolName, std::ios::out | std::ios::app);
  //  Check if the file was opened successfully
  if(!fout.is_open())
  {
    sendResponse(clientSocket, false);
    std::cerr << "Error occurred while opening Mail-Spool file." << std::endl;
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
    std::cerr << "Error writing to Mail-Spool file." << std::endl;
    sendResponse(clientSocket, false);
    return false;
  }

  fout.close();

  sendResponse(clientSocket, true);
  return true;
}

//  ??? hopefully this shit works now
std::vector<std::string> Server::sendParser(int clientSocket)
{
  std::vector<std::string> body;
  std::string line;

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
void Server::listHandler(int clientSocket)
{
  //  temporary
  std::string sender = "if23b281";
  std::fstream fin;
  std::string mailSpoolName = sender + ".csv";

  fin.open(mailSpoolName, std::ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    std::cerr << "File does not exist." << std::endl;
    return;
  }

  std::string line;
  std::vector<std::string> subjects;
  //  Loop until the end of the file
  while(getline(fin, line))
  {
    std::stringstream ss(line);
    std::string receiver, subject, message;

    getline(ss, receiver, ';');
    getline(ss, subject, ';');
    getline(ss, message);

    subjects.push_back(subject);
  }
  fin.close();
  
  std::string response = "Message Count: " + std::to_string(subjects.size()) + "\n";

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
void Server::readHandler(int clientSocket)
{
  int messageNr;
  try
  {
    messageNr = stoi(parser(clientSocket));
  }
  catch(const std::exception& e)
  {
    sendResponse(clientSocket, false);
    std::cerr << "Message number has to be of integer type." << std::endl;
    return;
  }

  std::string sender = "if23b281";
  std::fstream fin;
  std::string mailSpoolName = sender + ".csv";

  fin.open(mailSpoolName, std::ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    std::cerr << "File does not exist." << std::endl;
    return;
  }

  std::string line;
  std::string response;
  int count = 1;
  while(getline(fin, line))
  {
    std::stringstream ss(line);
    std::string receiver, subject, message;

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
    std::cerr << "Message does not exist." << std::endl;
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
// Copy the file contents without the specified line into a temporary file and replace the original file
void Server::delHandler(int clientSocket)
{
  std::string sender = "if23b281";
  int messageNr;
  try
  {
    messageNr = stoi(parser(clientSocket));
  }
  catch(const std::exception& e)
  {
    sendResponse(clientSocket, false);
    std::cerr << "Message number has to be of integer type." << std::endl;
    std::cerr << "Error:" << e.what() << std::endl;
    return;
  }

  //fstream fin;
  //fstream tempFile;
  //  Should probably save the name somewhere atp...
  std::string mailSpoolName = sender + ".csv";
  std::string tempFileName = sender + "Temp.csv";

  std::fstream fin(mailSpoolName, std::ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    std::cerr << "File does not exist." << std::endl;
    return;
  }

  std::fstream tempFile(tempFileName, std::ios::out);
  if(!tempFile.is_open())
  {
    sendResponse(clientSocket, false);
    std::cerr << "Error while opening temporary file for writing." << std::endl;
    fin.close();
    return;
  }

  std::string line;
  int count = 1;
  bool fileFound = false;

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
    if (remove(mailSpoolName.c_str()) != 0 || rename(tempFileName.c_str(), mailSpoolName.c_str()) != 0) {
        sendResponse(clientSocket, false);
        std::cerr << "Error updating the mail file." << std::endl;
        return;
    }
    sendResponse(clientSocket, true);
  }
  else
  {
    remove(tempFileName.c_str());
    sendResponse(clientSocket, false);
    std::cerr << "Message number not found." << std::endl;
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

bool Server::establishLDAPConnection(const std::string& bindPassword)
{
    LDAP* ldapHandle;
    int result;

    // Initialize LDAP connection using the class member ldapServer
    result = ldap_initialize(&ldapHandle, ldapServer);
    if (result != LDAP_SUCCESS)
    {
        std::cerr << "Failed to initialize LDAP connection: " << ldap_err2string(result) << std::endl;
        return false;
    }
    std::cout << "LDAP connection initialized successfully." << std::endl;

    // Set LDAP options (optional but recommended)
    int protocolVersion = LDAP_VERSION3;
    result = ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &protocolVersion);
    if (result != LDAP_SUCCESS)
    {
        std::cerr << "Failed to set LDAP protocol version: " << ldap_err2string(result) << std::endl;
        ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
        return false;
    }
    std::cout << "LDAP protocol version set to 3." << std::endl;

    // Bind to the LDAP server using the class member ldapBind
    BerValue bindCredentials;
    bindCredentials.bv_val = const_cast<char*>(bindPassword.c_str());
    bindCredentials.bv_len = bindPassword.length();

    BerValue* serverCredentials = nullptr;
    result = ldap_sasl_bind_s(ldapHandle, ldapBind, LDAP_SASL_SIMPLE, &bindCredentials, nullptr, nullptr, &serverCredentials);

    if (result != LDAP_SUCCESS)
    {
        std::cerr << "LDAP bind failed: " << ldap_err2string(result) << std::endl;
        ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
        return false;
    }
    std::cout << "LDAP bind successful." << std::endl;

    // Connection is established successfully
    ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
    return true;
}





