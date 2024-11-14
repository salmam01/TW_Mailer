#include "serverClass.h"

Server::Server(int port, string mailSpoolDir)
{
  this->port = port;
  this->mailSpoolDir = mailSpoolDir;
}

Server::bool start()
{
  //  Create a new socket
  if((this->serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    cerr << "Error while creating socket" << endl;
    return false;
  }

  if (setsockopt(this->serverSocket,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &reuseValue,
                      sizeof(reuseValue)) == -1)
  {
    cerr << "Set socket options - reuseAddr" << endl;
    return false;
  }

  if (setsockopt(this->serverSocket,
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
  if(bind(this->serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
  {
    cerr << "Error while binding socket." << endl;
    close(this->serverSocket);
    return false;
  }

  //  Listen for incoming clients
  if(listen(this->serverSocket, 5) < 0)
  {
    cerr << "Error while listening for clients." << endl;
    close(this->serverSocket);
    return false;
  }

  //  Accept client
  if(!acceptClients(this->serverSocket))
  {
    return false;
  }
  close(this->serverSocket);
  return true;
}

Server::bool acceptClients()
{
  while(!abortRequested)
  {
    cout << "Listening for client connections..." << endl;
    int clientSocket = accept(this->serverSocket, nullptr, nullptr);
    if (clientSocket >= 0) 
    {
        cout << "Client accepted" << endl;
    }
    else 
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

Server::bool clientHandler(int clientSocket)
{
  const char* welcomeMessage = "Welcome to myserver! Please login with LOGIN.\n";
  if (send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0) == -1)
  {
    perror("send failed");
    close(clientSocket);  // Close the socket on failure
    return false;
  }

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

Server::string parser(int clientSocket)
{
  // Buffer reads the data, bytesRead determines the actual number of bytes read
  char buffer[BUFFER_SIZE];
  ssize_t bytesRead = 0;
  string line;

  // Read the data and save it into the receivedData string
  while (true)
  {
      // Clear the buffer and read up to buffer_size - 1 bytes
      memset(buffer, 0, BUFFER_SIZE);
      bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

      if (bytesRead <= 0)
      {
          cerr << "Error while reading message body." << endl;
          close(clientSocket);
          return "";
      }

      buffer[bytesRead] = '\0'; // Null terminate the buffer
      line += buffer; // Append buffer content to the line

      // Debug output
      cout << "Received client input: " << buffer << endl;

      // If a newline character is found, we have the full line
      if (line.find("\n") != string::npos)
      {
          break;
      }
  }

  // Strip the newline character at the end, if present
  size_t pos = line.find("\n");
  if (pos != string::npos)
  {
      line = line.substr(0, pos);  // Remove the newline
  }

  return line;
}

Server::void commandHandler(int clientSocket, string command)
{
  cout << "Received command: " << command << endl;
  
  if (command == "LOGIN")
  {
    cout << "Handling LOGIN command" << endl;
    if (loginHandler(clientSocket)) {
        sendResponse(clientSocket, true);  // OK LOGIN SUCCESSFUL
    } else {
        sendResponse(clientSocket, false); // ERR LOGIN FAILED
    }
  }
  else if (command == "SEND")
  {
    cout << "Handling SEND command" << endl;
    if (sendHandler(clientSocket)) {
        sendResponse(clientSocket, true);  // OK MESSAGE SENT
    } else {
        sendResponse(clientSocket, false); // ERR MESSAGE SEND FAILED
    }
  }
  else if (command == "LIST")
  {
    cout << "Handling LIST command" << endl;
    listHandler(clientSocket);
  }
  else if (command == "READ")
  {
    cout << "Handling READ command" << endl;
    readHandler(clientSocket);
  }
  else if (command == "DEL")
  {
    cout << "Handling DEL command" << endl;
    delHandler(clientSocket);
  }
  else if (command == "QUIT")
  {
    cout << "Handling QUIT command" << endl;
    sendResponse(clientSocket, true);  // OK CONNECTION CLOSED
    close(clientSocket);  // Close the connection after QUIT
  }
  else
  {
    // Invalid command
    sendResponse(clientSocket, false); // ERR INVALID COMMAND
  }
}

Server::bool loginHandler(int clientSocket)
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


/*
SEND\n 
<Receiver>\n 
<Subject (max. 80 chars)>\n 
<message (multi-line; no length restrictions)\n> 
.\n  
*/
// -------------------------------------->MISSING PATH SPECIFICATION FOR ALL HANDLER FUNCTIONS<-----------------------------------------------
//  Ignore the fact that this is a bool for now, it will come in handy later
Server::bool sendHandler(int clientSocket)
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
    sendResponse(clientSocket, false);
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
Server::vector<string> sendParser(int clientSocket)
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
Server::void listHandler(int clientSocket)
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
Server::void readHandler(int clientSocket)
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
// Copy the file contents without the specified line into a temporary file and replace the original file
Server::void delHandler(int clientSocket)
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
    return;
  }

  //fstream fin;
  //fstream tempFile;
  //  Should probably save the name somewhere atp...
  string mailSpoolName = sender + ".csv";
  string tempFileName = sender + "Temp.csv";

  fstream fin(mailSpoolName, ios::in);
  if(!fin.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "File does not exist." << endl;
    return;
  }

  fstream tempFile(tempFileName, ios::out);
  if(!tempFile.is_open())
  {
    sendResponse(clientSocket, false);
    cerr << "Error while opening temporary file for writing." << endl;
    fin.close();
    return;
  }

  string line;
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
        cerr << "Error updating the mail file." << endl;
        return;
    }
    sendResponse(clientSocket, true);
  }
  else
  {
    remove(tempFileName.c_str());
    sendResponse(clientSocket, false);
    cerr << "Message number not found." << endl;
  }
}

//  Function that handles simple responses to the client
Server::void sendResponse(int clientSocket, bool state)
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


/*
int connectLdap(int client_socket)
{
  LDAP *ld;
  int ldapVersion = LDAP_VERSION3;
  int rc = 0;

  char buffer[BUFFER_SIZE];
  string username, password;
  
  memset(buffer, 0, BUFFER_SIZE);
  // Get username from client
  recv(client_socket, buffer, sizeof(buffer), 0);
  username = buffer;
  memset(buffer, 0, BUFFER_SIZE);
  // Get password from client
  recv(client_socket, buffer, sizeof(buffer), 0);
  password = buffer;

  // Initialize LDAP connection
  rc = ldap_initialize(&ld, ldapServer);
  if (rc != LDAP_SUCCESS)
  {
      fprintf(stderr, "ldap_init failed\n");
      return EXIT_FAILURE;
  }
  printf("connected to LDAP server %s\n", ldapServer);

  // Set LDAP protocol version to 3.0
  rc = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);
  if (rc == LDAP_SUCCESS)
      printf("ldap_set_option succeeded - version set to 3\n");
  else
  {
      printf("SetOption Error:%0X\n", rc);
  }

  // Start a secure TLS connection
  rc = ldap_start_tls_s(ld, NULL, NULL);
  if (rc != LDAP_SUCCESS)
  {
      fprintf(stderr, "ldap_start_tls_s() failed: %s\n", ldap_err2string(rc));
      ldap_unbind_ext_s(ld, NULL, NULL);
      return -1;
  }

  // Prepare the DN for SASL authentication
  char dn[256];
  sprintf(dn, "uid=%s,%s", username.c_str(), ldapBind);
  cout << "Attempting SASL bind with DN: " << dn << endl;
  // Perform SASL bind (using the PLAIN authentication method)
  rc = ldap_sasl_bind_s(ld, dn, LDAP_SASL_SIMPLE, &password[0], NULL, NULL, NULL);
  if (rc != LDAP_SUCCESS)
  {
      loginAttempts += 1;
      cerr << "Authentication failed for user " << username << endl;
      if (loginAttempts == maxLoginAttempts)
      {
          // Lock the account after 3 failed login attempts
          strcpy(buffer, "Too many login attempts! You are blacklisted for 1 minute.\n");
          if (send(client_socket, buffer, strlen(buffer), 0) == -1)
          {
              perror("send failed");
              return -1;
          }
          std::this_thread::sleep_for(std::chrono::minutes(1));
          loginAttempts = 0;
      }
      ldap_unbind_ext_s(ld, NULL, NULL);
      return -1;
  }

  cout << "SASL bind as " << username << " was successful!" << endl;

  // Save the current user
  currentUser = username;

  // Unbind and close the LDAP connection
  ldap_unbind_ext_s(ld, NULL, NULL);
  return 0;
}
*/





