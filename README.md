# Capture Go

A networked, multiplayer implementation of the Capture Go game in C++. Players can join as human or AI, connect via a server, and play on a 7x7 board.

## Features
- Server-client architecture for multiplayer gameplay.
- Human and AI player options.
- Simple text-based interface.
- Move input as `x,y` (e.g., `4,4`).

## Requirements
- C++ compiler (e.g., g++)
- POSIX-compliant system (e.g., Linux, macOS)

## Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/raghavsharma30/CaptureGo.git
   cd CaptureGo
2. Compile and Run (Server-Client): 
  ```bash
   make clean 
   make all
   ./server
   ./client

