#ifndef CLIENT_CLASS_H
#define CLIENT_CLASS_H

#include "allHeaders.h"

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
    void closeConnectionSignal();
    bool connectToServer();
    void receiveData();
    bool send_command(const std::string &command);
    void closeConnection();
    int get_socket_fd() const;
    char* get_buffer();
    int specificMessage(int socket);
    int getch();
    std::string getpass();
    int commandHandler(int socket);
    int loginCommand(int socket);
    int sendCommand(int socket);
    int listCommand(int socket);
    int readCommand(int socket);
    int delCommand(int socket);
};



#endif // CLIENT_CLASS_H