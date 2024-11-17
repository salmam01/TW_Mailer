#ifndef SERVER_CLASS_H
#define SERVER_CLASS_H

#define BUFFER_SIZE 1024
//#define MAIL_SPOOL_DIR "/mail_spool" --> not sure if we need it

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
    int serverSocket = -1;
    int reuseValue = 1;
    bool abortRequested = false;
    const int maxLoginAttempts = 3;
    int loginAttempts = 0;
    std::string currentUser ="";
    const char *ldapServer = "ldap://ldap.technikum-wien.at:389";
    const char *ldapBind = "ou=people,dc=technikum-wien,dc=at";
    sockaddr_in serverAddress;

  public:
    Server(int port, std::string mailSpoolName);
    bool start();
    void acceptClients();
    void clientHandler(int clientSocket);
    std::string parser(int clientSocket);
    void commandHandler(int clientSocket, std::string command);
    bool loginHandler(int clientSocket);
    void sendHandler(int clientSocket);
    std::vector<std::string> sendParser(int clientSocket);
    void listHandler(int clientSocket);
    void readHandler(int clientSocket);
    void delHandler(int clientSocket);
    void sendResponse(int clientSocket, bool state);
    bool establishLDAPConnection(const std::string& bindPassword);
};

#endif // SERVER_CLASS_H