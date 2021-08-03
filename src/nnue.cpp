#include <cstring>
#include <optional>

#include "nnue.hpp"


int NNUE::evaluate() {
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
    compute_activation<LAYER1_SIZE*2, LAYER2_SIZE>(combined, w1, b2, a2);
    compute_activation<LAYER2_SIZE, LAYER3_SIZE>(a2, w2, b3, a3);

    // 5. calculate the output layer and return;
    std::array<int8_t, 1> dummy;
    return compute_activation<LAYER3_SIZE, 1>(a3, w3, b4, dummy);
}

void NNUE::update_non_king_move(Move move, PieceType moved_piece, std::optional<PieceType> captured) {
    
}

void NNUE::update_king_move(Move move, std::optional<PieceType> captured) {
    
}
