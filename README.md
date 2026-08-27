# RESP-CPP

A simple, non-blocking TCP server written in C++ using `epoll`.

## Overview
This project implements a lightweight event-driven TCP server. Currently, it functions as an echo server that reads incoming data from connected clients and immediately writes it back out.

## Project Structure
- `src/main.cpp`: Entry point of the server.
- `src/server.h` / `src/server.cpp`: Contains the `Server` class which manages client connections and the `epoll` event loop.
- `src/client.h`: Defines the `ClientState` structure for tracking read/write buffers of connected clients.
- `src/utils.h` / `src/utils.cpp`: Contains utility functions such as `set_nonblocking`.

## Building

A `Makefile` is provided to build the project using `g++` (requires C++17 support).

```bash
make
```

The executable will be located at `build/resp-server`.

## Running

```bash
./build/resp-server
```

The server will listen for TCP connections on port 6380.

## Usage

You can test the server using `netcat`:

```bash
nc localhost 6380
```


## Some links to in mind while building something like this 

1. beej guide for network programming to understand what each thing does if you are new to socket programming in c++ or c 
link -> https://beej.us/guide/bgnet/html/split-wide/index.html
