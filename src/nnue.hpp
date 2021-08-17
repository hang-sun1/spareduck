#pragma once

#include <emmintrin.h>
#include <immintrin.h>

#include <array>
#include <cstdint>
#include <optional>
#include <utility>
#include <iostream>

#include "board.hpp"
#include "move.hpp"
#include "piece.hpp"

#ifndef TESTING
#include <emscripten/fetch.h>
#endif

using std::int16_t;
using std::size_t;

// the number of input nodes (importantly this is for each side)
static const size_t INPUT_SIZE = 49216;
static const size_t LAYER1_SIZE = 512;
static const size_t LAYER2_SIZE = 16;
static const size_t LAYER3_SIZE = 32;


class NNUE {
   public:
    NNUE();
    NNUE(Side current_to_move, emscripten_fetch_t* data);

    int evaluate(size_t piece_count, Side side_to_move);
    void update_non_king_move(Move move, Piece moved_piece, std::optional<Piece> captured, std::optional<Piece> promoted, uint8_t white_king_square,
        uint8_t black_king_square, Side side_that_moved, bool reverse_move);
    void reset_nnue(Move move, std::optional<Piece> captured, uint8_t white_king_square, uint8_t black_king_square, Side side_that_moved, Board &b);
    bool ready = false;
   private:
    std::unique_ptr<int16_t[]> w0;
    std::unique_ptr<int32_t[]> psqt_wts;
    std::unique_ptr<int32_t[]> wps;
    std::unique_ptr<int32_t[]> bps;
    std::unique_ptr<int32_t[]> ps;
    std::unique_ptr<int16_t[]> b1;
    std::unique_ptr<int16_t[]> a1_white;
    std::unique_ptr<int8_t[]> a1_white_with_bias;
    std::unique_ptr<int16_t[]> a1_black;
    std::unique_ptr<int8_t[]> a1_black_with_bias;
    std::unique_ptr<int8_t[]> w1;
    std::unique_ptr<int32_t[]> b2;
    std::unique_ptr<int8_t[]> a2;
    std::unique_ptr<int8_t[]> w2;
    std::unique_ptr<int32_t[]> b3;
    std::unique_ptr<int8_t[]> a3;
    std::unique_ptr<int8_t[]> w3;
    std::unique_ptr<int32_t[]> b4;

    size_t halfka_index(bool is_white_pov, uint8_t king_square, uint8_t square, Piece piece, Side side_of_piece);

    void update_first_layer_add(size_t white_pov_index, size_t black_pov_index);
    void update_first_layer_sub(size_t white_pov_index, size_t black_pov_index);

    // compute the activations of layers beyond the first (the first is a special case)
    // returns an int, but the result is only relevant when the output layer is being computed
    // otherwise it should be ignored
    template <size_t IN, size_t OUT>
    int compute_activation(int8_t* input, int8_t* weights, int32_t* biases,
                           int8_t* output, size_t piece_count) {
        size_t bucket = (piece_count - 1) / 4;
        std::array<int32_t, OUT> temp_out;
        for (size_t i = 0; i < OUT; ++i) {
            __m128i acc = _mm_setzero_si128();
            for (size_t j = 0; j < IN; j += 16) {
                __m128i i1 = _mm_load_si128((__m128i *)&input[j]);
                __m128i w1 = _mm_load_si128((__m128i *)&weights[IN*(bucket*OUT)+j+i*OUT]);
                __m128i p1 = _mm_maddubs_epi16(i1, w1);
                acc = _mm_adds_epi16(acc, p1);

                // __m128i i2 = _mm_load_si128((__m128i *)&input[j + 16]);
                // __m128i w2 = _mm_load_si128((__m128i *)&weight_vec[j + 16]);
                // __m128i p2 = _mm_maddubs_epi16(i2, w2);

                // acc = _mm_madd_epi16(acc, p2);
            }
            acc = _mm_madd_epi16(acc, _mm_set1_epi16(1));
            __m128i hi64 = _mm_shuffle_epi32(acc, _MM_SHUFFLE(1, 0, 3, 2));
            __m128i sum64 = _mm_add_epi32(hi64, acc);
            __m128i hi32 = _mm_shufflelo_epi16(sum64, _MM_SHUFFLE(1, 0, 3, 2));  // Swap the low two elements
            __m128i sum32 = _mm_add_epi32(sum64, hi32);
            int dot = _mm_cvtsi128_si32(sum32);
            temp_out[i] = dot;
            // std::cout << dot << std::endl;
        }
        // if this is computing the output layer we want to just return the 32 bit dot product
        // plus the bias
        if (OUT == 1) {
            return temp_out[0] + biases[bucket];
        }
        // otherwise compress everything back into 8 bits
        for (size_t i = 0; i < OUT; i += 16) {
            __m128i a1 = _mm_load_si128((__m128i *)&temp_out[i]);
            __m128i a2 = _mm_load_si128((__m128i *)&temp_out[i+4]);
            __m128i a3 = _mm_load_si128((__m128i *)&temp_out[i+8]);
            __m128i a4 = _mm_load_si128((__m128i *)&temp_out[i+12]);
            __m128i bias1 = _mm_load_si128((__m128i *) &biases[i+bucket*OUT]);
            __m128i bias2 = _mm_load_si128((__m128i *) &biases[i+4+bucket*OUT]);
            __m128i bias3 = _mm_load_si128((__m128i *) &biases[i+8+bucket*OUT]);
            __m128i bias4 = _mm_load_si128((__m128i *) &biases[i+12+bucket*OUT]);
            a1 = _mm_add_epi32(a1, bias1);
            a2 = _mm_add_epi32(a2, bias2);
            a3 = _mm_add_epi32(a3, bias3);
            a4 = _mm_add_epi32(a4, bias4);

            __m128i b1 = _mm_packs_epi32(a1, a2);
            __m128i b2 = _mm_packs_epi32(a3, a4);

            // apply relu here
            b1 = _mm_max_epi16(b1, _mm_setzero_si128());
            b2 = _mm_max_epi16(b2, _mm_setzero_si128());
            __m128i c = _mm_packs_epi16(b1, b2);
            _mm_store_si128((__m128i *)&output[i], c);
        }
        return 0;
    }
};
