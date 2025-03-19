# Compiler to use
CC = g++

# Compiler flags
CFLAGS = -std=c++11 -Wall

# Linker flags (for pthread)
LDFLAGS = -pthread

# Target executables
SERVER_TARGET = server
CLIENT_TARGET = client

# Object files
SERVER_OBJS = server_main.o server.o game.o network.o
CLIENT_OBJS = client_main.o client.o game.o network.o

# Header files (dependencies)
HEADERS = client.h server.h game.h network.h

# Default target: build both server and client
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Link server executable
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVER_TARGET) $(LDFLAGS)

# Link client executable
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o $(CLIENT_TARGET) $(LDFLAGS)

# Compile source files to object files
client_main.o: client_main.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c client_main.cpp

client.o: client.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c client.cpp

server_main.o: server_main.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c server_main.cpp

server.o: server.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c server.cpp

game.o: game.cpp game.h
	$(CC) $(CFLAGS) -c game.cpp

network.o: network.cpp network.h
	$(CC) $(CFLAGS) -c network.cpp

# Clean up
clean:
	rm -f *.o $(SERVER_TARGET) $(CLIENT_TARGET)

# Phony targets
.PHONY: all clean
