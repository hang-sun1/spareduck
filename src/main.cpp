#include <iostream>
#include <emscripten.h>

extern "C" {
EMSCRIPTEN_KEEPALIVE 
int add_two(int a, int b) { 
    return a+b;
}
}


extern "C" {
EMSCRIPTEN_KEEPALIVE
int main() {
    std::cout << "hello" << std::endl;
}
}