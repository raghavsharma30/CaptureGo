CC = g++
CFLAGS = -std=c++17 -Wall -I/usr/include/SDL2 -Iinclude
LDFLAGS = -lSDL2 -lSDL2_ttf -pthread

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

SERVER_SRC = $(SRC_DIR)/server.cpp $(SRC_DIR)/game.cpp $(SRC_DIR)/network.cpp $(SRC_DIR)/server_main.cpp
CLIENT_SRC = $(SRC_DIR)/client.cpp $(SRC_DIR)/game.cpp $(SRC_DIR)/network.cpp $(SRC_DIR)/client_main.cpp \
             $(SRC_DIR)/imgui.cpp $(SRC_DIR)/imgui_draw.cpp $(SRC_DIR)/imgui_widgets.cpp $(SRC_DIR)/imgui_tables.cpp \
             $(SRC_DIR)/imgui_impl_sdl2.cpp $(SRC_DIR)/imgui_impl_sdlrenderer2.cpp

SERVER_OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SERVER_SRC))
CLIENT_OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CLIENT_SRC))

all: $(BUILD_DIR)/server $(BUILD_DIR)/client

$(BUILD_DIR)/server: $(SERVER_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/client: $(CLIENT_OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
