#include "clientHeaders.h"
#include "clientClass.cpp"

using namespace std;

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

    client.receive_data();  // Read initial data or welcome message

    bool isQuit = false;
    do
    {
        // Print the prompt to get user command
        printf(">> ");
        string command;
        getline(cin, command);
        transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "SEND")
        {
            if (client.sendCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "LIST")
        {
            if (client.listCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "READ")
        {
            if (client.readCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "DEL")
        {
            if (client.delCommand(client.get_socket_fd()) == -1)
            {
                continue;
            }
        }
        else if (command == "LOGIN")
        {
            if (client.loginCommand(client.get_socket_fd()) == -1)
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

        // Process the server's response
        if (!isQuit)
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

                // Handle server response like << OK or << ERR
                if (strstr(client.get_buffer(), "<< OK") ||
                    strstr(client.get_buffer(), "<< ERR") ||
                    strstr(client.get_buffer(), "<< LOGIN FIRST"))
                {
                    memset(client.get_buffer(), 0, BUFFER_SIZE);  // Clear buffer for next command
                }
            }
        }

    } while (!isQuit);

    client.close_connection();

    return EXIT_SUCCESS;
}


