#pragma once

#include <cstdint>
#include <array>
#include <immintrin.h>
// #include <wasm_simd128.h>

using std::int16_t;
using std::size_t;

// the number of input nodes (importantly this is for each side)
static const size_t INPUT_SIZE = 64 * 64 * 10;
static const size_t LAYER1_SIZE = 256;
static const size_t LAYER2_SIZE = 32;
static const size_t LAYER3_SIZE = 32;

class NNUE {
public:
    NNUE() {
            
    }

    int evaluate() {
        // efficient updating should have already updated a1_white and a1_black.
        // however the bias and relu are not yet applied to them in order to facilitate
        // efficient updates
        
        // 1. apply the bias to the neurons of the first layer
        // 2. apply ReLu to the neurons of the first layer


        // 3. construct a vector that is LAYER1_SIZE * 2 long containing the activations
        // of the combined black and white first layer (with the side to move at the start)


        // 4. go through the rest of the hidden layers
        
        // 5. calculate the output layer and return;
    }
private:
    // std::array<int16_t, INPUT_SIZE * LAYER1_SIZE> w0_white;
    //alignas(16) std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0_white;
    //alignas(16) std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0_black;
    alignas(16) std::array<std::array<int16_t, LAYER1_SIZE>, INPUT_SIZE> w0_white;
    alignas(16) std::array<std::array<int16_t, LAYER1_SIZE>, INPUT_SIZE> w0_black;

    // std::array<int16_t, INPUT_SIZE * LAYER1_SIZE> w0_black;
    alignas(16) std::array<int16_t, LAYER1_SIZE> b1_white;
    alignas(16) std::array<int16_t, LAYER1_SIZE> b1_black;
    alignas(16) std::array<int8_t, LAYER1_SIZE> a1_white;
    alignas(16) std::array<int8_t, LAYER1_SIZE> a1_black;
    // std::array<int8_t, LAYER1_SIZE * LAYER2_SIZE> w1;
    alignas(16) std::array<std::array<int16_t, LAYER1_SIZE * 2>, LAYER2_SIZE> w1;
    alignas(16) std::array<int8_t, LAYER2_SIZE> b2;
    alignas(16) std::array<int8_t, LAYER2_SIZE> a2;
    // std::array<int8_t, LAYER2_SIZE * LAYER3_SIZE> w2;
    alignas(16) std::array<std::array<int16_t, LAYER2_SIZE>, LAYER3_SIZE> w2;
    alignas(16) std::array<int8_t, LAYER3_SIZE> b3;
    alignas(16) std::array<int8_t, LAYER3_SIZE> a3;
    alignas(16) std::array<std::array<uint8_t, INPUT_SIZE>, 2> halfkp;

    void efficiently_update() {
        // the dimensions of the first weights are flipped so when a piece is moved
        // you can just do essentially layer1 - w0(moved_from) + w0(moved_to)
    }

    // compute the activations of layers beyond the first (the first is a special case)
    template<size_t IN, size_t OUT>
    static void compute_activation(std::array<int8_t, IN>& input, std::array<std::array<int8_t, IN>, OUT>& weights, std::array<int8_t, OUT>& biases,
        std::array<int8_t, OUT>& output) {
            std::array<int32_t, OUT> temp_out;
            for (size_t i = 0; i < OUT; ++i) {
                int8_t a = 0;
                __m128i acc = _mm_setzero_si128();
                auto weight_vec = weights[i];
                for (size_t j = 0; j < IN; j += 32) {
                    __m128i i1 = _mm_load_si128((__m128i *) &input[j]);
                    __m128i w1 = _mm_load_si128((__m128i *) &weight_vec[j]);
                    __m128i p1 = _mm_maddubs_epi16(i1, p1); 
                    acc = _mm_madd_epi16(acc, p1);
                    
                    __m128i i2 = _mm_load_si128((__m128i *) &input[j+16]);
                    __m128i w2 = _mm_load_si128((__m128i *) &weight_vec[j+16]);
                    __m128i p2 = _mm_maddubs_epi16(i2, p2); 

                    acc = _mm_madd_epi16(acc, p2);
                }
                __m128i hi64  = _mm_shuffle_epi32(acc, _MM_SHUFFLE(1, 0, 3, 2));
                __m128i sum64 = _mm_add_epi32(hi64, acc);
                __m128i hi32  = _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2));    // Swap the low two elements
                __m128i sum32 = _mm_add_epi32(sum64, hi32);
                int dot = _mm_cvtsi128_si32(sum32);       
                temp_out[i] = dot;
            }

            for (size_t i = 0; i < OUT; i += 16) {
                __m128i a1 = _mm_load_si128((__m128i *)&temp_out[i]);
                __m128i a2 = _mm_load_si128((__m128i *) &temp_out[i+4]);
                __m128i a3 = _mm_load_si128((__m128i *) &temp_out[i+8]);
                __m128i a4 = _mm_load_si128((__m128i *) &temp_out[i+12]);
                __m128i b1 = _mm_packs_epi32(a1, a2);
                __m128i b2 = _mm_packs_epi32(a3, a4);

                // add biases here
                __m128i act1 = _mm_load_si128((__m128i *) &biases[i]);
                __m128i act2 = _mm_load_si128((__m128i *) &biases[i+8]);
                b1 = _mm_add_epi16(b1, act1);
                b2 = _mm_add_epi16(b2, act2);
                // apply relu here
                b1 = _mm_max_epi16(b1, _mm_setzero_si128());
                b2 = _mm_max_epi16(b2, _mm_setzero_si128());
                __m128i c = _mm_packs_epi16(b1, b2);
                _mm_store_si128(&output[i], c);
            }
        } 
};
    
