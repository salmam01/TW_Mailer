CXX = g++
CXXFLAGS = -Wall #for Warnings

# Server conf
SERVER_SRCS = server.cpp serverClass.cpp
SERVER_HEADERS = serverHeaders.h serverClass.h
SERVER_OUTPUT = server
SERVER_LDFLAGS = -lldap -llber

# Client conf
CLIENT_SRCS = client.cpp clientClass.cpp
CLIENT_HEADERS = clientHeaders.h clientClass.h
CLIENT_OUTPUT = client

all: $(SERVER_OUTPUT) $(CLIENT_OUTPUT)

#server bin
$(SERVER_OUTPUT): $(SERVER_SRCS) $(SERVER_HEADERS)
	$(CXX) $(CXXFLAGS) $(SERVER_SRCS) -o $(SERVER_OUTPUT) $(SERVER_LDFLAGS)

#client bin
$(CLIENT_OUTPUT): $(CLIENT_SRCS) $(CLIENT_HEADERS)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRCS) -o $(CLIENT_OUTPUT)