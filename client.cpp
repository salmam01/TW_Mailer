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
#include <algorithm> //for transform
#include <ctype.h> //for toupper
#include "client_functions.h"
using namespace std;

///////////////////////////////////////////////////////////////////////////////

#define BUF 1024

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
   int create_socket;
   char buffer[BUF];
   struct sockaddr_in address;
   int size;
   int isQuit = 0;

   if(argc != 3){
      cerr << "Usage: ./client <ip> <port>\n";
      return EXIT_FAILURE;
   }

   std::istringstream iss(argv[2]);
   int port;
   if(!(iss >> port)){
      cerr << "Invalid port - not a number\n";
      return EXIT_FAILURE;
   }
   ////////////////////////////////////////////////////////////////////////////
   // CREATE A SOCKET
   // https://man7.org/linux/man-pages/man2/socket.2.html
   // https://man7.org/linux/man-pages/man7/ip.7.html
   // https://man7.org/linux/man-pages/man7/tcp.7.html
   // IPv4, TCP (connection oriented), IP (same as server)
   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket error");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // INIT ADDRESS
   // Attention: network byte order => big endian
   memset(&address, 0, sizeof(address)); // init storage with 0
   address.sin_family = AF_INET;         // IPv4
   // https://man7.org/linux/man-pages/man3/htons.3.html
   address.sin_port = htons(port);
   inet_aton(argv[1], &address.sin_addr); //127.0.0.1 for now
   // https://man7.org/linux/man-pages/man3/inet_aton.3.html

   ////////////////////////////////////////////////////////////////////////////
   // CREATE A CONNECTION
   // https://man7.org/linux/man-pages/man2/connect.2.html
   if (connect(create_socket,
               (struct sockaddr *)&address,
               sizeof(address)) == -1)
   {
      // https://man7.org/linux/man-pages/man3/perror.3.html
      perror("Connect error - no server available");
      return EXIT_FAILURE;
   }

   // ignore return value of printf
   printf("Connection with server (%s) established\n",
          inet_ntoa(address.sin_addr));

   ////////////////////////////////////////////////////////////////////////////
   // RECEIVE DATA
   // https://man7.org/linux/man-pages/man2/recv.2.html
   size = recv(create_socket, buffer, BUF - 1, 0);
   if (size == -1)
   {
      perror("recv error");
   }
   else if (size == 0)
   {
      printf("Server closed remote socket\n"); // ignore error
   }
   else
   {
      buffer[size] = '\0';
      printf("%s", buffer); // ignore error
   }
   memset(buffer, 0, BUF);

   do
   {
      printf(">> ");
      string command;
      getline(cin, command);
      transform(command.begin(), command.end(), command.begin(), ::toupper);
      if(command=="SEND"){
        if(sendCommand(create_socket) == -1){
            continue;
        }
      }
      else if(command=="LIST"){
         if(listCommand(create_socket) == -1){
            continue;
        }
      }
      else if(command=="READ"){
         if(readCommand(create_socket) == -1){
            continue;
        }
      }
      else if(command=="DEL"){
         if(delCommand(create_socket) == -1){
            continue;
        }
      }
      else if(command=="LOGIN"){
         if(loginCommand(create_socket) ==-1){
            continue;
         }
      }
      else if(command=="QUIT"){
         isQuit = 1;
         if ((send(create_socket, "QUIT", 4, 0)) == -1) 
            {
               perror("send error");
               return -1;
            }
      } else {
         cout << "No valid command!" << endl;
         continue;
      }
         if(!isQuit){

            while(true){
               size = recv(create_socket, buffer, BUF - 1, 0);

            if (size == -1)
            {
               perror("recv error");
               break;
            }
            else if (size == 0)
            {
               printf("Server closed remote socket\n"); // ignore error
               break;
            }
            else
            {
               buffer[size] = '\0';
               printf("%s\n", buffer);

               //Buffer doesnt receive line by line: check if OK or ERR is contained anywhere in there.
               char *output = NULL;
               output = strstr (buffer,"<< OK");
               if(output) {
                  memset(buffer, 0, BUF);
                  break;
               }
               output = strstr (buffer,"<< ERR");
               if(output) {
                  memset(buffer, 0, BUF);
                  break;
               }
               output = strstr (buffer, "<< LOGIN FIRST");
               if(output) {
                  memset(buffer, 0, BUF);
                  break;
               }
            }
            }
         }
   } while (!isQuit);

   ////////////////////////////////////////////////////////////////////////////
   // CLOSES THE DESCRIPTOR
   if (create_socket != -1)
   {
      if (shutdown(create_socket, SHUT_RDWR) == -1)
      {
         // invalid in case the server is gone already
         perror("shutdown create_socket"); 
      }
      if (close(create_socket) == -1)
      {
         perror("close create_socket");
      }
      create_socket = -1;
   }

   return EXIT_SUCCESS;
}
