#include <cstring>
#include <optional>
#include <iostream>

#include "nnue.hpp"

#ifndef TESTING
#include <emscripten/fetch.h>
#endif



int NNUE::evaluate(size_t piece_count) {
    // efficient updating should have already updated a1_white and a1_black.
    // however the bias and relu are not yet applied to them in order to facilitate
    // efficient updates
    
    // 1. apply the bias to the neurons of the first layer
    // 2. apply ReLu to the neurons of the first layer
    for (size_t i = 0; i < LAYER1_SIZE; i += 16) {
        __m128i bias = _mm_load_si128((__m128i *) &b1[i]);
        __m128i w = _mm_load_si128((__m128i *) &a1_white[i]);
        __m128i b = _mm_load_si128((__m128i *) &a1_black[i]);

        __m128i wplusb = _mm_add_epi16(w, bias);
        wplusb = _mm_max_epi16(wplusb, _mm_setzero_si128());
        __m128i bplusb = _mm_add_epi16(b, bias);
        bplusb = _mm_max_epi16(bplusb, _mm_setzero_si128());

        
        __m128i bias2 = _mm_load_si128((__m128i *) &b1[i+8]);
        __m128i w2 = _mm_load_si128((__m128i *) &a1_white[i+8]);
        __m128i b2 = _mm_load_si128((__m128i *) &a1_black[i+8]);

        __m128i wplusb2 = _mm_add_epi16(w2, bias2);
        wplusb2 = _mm_max_epi16(wplusb2, _mm_setzero_si128());
        __m128i bplusb2 = _mm_add_epi16(b2, bias2);
        bplusb2 = _mm_max_epi16(bplusb2, _mm_setzero_si128());

        __m128i packed_white = _mm_packs_epi16(wplusb, wplusb2);
        __m128i packed_black = _mm_packs_epi16(bplusb, bplusb2);

        _mm_store_si128((__m128i *) &a1_white_with_bias, packed_white);
        _mm_store_si128((__m128i *) &a1_black_with_bias, packed_black);
    }

    // 3. construct a vector that is LAYER1_SIZE * 2 long containing the activations
    // of the combined black and white first layer (with the side to move at the start)
    // technically more efficient code would get around this memcpy but for now this is fine
    // TODO order this based on the side to move
    std::array<int8_t, LAYER1_SIZE*2> combined;
    std::memcpy(&combined, &a1_white_with_bias, LAYER1_SIZE);
    std::memcpy(&combined[LAYER1_SIZE], &a1_black_with_bias, LAYER1_SIZE);

    // 4. go through the rest of the hidden layers
    compute_activation<LAYER1_SIZE*2, LAYER2_SIZE>(combined, w1, b2, a2, piece_count);
    compute_activation<LAYER2_SIZE, LAYER3_SIZE>(a2, w2, b3, a3, piece_count);

    // 5. calculate the output layer and return;
    std::array<int8_t, 1> dummy;
    return compute_activation<LAYER3_SIZE, 1>(a3, w3, b4, dummy, piece_count);
}

void NNUE::update_non_king_move(Move move, Piece moved_piece, std::optional<Piece> captured) {
    
}

void NNUE::update_king_move(Move move, std::optional<Piece> captured) {
    
}

NNUE::NNUE(Side current_to_move) {
    std::cout << "attempting to load network" << std::endl;
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
    emscripten_fetch_t* fetch = emscripten_fetch(&attr, "network.bin");
    if (fetch->status == 200) {
        std::cout << "successfully loaded network" << std::endl;
        size_t idx = 0;
        // multiply by 2 as the weights are stored as 16 bit integers
        std::memcpy(&w0, &fetch->data[idx], w0.size()*2);
        idx += w0.size()*2;
        // multiply by 4 as these weights are stored as 32 bit integers
        std::memcpy(&wps, &fetch->data[idx], wps.size()*4);
        idx += wps.size()*4;
        // these biases are also 16 bit
        std::memcpy(&b1, &fetch->data[idx], b1.size()*2);
        idx += b1.size()*2;

        size_t temp_idx = idx;
        for (int i = 0; i < 8; ++i) {
            // weights are stored as int8_t so no multiplication needed for the size
            std::memcpy(&w1[i*w1.size()/8], &fetch->data[temp_idx], w1.size());
            temp_idx += w1.size();

            std::memcpy(&b2[i*b2.size()/8], &fetch->data[temp_idx], b2.size()*4);
            temp_idx += b2.size()*4;
            
            std::memcpy(&w2[i*w2.size()/8], &fetch->data[temp_idx], w2.size());
            temp_idx += w2.size();

            std::memcpy(&b3[i*b3.size()/8], &fetch->data[temp_idx], b3.size()*4);
            temp_idx += b3.size()*4;
            
            std::memcpy(&w3[i*w3.size()/8], &fetch->data[temp_idx], w3.size());
            temp_idx += w3.size();

            std::memcpy(&b4[i*b4.size()/8], &fetch->data[temp_idx], b4.size()*4);
            temp_idx += b4.size()*4;

            temp_idx = idx;
        }
    } else {
        std::cout << "failed to load network" << std::endl;
    }
    emscripten_fetch_close(fetch)
}
