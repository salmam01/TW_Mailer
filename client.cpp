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
#include <algorithm> // for transform
#include <ctype.h>   // for toupper
#include "client_functions.h"

#define BUFFER_SIZE 1024

using namespace std;

class Client
{
private:
    int socket_fd;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

public:
    Client(const char *ip, int port)
    {
        // Initialize socket
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1)
        {
            perror("Socket error");
            exit(EXIT_FAILURE);
        }

        // Initialize address structure
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        inet_aton(ip, &server_address.sin_addr);
    }

    bool connect_to_server()
    {
        if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        {
            perror("Connect error - no server available");
            return false;
        }
        printf("Connection with server (%s) established\n", inet_ntoa(server_address.sin_addr));
        return true;
    }

    void receive_data()
    {
        int size = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (size == -1)
        {
            perror("recv error");
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

    bool send_command(const string &command)
    {
        if (send(socket_fd, command.c_str(), command.length(), 0) == -1)
        {
            perror("send error");
            return false;
        }
        return true;
    }

    void close_connection()
    {
        if (socket_fd != -1)
        {
            if (shutdown(socket_fd, SHUT_RDWR) == -1)
            {
                perror("shutdown socket");
            }
            if (close(socket_fd) == -1)
            {
                perror("close socket");
            }
            socket_fd = -1;
        }
    }

    int get_socket_fd() const
    {
        return socket_fd;
    }

    char *get_buffer()
    {
        return buffer;
    }
};

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: ./client <ip> <port>\n";
        return EXIT_FAILURE;
    }

    // Parse port argument
    int port;
    istringstream iss(argv[2]);
    if (!(iss >> port))
    {
        cerr << "Invalid port - not a number\n";
        return EXIT_FAILURE;
    }

    Client client(argv[1], port);

    if (!client.connect_to_server())
    {
        return EXIT_FAILURE;
    }

    client.receive_data();

    bool isQuit = false;
    do
    {
        printf(">> ");
        string command;
        getline(cin, command);
        transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "SEND")
        {
            if (sendCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "LIST")
        {
            if (listCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "READ")
        {
            if (readCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "DEL")
        {
            if (delCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "LOGIN")
        {
            if (loginCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "QUIT")
        {
            isQuit = true;
            if (!client.send_command("QUIT"))
            {
                continue;
            }
        }
        else
        {
            cout << "No valid command!" << endl;
            continue;
        }

        if (!isQuit)
        {
            while (true)
            {
                int size = recv(client.get_socket_fd(), client.get_buffer(), BUFFER_SIZE - 1, 0);
                if (size == -1)
                {
                    perror("recv error");
                    break;
                }
                else if (size == 0)
                {
                    printf("Server closed remote socket\n");
                    break;
                }
                else
                {
                    client.get_buffer()[size] = '\0';
                    printf("%s\n", client.get_buffer());

                    // Check for specific responses
                    if (strstr(client.get_buffer(), "<< OK") ||
                        strstr(client.get_buffer(), "<< ERR") ||
                        strstr(client.get_buffer(), "<< LOGIN FIRST"))
                    {
                        memset(client.get_buffer(), 0, BUFFER_SIZE);
                        break;
                    }
                }
            }
        }

    } while (!isQuit);

    client.close_connection();

    return EXIT_SUCCESS;
}
