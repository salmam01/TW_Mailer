#include "allHeaders.h"
#include "clientClass.h"

using namespace std;

//  Global client pointer to be able to pass a function if a signal is called
Client * clientPtr = nullptr;

//  Function to print the usage of the client
void printUsage()
{
    cout << "** CLIENT USAGE **" << endl;
    cout << "./client <ip> <port>" << endl;
    cout << "<ip>: must be a NUMBER or LOCALHOST" << endl;
    cout << "<port>: must be a NUMBER" << endl;
}

//  Function that handles CTRL + C on client side
void signalHandler(int sig)
{
  if(sig == SIGINT || sig == SIGHUP)
  {
    if(clientPtr != nullptr)
    {
      cerr << "Shutdown requested by Client." << endl;
      clientPtr->closeConnectionSignal();
    }
    exit(0);
  }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        cerr << "Usage: ./client <ip> <port>"<<endl;
        return EXIT_FAILURE;
    }

    // Parse port argument
    int port;
    istringstream iss(argv[2]);
    if (!(iss >> port))
    {
        cerr << "Invalid port - not a number"<<endl;
        return EXIT_FAILURE;
    }

    Client client(argv[1], port);
    clientPtr = &client;

    if (!client.connectToServer())
    {
        return EXIT_FAILURE;
    }

    client.receiveData();  // Read initial data or welcome message
    
    if(signal(SIGINT, signalHandler) == SIG_ERR)
    {
        cerr << "Error registering signal." << endl;
    }
    if(signal(SIGHUP, signalHandler) == SIG_ERR)
    {
    cerr << "Error registering signal." << endl;
    }

    do
    {
        // Print the prompt to get user command
        cout << ">> ";
        string command;
        getline(cin, command);
        transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (!client.isLoggedIn)
        {
            // Handle commands allowed when not logged in
            if (command == "LOGIN")
            {
                if (client.loginCommand(client.get_socket_fd()) == -1)
                {
                    continue;
                }
                client.isLoggedIn = true; // Assume successful login sets this to true
                continue;
            }
            else if (command == "QUIT")
            {
                client.isQuit = true;
                if (!client.sendCommandToServer("QUIT"))
                {
                    continue;
                }
            }
            else
            {
                cout << "You need to login first!" << endl;
                continue;
            }
        }
        else
        {
            // Handle commands allowed when logged in
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
            else if (command == "QUIT")
            {
                client.isQuit = true;
                if (!client.sendCommandToServer("QUIT"))
                {
                    continue;
                }
            }
            else if (command == "LOGIN")
            {
                cout << "You are already logged in!" << endl;
                continue;
            }
            else
            {
                cout << "No valid command!" << endl;
                continue;
            }
        }

        // Process the server's response
        if (!client.isQuit)
        {
            int size = recv(client.get_socket_fd(), client.get_buffer(), BUFFER_SIZE - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                cerr << "Server closed remote socket" << endl;
                break;
            }
            else
            {
                client.get_buffer()[size] = '\0';
                cout << client.get_buffer() << endl;

                // Handle server response like << OK or << ERR
                if (strstr(client.get_buffer(), "<< OK") ||
                    strstr(client.get_buffer(), "<< ERR") ||
                    strstr(client.get_buffer(), "<< LOGIN FIRST"))
                {
                    memset(client.get_buffer(), 0, BUFFER_SIZE); // Clear buffer for next command
                }
            }
        }

    } while (!client.isQuit);

    client.closeConnection();

    return EXIT_SUCCESS;
}


