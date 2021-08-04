#pragma once

#include <cstdint>
#include <array>
#include <immintrin.h>
#include <optional>

#include "move.hpp"
#include "board.hpp"

using std::int16_t;
using std::size_t;


// the number of input nodes (importantly this is for each side)
static const size_t INPUT_SIZE = 64 * 64 * 10;
static const size_t LAYER1_SIZE = 256;
static const size_t LAYER2_SIZE = 32;
static const size_t LAYER3_SIZE = 32;

enum PieceType: size_t {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    QUEEN = 3,
    KING = 4,
    ROOK = 5,
};

class NNUE {
public:
    NNUE(Side current_to_move);

    int evaluate();
    void update_non_king_move(Move move, PieceType moved_piece, std::optional<PieceType> captured);
    void update_king_move(Move move, std::optional<PieceType> captured);
private:
    // std::array<int16_t, INPUT_SIZE * LAYER1_SIZE> w0_white;
    alignas(16) std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0;
    //alignas(16) std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0_white;
    //alignas(16) std::array<std::array<int16_t, INPUT_SIZE>, LAYER1_SIZE> w0_black;
    //alignas(16) std::array<std::array<int16_t, LAYER1_SIZE>, INPUT_SIZE> w0_white;
    //alignas(16) std::array<std::array<int16_t, LAYER1_SIZE>, INPUT_SIZE> w0_black;

    // std::array<int16_t, INPUT_SIZE * LAYER1_SIZE> w0_black;
    alignas(16) std::array<int16_t, LAYER1_SIZE> b1;
    // alignas(16) std::array<int16_t, LAYER1_SIZE> b1_white;
    // alignas(16) std::array<int16_t, LAYER1_SIZE> b1_black;
    alignas(16) std::array<int16_t, LAYER1_SIZE> a1_white;
    alignas(16) std::array<int8_t, LAYER1_SIZE> a1_white_with_bias;
    alignas(16) std::array<int16_t, LAYER1_SIZE> a1_black;
    alignas(16) std::array<int8_t, LAYER1_SIZE> a1_black_with_bias;
    // std::array<int8_t, LAYER1_SIZE * LAYER2_SIZE> w1;
    alignas(16) std::array<std::array<int8_t, LAYER1_SIZE * 2>, LAYER2_SIZE> w1;
    alignas(16) std::array<int8_t, LAYER2_SIZE> b2;
    alignas(16) std::array<int8_t, LAYER2_SIZE> a2;
    // std::array<int8_t, LAYER2_SIZE * LAYER3_SIZE> w2;
    alignas(16) std::array<std::array<int8_t, LAYER2_SIZE>, LAYER3_SIZE> w2;
    alignas(16) std::array<int8_t, LAYER3_SIZE> b3;
    alignas(16) std::array<int8_t, LAYER3_SIZE> a3;
    alignas(16) std::array<std::array<int8_t, LAYER3_SIZE>, 1> w3;
    std::array<int8_t, 1> b4;
    alignas(16) std::array<std::array<uint8_t, INPUT_SIZE>, 2> halfkp;

    // compute the activations of layers beyond the first (the first is a special case)
    // returns an int, but the result is only relevant when the output layer is being computed
    // otherwise it should be ignored
    template<size_t IN, size_t OUT>
    int compute_activation(std::array<int8_t, IN>& input, std::array<std::array<int8_t, IN>, OUT>& weights, std::array<int8_t, OUT>& biases,
        std::array<int8_t, OUT>& output) {
            std::array<int32_t, OUT> temp_out;
            for (size_t i = 0; i < OUT; ++i) {
                __m128i acc = _mm_setzero_si128();
                auto weight_vec = weights[i];
                for (size_t j = 0; j < IN; j += 32) {
                    __m128i i1 = _mm_load_si128((__m128i *) &input[j]);
                    __m128i w1 = _mm_load_si128((__m128i *) &weight_vec[j]);
                    __m128i p1 = _mm_maddubs_epi16(i1, w1); 
                    acc = _mm_madd_epi16(acc, p1);
                    
                    __m128i i2 = _mm_load_si128((__m128i *) &input[j+16]);
                    __m128i w2 = _mm_load_si128((__m128i *) &weight_vec[j+16]);
                    __m128i p2 = _mm_maddubs_epi16(i2, w2); 

                    acc = _mm_madd_epi16(acc, p2);
                }
                __m128i hi64  = _mm_shuffle_epi32(acc, _MM_SHUFFLE(1, 0, 3, 2));
                __m128i sum64 = _mm_add_epi32(hi64, acc);
                __m128i hi32  = _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2));    // Swap the low two elements
                __m128i sum32 = _mm_add_epi32(sum64, hi32);
                int dot = _mm_cvtsi128_si32(sum32);       
                temp_out[i] = dot;
            }
            // if this is computing the outer layer we want to just return the 32 bit dot product
            if (OUT == 1) {
                return temp_out[0];
            }
            // otherwise compress everything back into 8 bits
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
                _mm_store_si128((__m128i *) &output[i], c);
            }
            return 0;
        } 
};


    
