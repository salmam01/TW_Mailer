#include "serverHeaders.h"
#include "serverClass.cpp"

#define BUFFER_SIZE 1024
#define MAIL_SPOOL_DIR "/mail_spool"

using namespace std;
using namespace std::filesystem;

class Server
{
  private:
    int port;
    string mailSpoolDir;
    int serverSocket = -1;
    int reuseValue = 1;
    int abortRequested = 0;
    const int maxLoginAttempts = 3;
    int loginAttempts = 0;
    string currentUser ="";
    const char *ldapServer = "ldap://ldap.technikum-wien.at:389";
    const char *ldapBind = "ou=people,dc=technikum-wien,dc=at";

  public:
    Server(int port, string mailSpoolDir);
    bool start();
    bool acceptClients();
    bool clientHandler(int clientSocket);
    string parser(int clientSocket);
    void commandHandler(int clientSocket, string command);
    bool loginHandler(int clientSocket);
    bool sendHandler(int clientSocket);
    vector<string> sendParser(int clientSocket);
    void listHandler(int clientSocket);
    void readHandler(int clientSocket);
    void delHandler(int clientSocket);
    void sendResponse(int clientSocket, bool state);

}