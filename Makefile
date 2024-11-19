CXX = g++

CXXFLAGS = -Wall

SRCS = server.cpp serverClass.cpp

HEADERS = serverHeaders.h serverClass.h

OUTPUT = server

LDFLAGS = -lldap -llber

$(OUTPUT): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(OUTPUT) $(LDFLAGS)

# um die kompilierte binary zu löschen, build from scratch falls library/header probleme/änderungen
clean:
	rm -f $(OUTPUT)