#ifndef SERVER_CLASS_H
#define SERVER_CLASS_H

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 50
#define SENDER "if23b280"

#include "serverHeaders.h"


class Server
{
  private:
    struct MailSpoolDir
    {
      std::string name;
      std::filesystem::path path;
    };
    MailSpoolDir mailSpoolDir;

    sockaddr_in serverAddress;
    std::vector<std::thread> activeThreads;
    std::mutex threadsMutex;

    bool abortRequested = false;
    int serverSocket = -1;
    int reuseValue = 1;
    bool isLoggedIn = false;
    std::string username;

    std::vector<std::string> blackList;
    int loginAttempts = 0;
    const int maxLoginAttempts = 3;
    const int blackListDuration = 60;

    std::string currentUser ="";
    const char *ldapServer = "ldap://ldap.technikum-wien.at:389";
    const char *ldapBind = "ou=people,dc=technikum-wien,dc=at";

  public:
    Server(int port, std::string mailSpoolName);
    bool start();
    void shutDown();

    void acceptClients();
    void cleanUpThreads();

    void clientHandler(int clientSocket);
    std::string parser(int clientSocket);
    
    bool loginHandler(int clientSocket);
    bool establishLDAPConnection(const std::string& username, const std::string& password);
    void checkLoginAttempts();
    bool isBlackListed(std::string ip);

    void commandHandler(int clientSocket, std::string command);
    void sendHandler(int clientSocket);
    std::vector<std::string> sendParser(int clientSocket);
    void listHandler(int clientSocket);
    void readHandler(int clientSocket);
    void delHandler(int clientSocket);

    void sendResponse(int clientSocket, bool state);
};

#endif // SERVER_CLASS_H