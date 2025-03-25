# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -Wall
LDFLAGS = -lSDL2 -lSDL2_ttf -pthread

# ImGui source files
IMGUI_FILES = imgui.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp imgui_impl_sdl2.cpp imgui_impl_sdlrenderer2.cpp

# Targets
all: server client

# Server target
server: server.o game.o network.o server_main.o
	$(CC) server.o game.o network.o server_main.o -o server $(LDFLAGS)

# Client target
client: client.o game.o network.o client_main.o $(IMGUI_FILES)
	$(CC) client.o game.o network.o client_main.o $(IMGUI_FILES) -o client $(LDFLAGS)

# Object file rules
server.o: server.cpp server.h game.h network.h
	$(CC) $(CFLAGS) -c server.cpp

server_main.o: server_main.cpp server.h
	$(CC) $(CFLAGS) -c server_main.cpp

client.o: client.cpp client.h game.h network.h
	$(CC) $(CFLAGS) -c client.cpp

client_main.o: client_main.cpp client.h
	$(CC) $(CFLAGS) -c client_main.cpp

game.o: game.cpp game.h
	$(CC) $(CFLAGS) -c game.cpp

network.o: network.cpp network.h
	$(CC) $(CFLAGS) -c network.cpp

# ImGui object files (assuming these files are in the directory)
imgui.o: imgui.cpp imgui.h
	$(CC) $(CFLAGS) -c imgui.cpp

imgui_draw.o: imgui_draw.cpp imgui.h
	$(CC) $(CFLAGS) -c imgui_draw.cpp

imgui_widgets.o: imgui_widgets.cpp imgui.h
	$(CC) $(CFLAGS) -c imgui_widgets.cpp

imgui_tables.o: imgui_tables.cpp imgui.h
	$(CC) $(CFLAGS) -c imgui_tables.cpp

imgui_impl_sdl2.o: imgui_impl_sdl2.cpp imgui_impl_sdl2.h
	$(CC) $(CFLAGS) -c imgui_impl_sdl2.cpp

imgui_impl_sdlrenderer2.o: imgui_impl_sdlrenderer2.cpp imgui_impl_sdlrenderer2.h
	$(CC) $(CFLAGS) -c imgui_impl_sdlrenderer2.cpp

# Clean up
clean:
	rm -f *.o server client

.PHONY: all clean
