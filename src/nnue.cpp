#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <iostream>

#include "nnue.hpp"

#ifndef TESTING
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
    // std::array<int8_t, LAYER1_SIZE*2> combined;
    std::unique_ptr<int8_t[]> combined = std::make_unique<int8_t[]>(LAYER1_SIZE*2);
    std::memcpy(combined.get(), &a1_white_with_bias, LAYER1_SIZE);
    std::memcpy(&combined.get()[LAYER1_SIZE], &a1_black_with_bias, LAYER1_SIZE);

    // 4. go through the rest of the hidden layers
    compute_activation<LAYER1_SIZE*2, LAYER2_SIZE>(combined.get(), w1.get(), b2.get(), a2.get(), piece_count);
    compute_activation<LAYER2_SIZE, LAYER3_SIZE>(a2.get(), w2.get(), b3.get(), a3.get(), piece_count);

    // 5. calculate the output layer and return;
    return compute_activation<LAYER3_SIZE, 1>(a3.get(), w3.get(), b4.get(), nullptr, piece_count);
}

void NNUE::update_non_king_move(Move move, Piece moved_piece, std::optional<Piece> captured) {
    
}

void NNUE::update_king_move(Move move, std::optional<Piece> captured) {
    
}

NNUE::NNUE() {

}

NNUE::NNUE(Side current_to_move, emscripten_fetch_t* fetch) {
    w0 = std::make_unique<int16_t[]>(INPUT_SIZE*LAYER1_SIZE);
    wps = std::make_unique<int32_t[]>(INPUT_SIZE*8);
    b1 = std::make_unique<int16_t[]>(LAYER1_SIZE);
    a1_white = std::make_unique<int16_t[]>(LAYER1_SIZE);
    a1_white_with_bias = std::make_unique<int8_t[]>(LAYER1_SIZE);
    a1_black = std::make_unique<int16_t[]>(LAYER1_SIZE);
    a1_black_with_bias = std::make_unique<int8_t[]>(LAYER1_SIZE);
    w1 = std::make_unique<int8_t[]>(8*2*LAYER1_SIZE*LAYER2_SIZE);
    b2 = std::make_unique<int32_t[]>(8*LAYER2_SIZE);
    a2 = std::make_unique<int8_t[]>(LAYER2_SIZE);
    
    w2 = std::make_unique<int8_t[]>(8*LAYER2_SIZE*LAYER3_SIZE);
    b3 = std::make_unique<int32_t[]>(8*LAYER3_SIZE);
    a3 = std::make_unique<int8_t[]>(LAYER3_SIZE);
    
    w3 = std::make_unique<int8_t[]>(8*LAYER3_SIZE);
    b4 = std::make_unique<int32_t[]>(8);
    
    size_t idx = 0;
    // multiply by 2 as the weights are stored as 16 bit integers
    std::memcpy((void*) w0.get(), &fetch->data[idx], INPUT_SIZE*LAYER1_SIZE*2);
    idx += INPUT_SIZE*LAYER1_SIZE*2;
    // multiply by 4 as these weights are stored as 32 bit integers
    std::memcpy(wps.get(), &fetch->data[idx], INPUT_SIZE*8*4);
    idx += INPUT_SIZE*8*4;
    // these biases are also 16 bit
    std::memcpy(b1.get(), &fetch->data[idx], LAYER1_SIZE*2);
    idx += LAYER1_SIZE*2;

    size_t temp_idx = idx;
    for (int i = 0; i < 8; ++i) {
        // weights are stored as int8_t so no multiplication needed for the size
        std::memcpy(&w1.get()[i*LAYER1_SIZE*LAYER2_SIZE*2], &fetch->data[temp_idx], 2*LAYER1_SIZE*LAYER2_SIZE);
        temp_idx += 2*LAYER1_SIZE*LAYER2_SIZE;

        std::memcpy(&b2.get()[i*LAYER2_SIZE], &fetch->data[temp_idx], LAYER2_SIZE*4);
        temp_idx += LAYER2_SIZE*4;
        
        std::memcpy(&w2.get()[i*LAYER2_SIZE*LAYER3_SIZE], &fetch->data[temp_idx], LAYER2_SIZE*LAYER3_SIZE);
        temp_idx += LAYER2_SIZE*LAYER3_SIZE;

        std::memcpy(&b3.get()[i*LAYER3_SIZE], &fetch->data[temp_idx], LAYER3_SIZE*4);
        temp_idx += LAYER3_SIZE*4;
        
        std::memcpy(&w3.get()[i*LAYER3_SIZE], &fetch->data[temp_idx], LAYER3_SIZE);
        temp_idx += LAYER3_SIZE;

        std::memcpy(&b4.get()[i], &fetch->data[temp_idx], 8*4);
        temp_idx += 8*4;

        temp_idx = idx;
    }
}
