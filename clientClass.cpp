#include "clientHeaders.h"
#include "clientClass.h"

using namespace std;

Client::Client(const char *ip, int port)
    {
        // Initialize socket
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1)
        {
            cerr << "Socket error" << endl;
            exit(EXIT_FAILURE);
        }

        // Initialize address structure
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        inet_aton(ip, &server_address.sin_addr);
    }

bool Client::connect_to_server()
    {
        if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        {
            cerr << "Connection error - no server available" << endl;
            return false;
        }
        printf("Connection with server (%s) established\n", inet_ntoa(server_address.sin_addr));
        return true;
    }

void Client::receive_data()
    {
        int size = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (size == -1)
        {
            cerr << "recv error" << endl;
        }
        else if (size == 0)
        {
            printf("Server closed remote socket\n");
        }
        else
        {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

bool Client::send_command(const string &command)
    {
        if (send(socket_fd, command.c_str(), command.length(), 0) == -1)
        {
            cerr << "Error: " << strerror(errno) << endl;
            return false;
        }
        return true;
    }

void Client::close_connection()
    {
        if (socket_fd != -1)
        {
            if (shutdown(socket_fd, SHUT_RDWR) == -1)
            {   
                cerr << "Error: shutdown socket" << endl;
            }
            if (close(socket_fd) == -1)
            {
                cerr << "Error: close socket" << endl;
            }
            socket_fd = -1;
        }
    }

int Client::get_socket_fd() const
    {
        return socket_fd;
    }

char* Client::get_buffer()
    {
        return buffer;
    }

int Client::specificMessage(int socket){
   string msgNumber;
   cout << "Number of messages: ";
   getline(cin, msgNumber);
   if ((send(socket, msgNumber.c_str(), msgNumber.size(), 0)) == -1) 
      {
         cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   return 1;
}

int Client::getch() {
   int ch;
   struct termios t_old, t_new;

   // Get the current terminal attributes
   tcgetattr(STDIN_FILENO, &t_old);
   t_new = t_old;

   // Disable canonical mode and echoing
   // Canonical mode: entered data(in buffer) becomes available after line editing termination
   t_new.c_lflag &= ~(ICANON | ECHO);

   // Apply the new terminal settings
   tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

   ch = getchar(); // Read single character

   // Restore the original terminal settings
   tcsetattr(STDIN_FILENO, TCSANOW, &t_old);

   return ch;
}

string Client::getpass() {
   const char BACKSPACE = 127;
   const char RETURN = 10;

   unsigned char ch = 0;
   string password;

   cout << "Password: ";

   while ((ch = getch()) != RETURN) {
      if (ch == BACKSPACE) {
         if (!password.empty()) {
               password.pop_back();
         }
      } else {
         password += ch;
      }
   }

   cout << endl;
   return password;
}

int Client::loginCommand(int socket){
   if ((send(socket, "LOGIN", 6, 0)) == -1) 
      {
         cerr << "Error sending LOGIN command: " << strerror(errno) << endl;
         return -1;
      }

   string username;
   string password;

   cout << "User: ";
   getline(cin, username);
   if ((send(socket, username.c_str(), username.size(), 0)) == -1) 
      {
         cerr << "Error sending username: " << strerror(errno) << endl;
         return -1;
      }

   password = getpass();
   if ((send(socket, password.c_str(), password.size(), 0)) == -1) {
      cerr << "Error sending password: " << strerror(errno) << endl;
      return -1;
   }

   password.clear();

   char buffer[256] = {0};
   int size = recv(socket, buffer, sizeof(buffer) - 1, 0);
   if (size <= 0)
   {
      cerr << "Error receiving server response: " << strerror(errno) << endl;
      return -1;
   }
   buffer[size] = '\0';

   if (strstr(buffer, "<< OK"))
   {
      cout << "Login successful!" << endl;
      Client::isLoggedIn = true;
      return 1;
   }
   else if (strstr(buffer, "<< ERR"))
   {
      cout << "Login failed: " << buffer << endl;
      return -1;
   }
   else
   {
      cerr << "Unexpected server response: " << buffer << endl;
      return -1;
   }
}

int Client::sendCommand(int socket){
   if ((send(socket, "SEND", 4, 0)) == -1) 
      {
         cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   string receiver;
   cout << "Receiver: ";
   getline(cin, receiver);
   if ((send(socket, receiver.c_str(), receiver.size(), 0)) == -1) 
      {
         cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   string subject;
   cout << "Subject(max. 80 chars): ";
   getline(cin, subject);
   if(subject.size()>80){
        cerr << "Error: Maximum size of 80 characters for subject!" << endl;
        return -1;
   }
   if ((send(socket, subject.c_str(), subject.size(), 0)) == -1) 
      {
         cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   string message;
    cout << "Message (end with '.' on new line.):\n";
    while (true) {
        getline(cin, message);
        if ((send(socket, message.c_str(), message.size(), 0)) == -1) 
         {
            cerr << "Error: " << strerror(errno) << endl;
            return -1;
         }
         if (message == ".")
            break;
    }
   
   return 1;
}

int Client::listCommand(int socket){
   if ((send(socket, "LIST", 4, 0)) == -1) 
      {
         cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   return 1;
}

int Client::readCommand(int socket){
   if ((send(socket, "READ", 4, 0)) == -1) 
      {
         cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   if(specificMessage(socket)==-1){
      return -1;
   }
   
   return 1;
}

int Client::delCommand(int socket){
   if ((send(socket, "DEL", 4, 0)) == -1) 
      {
        cerr << "Error: " << strerror(errno) << endl;
         return -1;
      }
   
   if(specificMessage(socket)==-1){
      return -1;
   }
   
   return 1;
}