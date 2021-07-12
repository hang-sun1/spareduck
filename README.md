# spareduck

A chess engine using webassembly

## Building the project

In order to compile to webassembly, emscripten is required. It be can downloaded [here](https://emscripten.org/docs/getting_started/downloads.html)  
After the emscripten sdk is installed, C++ can be compiled into webassembly by running the `build.sh` script  
A test server with automatic reloading can be launched by entering the the `www` directory and running `npm run start`

## Building with Docker

To build the project in an isolated environment, use the Docker config.

First build the Docker image. The project is compiled during this step.

> docker build --file Dockerfile --tag spareduck:latest .

Then, run the Docker image and expose the port to http://localhost:8080.

> docker run -d -p 8080:8080 spareduck:latest

## Chess programming resources

- [Chess Programming Wiki](https://www.chessprogramming.org/Getting_Started)
- [Bitboards](https://en.wikipedia.org/wiki/Bitboard)
- [Search Functions](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning)
