#pragma once

#include <cstdint>
#include <array>
#include <wasm_simd128.h>

using std::int16_t;
using std::size_t;

// the number of input nodes (importantly this is for each side)
static const size_t INPUT_SIZE = 64 * 64 * 10;
static const size_t LAYER1_SIZE = 256;
static const size_t LAYER2_SIZE = 32;
static const size_t LAYER3_SIZE = 32;

class NNUE {
private:
    // std::array<int16_t, INPUT_SIZE * LAYER1_SIZE> w0_white;
    std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0_white;
    std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0_black;
    // std::array<int16_t, INPUT_SIZE * LAYER1_SIZE> w0_black;
    std::array<int16_t, LAYER1_SIZE> b1_white;
    std::array<int16_t, LAYER1_SIZE> b1_black;
    std::array<int8_t, LAYER1_SIZE> a1_white;
    std::array<int8_t, LAYER1_SIZE> a1_black;
    // std::array<int8_t, LAYER1_SIZE * LAYER2_SIZE> w1;
    std::array<std::array<int16_t, LAYER1_SIZE>, LAYER2_SIZE> w1;
    std::array<int8_t, LAYER2_SIZE> b2;
    std::array<int8_t, LAYER2_SIZE> a2;
    // std::array<int8_t, LAYER2_SIZE * LAYER3_SIZE> w2;
    std::array<std::array<int16_t, LAYER2_SIZE>, LAYER3_SIZE> w2;
    std::array<int8_t, LAYER3_SIZE> b3;
    std::array<int8_t, LAYER3_SIZE> a3;

    // the activation function used by the neural net 
    int16_t activation_func(int16_t n);

    template<size_t IN, size_t OUT>
    void compute_activation(std::array<int8_t, IN>& input, std::array<std::array<int8_t, OUT>, IN>& weights, std::array<int8_t, OUT>& biases,
        std::array<int8_t, OUT>& output) {
            for (size_t i = 0; i < OUT; ++i) {
                int8_t a = 0;
                auto weight_vec = weights[i];
                for (size_t j = 0; j < OUT; j += 16) {
                    v128_t in = wasm_v128_load(&input);
                    v128_t w = wasm_v128_load(&weight_vec);
                    v128_t dot = wasm_i8x16_mul(in, w);
                }
                output[i] = activation_func(a);
            }
        } 
public:
};
    
