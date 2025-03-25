# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -Wall -I/usr/include/SDL2 -Iinclude
LDFLAGS = -lSDL2 -lSDL2_ttf -pthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# ImGui source files
IMGUI_FILES = $(SRC_DIR)/imgui.cpp $(SRC_DIR)/imgui_draw.cpp $(SRC_DIR)/imgui_widgets.cpp \
              $(SRC_DIR)/imgui_tables.cpp $(SRC_DIR)/imgui_impl_sdl2.cpp $(SRC_DIR)/imgui_impl_sdlrenderer2.cpp

# Targets
all: $(BUILD_DIR)/server $(BUILD_DIR)/client

# Server target
$(BUILD_DIR)/server: $(BUILD_DIR)/server.o $(BUILD_DIR)/game.o $(BUILD_DIR)/network.o $(BUILD_DIR)/server_main.o
	$(CC) $^ -o $@ $(LDFLAGS)

# Client target
$(BUILD_DIR)/client: $(BUILD_DIR)/client.o $(BUILD_DIR)/game.o $(BUILD_DIR)/network.o $(BUILD_DIR)/client_main.o $(IMGUI_FILES:%.cpp=$(BUILD_DIR)/%.o)
	$(CC) $^ -o $@ $(LDFLAGS)

# Object file rules
$(BUILD_DIR)/server.o: $(SRC_DIR)/server.cpp $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/game.h $(INCLUDE_DIR)/network.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/server_main.o: $(SRC_DIR)/server_main.cpp $(INCLUDE_DIR)/server.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/client.o: $(SRC_DIR)/client.cpp $(INCLUDE_DIR)/client.h $(INCLUDE_DIR)/game.h $(INCLUDE_DIR)/network.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/client_main.o: $(SRC_DIR)/client_main.cpp $(INCLUDE_DIR)/client.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/game.o: $(SRC_DIR)/game.cpp $(INCLUDE_DIR)/game.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/network.o: $(SRC_DIR)/network.cpp $(INCLUDE_DIR)/network.h
	$(CC) $(CFLAGS) -c $< -o $@

# ImGui object files
$(BUILD_DIR)/imgui.o: $(SRC_DIR)/imgui.cpp $(INCLUDE_DIR)/imgui.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui_draw.o: $(SRC_DIR)/imgui_draw.cpp $(INCLUDE_DIR)/imgui.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui_widgets.o: $(SRC_DIR)/imgui_widgets.cpp $(INCLUDE_DIR)/imgui.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui_tables.o: $(SRC_DIR)/imgui_tables.cpp $(INCLUDE_DIR)/imgui.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui_impl_sdl2.o: $(SRC_DIR)/imgui_impl_sdl2.cpp $(INCLUDE_DIR)/imgui_impl_sdl2.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui_impl_sdlrenderer2.o: $(SRC_DIR)/imgui_impl_sdlrenderer2.cpp $(INCLUDE_DIR)/imgui_impl_sdlrenderer2.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/server $(BUILD_DIR)/client

.PHONY: all clean
