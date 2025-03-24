CXX = g++
CXXFLAGS = -std=c++11 -pthread

SERVER_OBJS = server_main.o server.o game.o network.o
CLIENT_OBJS = client_main.o client.o game.o network.o

all: server client

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJS)

server_main.o: server_main.cpp server.h
	$(CXX) $(CXXFLAGS) -c server_main.cpp

client_main.o: client_main.cpp client.h
	$(CXX) $(CXXFLAGS) -c client_main.cpp

server.o: server.cpp server.h game.h network.h
	$(CXX) $(CXXFLAGS) -c server.cpp

client.o: client.cpp client.h game.h network.h
	$(CXX) $(CXXFLAGS) -c client.cpp

game.o: game.cpp game.h
	$(CXX) $(CXXFLAGS) -c game.cpp

network.o: network.cpp network.h
	$(CXX) $(CXXFLAGS) -c network.cpp

clean:
	rm -f *.o server client

.PHONY: all clean
