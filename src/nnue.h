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
    static void compute_activation(std::array<int8_t, IN>& input, std::array<std::array<int8_t, IN>, OUT>& weights, std::array<int8_t, OUT>& biases,
        std::array<int8_t, OUT>& output) {
            std::array<int32_t, OUT>& temp_out;
            for (size_t i = 0; i < OUT; ++i) {
                int8_t a = 0;
                __m128i acc = _mm_setzero_si128();
                auto weight_vec = weights[i];
                for (size_t j = 0; j < IN; j += 32) {
                    __m128i i1 = _mm_load_si128((__m128i *) &in[j]);
                    __m128i w1 = _mm_load_si128((__m128i *) &weight_vec[j]);
                    __m128i p1 = _mm_maddubs_epi16(i1, p1); 
                    acc = _mm_madd_epi16(acc, p1);
                    
                    __m128i i2 = _mm_load_si128((__m128i *) &in[j+16]);
                    __m128i w2 = _mm_load_si128((__m128i *) &weight_vec[j+16]);
                    __m128i p2 = _mm_maddubs_epi16(i2, p2); 

                    acc = _mm_madd_epi16(acc, p2);
                }
                __m128i hi64  = _mm_shuffle_epi32(acc, _MM_SHUFFLE(1, 0, 3, 2));
                __m128i sum64 = _mm_add_epi32(hi64, x);
                __m128i hi32  = _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2));    // Swap the low two elements
                __m128i sum32 = _mm_add_epi32(sum64, hi32);
                int dot = _mm_cvtsi128_si32(sum32);       
                temp_out[i] = dot;
            }

            for (size_t i = 0; i < OUT; i += 16) {
                __m128i a1 = _mm_load_si128(temp_out[i]);
                __m128i a2 = _mm_load_si128(temp_out[i+4]);
                __m128i a3 = _mm_load_si128(temp_out[i+8]);
                __m128i a4 = _mm_load_si128(temp_out[i+12]);
                __m128i b1 = _mm_packs_epi32(a1, a2);
                __m128i b2 = _mm_packs_epi32(a3, a4);

                __m128i c = _mm_packs_epi16(b1, b2);
                __m128i relu = _mm_max_epi8(c, _mm_setzero_si128);
                _mm_store_si128(&output[i]);
            }
            //for (auto &n: temp_out) {
            //    std::cout << n << std::endl;
            //}
        } 
public:
};
    
