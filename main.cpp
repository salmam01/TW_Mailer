#include <iostream>
#include <server.cpp>
#include <client.cpp>

using namespace std;

int main(int argc, char *argv[])
{
  //  Check for missing arguments
  if(argc != 3)
  {
    cerr << "Invalid Input!" << endl;
    return -1;
  }

  Server server(stoi(argv[1]), argv[2]);
  server.start();

  return 0;
}