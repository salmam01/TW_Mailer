#include <iostream>
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
  
  return 0;
}