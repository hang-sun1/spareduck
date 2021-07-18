# spareduck

A chess engine using webassembly

## Building with Docker

To build the project in an isolated environment, use the Docker config.

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
