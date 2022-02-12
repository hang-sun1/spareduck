# spareduck

A chess engine that runs in the browser using webassembly. Searches via alpha-beta pruning and evaluates positions using an efficiently updateable neural network (NNUE). 

## Building with Docker

To build the project in an isolated environment, use the Docker config.

#### `docker compose`:

By using docker compose we can avoid having to recompiling after changing the main JS files.

For the first time and **whenever you change the c files** you have to run:

> docker-compose up --build -d

Otherwise you should be able to run:

> docker-compose up -d

#### `docker run`:

First build the Docker image. The project is compiled during this step.

> docker build --file Dockerfile --tag spareduck:latest .

Then, run the Docker image and expose the port to http://localhost:8080.

> docker run -d -p 8080:8080 spareduck:latest

Unit tests can be run with the following.

> docker run spareduck:latest bash -c "cd .. && bash test.sh"

## Chess programming resources

- [Chess Programming Wiki](https://www.chessprogramming.org/Getting_Started)
- [Bitboards](https://en.wikipedia.org/wiki/Bitboard)
- [Search Functions](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning)

## VS Code styling settings

`C_Cpp.clang_format_fallbackStyle": "{ BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 0}`
