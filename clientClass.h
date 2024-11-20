#ifndef CLIENT_CLASS_H
#define CLIENT_CLASS_H

#include "clientHeaders.h"

#define BUFFER_SIZE 1024

class Client
{
    private:
    int socket_fd;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    public:
    bool isLoggedIn = false;
    bool isQuit = false;
    Client(const char *ip, int port);
    bool connect_to_server();
    void receive_data();
    bool send_command(const std::string &command);
    void close_connection();
    int get_socket_fd() const;
    char* get_buffer();
    int specificMessage(int socket);
    int getch();
    std::string getpass();
    int loginCommand(int socket);
    int sendCommand(int socket);
    int listCommand(int socket);
    int readCommand(int socket);
    int delCommand(int socket);


};



#endif // CLIENT_CLASS_H