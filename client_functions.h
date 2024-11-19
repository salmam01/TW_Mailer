#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <termios.h>

using namespace std;


int specificMessage(int socket);

int sendCommand(int socket){
   if ((send(socket, "SEND", 4, 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   string receiver;
   cout << "Receiver: ";
   getline(cin, receiver);
   if ((send(socket, receiver.c_str(), receiver.size(), 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   string subject;
   cout << "Subject(max. 80 chars): ";
   getline(cin, subject);
   if(subject.size()>80){
      perror("Maximum size of 80 characters for subject!\n");
      return -1;
   }
   if ((send(socket, subject.c_str(), subject.size(), 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   string message;
    std::cout << "Message (end with '.' on new line.):\n";
    while (true) {
        getline(cin, message);
        if ((send(socket, message.c_str(), message.size(), 0)) == -1) 
         {
            perror("send error");
            return -1;
         }
         if (message == ".")
            break;
    }
   
   return 1;
}

int listCommand(int socket){
   if ((send(socket, "LIST", 4, 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   return 1;
}

int readCommand(int socket){
   if ((send(socket, "READ", 4, 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   if(specificMessage(socket)==-1){
      return -1;
   }
   
   return 1;
}

int delCommand(int socket){
   if ((send(socket, "DEL", 4, 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   if(specificMessage(socket)==-1){
      return -1;
   }
   
   return 1;
}

int specificMessage(int socket){
   string msgNumber;
   cout << "Number of message: ";
   getline(cin, msgNumber);
   if ((send(socket, msgNumber.c_str(), msgNumber.size(), 0)) == -1) 
      {
         perror("send error");
         return -1;
      }
   
   return 1;
}

int getch() {
   int ch;
   struct termios t_old, t_new;

   // Get the current terminal attributes
   tcgetattr(STDIN_FILENO, &t_old);
   t_new = t_old;

   // Disable canonical mode and echoing
   t_new.c_lflag &= ~(ICANON | ECHO);

   // Apply the new terminal settings
   tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

   // Read a single character
   ch = getchar();

   // Restore the original terminal settings
   tcsetattr(STDIN_FILENO, TCSANOW, &t_old);

   return ch;
}

string getpass() {
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

int loginCommand(int socket){
   if ((send(socket, "LOGIN", 6, 0)) == -1) 
      {
         perror("send error");
         return -1;
      }

   string username;
   string password;

   cout << "User: ";
   getline(cin, username);
   if ((send(socket, username.c_str(), username.size(), 0)) == -1) 
      {
         perror("send error");
         return -1;
      }

   password = getpass();
   if ((send(socket, password.c_str(), password.size(), 0)) == -1) {
      perror("send error");
      return -1;
   }

   return 1;
}

#endif