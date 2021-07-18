#include <iostream>
#ifndef TESTING
#include <emscripten.h>
#endif

extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int add_two(int a, int b) { return a + b; }
}

extern "C" {
#ifndef TESTING
EMSCRIPTEN_KEEPALIVE
#endif
int main() { std::cout << "hello" << std::endl; }
}
