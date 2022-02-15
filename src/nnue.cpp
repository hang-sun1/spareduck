#include <algorithm>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
#include <immintrin.h>
#include <assert.h>
#include <memory>
#include <new>
#include <optional>
#include <iostream>
#include <assert.h>

#include "move.hpp"
#include "nnue.hpp"
#include "piece.hpp"

#ifndef TESTING
#endif

int NNUE::evaluate(size_t piece_count, Side side_to_move) {
    // return 1;
    if (!ready) {
        return 0;
    }
    //
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

        _mm_store_si128((__m128i *) &a1_white_with_bias[i], packed_white);
        _mm_store_si128((__m128i *) &a1_black_with_bias[i], packed_black);
    }

    int32_t* psqtw_to_move;
    int32_t* psqtw_other;

    int8_t* a1_to_move;
    int8_t* a1_other;

    if (side_to_move == Side::WHITE) {
        psqtw_to_move = wps;
        psqtw_other = bps;
        a1_to_move = a1_white_with_bias;
        a1_other = a1_black_with_bias;
    } else {
        psqtw_to_move = wps;
        psqtw_other = bps;
        a1_to_move = a1_black_with_bias;
        a1_other = a1_white_with_bias;
    }

    auto side = static_cast<size_t>(side_to_move);
    // auto a = static_cast<int>(side) - 0.5;
    // auto a = side ? -2: 2;
    // auto a = 2;    
    size_t bucket = (piece_count - 1) / 4;
    // evaluate psqts
    for (int i = 0; i < 8; i += 4) {
        __m128i tmpsqt = _mm_load_si128((__m128i *) &psqtw_to_move[i]);
        __m128i opsqt = _mm_load_si128((__m128i *) &psqtw_other[i]);
        __m128i diff = _mm_sub_epi32(tmpsqt, opsqt);
        // __m128i as = _mm_set1_epi32(a);
        // __m128i res = _mm_div_epi32(diff, as);
        _mm_store_si128((__m128i *) &ps[i], diff);
    }

    // for (int i  = 0; i < 8; ++i) {
    //     ps[i] /= a;    
    // }

    // 3. construct a vector that is LAYER1_SIZE * 2 long containing the activations
    // of the combined black and white first layer (with the side to move at the start)
    // technically more efficient code would get around this memcpy but for now this is fine
    // TODO order this based on the side to move
    // std::array<int8_t, LAYER1_SIZE*2> combined;
    // int8_t* combined = new int8_t[LAYER1_SIZE*2];
    alignas(16) int8_t combined[LAYER1_SIZE*2];
    // std::unique_ptr<int8_t[]> combined = std::make_unique<int8_t[]>(LAYER1_SIZE*2);
    std::memcpy(combined, a1_to_move, LAYER1_SIZE);
    std::memcpy(&combined[LAYER1_SIZE], a1_other, LAYER1_SIZE);

    // 4. go through the rest of the hidden layers
    compute_activation<LAYER1_SIZE*2, LAYER2_SIZE>(combined, w1, b2, a2, piece_count);
    compute_activation<LAYER2_SIZE, LAYER3_SIZE>(a2, w2, b3, a3, piece_count);

    // 5. calculate the output layer and return;
    auto eval = compute_activation<LAYER3_SIZE, 1>(a3, w3, b4, nullptr, piece_count);
    
    return (eval * (static_cast<size_t>(side_to_move) ? 1 : 1) + ps[bucket] / 2) / 64;
}

void NNUE::update_non_king_move(Move move, Piece moved_piece, std::optional<Piece> captured, std::optional<Piece> promoted, uint8_t white_king_square,
    uint8_t black_king_square, Side side_that_moved, bool reverse_move) {
        if (!ready) {
            return;
        }
        auto side = static_cast<size_t>(side_that_moved);
        auto other = static_cast<Side>(1 - side);
        auto origin_index_wpov = halfka_index(true, white_king_square, move.origin_square(), moved_piece, side_that_moved);
        auto origin_index_bpov = halfka_index(false, black_king_square, move.origin_square(), moved_piece, side_that_moved);

        if (!reverse_move) {
            update_first_layer_sub(origin_index_wpov, origin_index_bpov);
        } else {
            update_first_layer_add(origin_index_wpov, origin_index_bpov);
        }
        if (move.type() == MoveType::EN_PASSANT) {
            int diff = size_t(side_that_moved) ? -8 : 8;
            auto piece = captured.value();
            auto dest = uint8_t(int(move.destination_square()) + diff);
            
            auto idx_wpov = halfka_index(true, white_king_square, dest, piece, other);
            auto idx_bpov = halfka_index(false, black_king_square, dest, piece, other);
            if (!reverse_move) {
                update_first_layer_sub(idx_wpov, idx_bpov);
            } else {
                update_first_layer_add(idx_wpov, idx_bpov);
            }
        } else if (captured.has_value()) {
            auto piece = captured.value();
            auto idx_wpov = halfka_index(true, white_king_square, move.destination_square(), piece, other);
            auto idx_bpov = halfka_index(false, black_king_square, move.destination_square(), piece, other);
            if (!reverse_move) {
                update_first_layer_sub(idx_wpov, idx_bpov);
            } else {
                update_first_layer_add(idx_wpov, idx_bpov);
            }
        }

        if (move.is_promotion()) {
            size_t idx_wpov;
            size_t idx_bpov;
            if (move.type() == MoveType::PROMOTE_TO_QUEEN || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_QUEEN) {
                idx_wpov = halfka_index(true, white_king_square, move.destination_square(), QUEEN, side_that_moved);
                idx_bpov = halfka_index(false, black_king_square, move.destination_square(), QUEEN, side_that_moved);
            } else if (move.type() == MoveType::PROMOTE_TO_ROOK || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_ROOK) {
                idx_wpov = halfka_index(true, white_king_square, move.destination_square(), ROOK, side_that_moved);
                idx_bpov = halfka_index(false, black_king_square, move.destination_square(), ROOK, side_that_moved);
            } else if (move.type() == MoveType::PROMOTE_TO_BISHOP || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_BISHOP) {
                idx_wpov = halfka_index(true, white_king_square, move.destination_square(), BISHOP, side_that_moved);
                idx_bpov = halfka_index(false, black_king_square, move.destination_square(), BISHOP, side_that_moved);
            } else if (move.type() == MoveType::PROMOTE_TO_KNIGHT || move.type() == MoveType::CAPTURE_AND_PROMOTE_TO_KNIGHT) {
                idx_wpov = halfka_index(true, white_king_square, move.destination_square(), KNIGHT, side_that_moved);
                idx_bpov = halfka_index(false, black_king_square, move.destination_square(), KNIGHT, side_that_moved);
            }
            if (!reverse_move) {
                update_first_layer_add(idx_wpov, idx_bpov);
            } else {
                update_first_layer_sub(idx_wpov, idx_bpov);
            }
        } else {
            auto idx_wpov = halfka_index(true, white_king_square, move.destination_square(), moved_piece, side_that_moved);
            auto idx_bpov = halfka_index(false, black_king_square, move.destination_square(), moved_piece, side_that_moved);
            // update_first_layer_add(idx_wpov, idx_bpov);
            if (!reverse_move) {
                update_first_layer_add(idx_wpov, idx_bpov);
            } else {
                update_first_layer_sub(idx_wpov, idx_bpov);
            }
        }
}

void NNUE::reset_nnue(std::optional<Piece> captured, Board &b) {
    if (!ready) { 
        return;
    }
    
    std::memset(wps, 0, 8*sizeof(int32_t));
    std::memset(a1_white, 0, LAYER1_SIZE*sizeof(int16_t));
    std::memset(bps, 0, 8*sizeof(int32_t));
    std::memset(a1_black, 0, LAYER1_SIZE*sizeof(int16_t));

    auto kings = b.get_kings(); 
    uint8_t white_king_square = __builtin_ffsll(kings[0]) - 1;
    uint8_t black_king_square = __builtin_ffsll(kings[1]) - 1; 

    std::array<Piece, 6> pieces = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
    for (auto &p: pieces) {
        auto white_locs = b.get_piece_pos(p, Side::WHITE);
        auto black_locs = b.get_piece_pos(p, Side::BLACK);
        for (auto &loc: white_locs) {
            auto idx_wpov = halfka_index(true, white_king_square, loc, p, Side::WHITE);
            auto idx_bpov = halfka_index(false, black_king_square, loc, p, Side::WHITE);
            update_first_layer_add(idx_wpov, idx_bpov);
        }

        for (auto &loc: black_locs) {
            auto idx_wpov = halfka_index(true, white_king_square, loc, p, Side::BLACK);
            auto idx_bpov = halfka_index(false, black_king_square, loc, p, Side::BLACK);
            update_first_layer_add(idx_wpov, idx_bpov);
        }
    }
}

size_t NNUE::halfka_index(bool is_white_pov, uint8_t king_square, uint8_t square, Piece piece, Side side_of_piece) {
    size_t side = 1 - static_cast<size_t>(side_of_piece);
    size_t pov = is_white_pov ? 1: 0;
    size_t color_offset = side == pov ? 0 : 1;
    size_t index = piece * 2 + color_offset;
    size_t oriented_square = (56 * (!pov)) ^ static_cast<size_t>(square);
    index = 1 + oriented_square + index * 64 + static_cast<size_t>(king_square) * (64 * 12 + 1);
    if (index >= 49216) {
        std::cout << "Invalid halfka index generated " << (int) king_square << " " << (int) square << " " << piece << " " << 1 - side << std::endl;;
    }
    return index;
}

void NNUE::update_first_layer_add(size_t white_pov_index, size_t black_pov_index) {
    assert(white_pov_index < 49216);
    assert(black_pov_index < 49216);
    for (size_t i = 0; i < LAYER1_SIZE; i += 8) { 
        __m128i white1 = _mm_load_si128((__m128i *) &w0[white_pov_index*LAYER1_SIZE+i]);

        __m128i black1 = _mm_load_si128((__m128i*) &w0[black_pov_index*LAYER1_SIZE+i]);

        __m128i act_white = _mm_load_si128((__m128i *) &a1_white[i]);
        __m128i act_black = _mm_load_si128((__m128i *) &a1_black[i]);
        
        act_white = _mm_add_epi16(white1, act_white);
        act_black = _mm_add_epi16(black1, act_black);

        _mm_store_si128((__m128i *) &a1_white[i], act_white);
        _mm_store_si128((__m128i *) &a1_black[i], act_black);
    }

    for (size_t i = 0; i < 8; i += 4) {
        __m128i w_psqt_wts = _mm_load_si128((__m128i *) &psqt_wts[white_pov_index*8+i]);
        __m128i b_psqt_wts = _mm_load_si128((__m128i *) &psqt_wts[black_pov_index*8+i]);
        
        __m128i act_white = _mm_load_si128((__m128i *) &wps[i]);
        __m128i act_black = _mm_load_si128((__m128i *) &bps[i]);
        
        act_white = _mm_add_epi32(w_psqt_wts, act_white);
        act_black = _mm_add_epi32(b_psqt_wts, act_black);

        _mm_store_si128((__m128i *) &wps[i], act_white);
        _mm_store_si128((__m128i *) &bps[i], act_black);
    }
}

void NNUE::update_first_layer_sub(size_t white_pov_index, size_t black_pov_index) {
    assert(white_pov_index < 49216);
    assert(black_pov_index < 49216);
    for (size_t i = 0; i < LAYER1_SIZE; i += 8) { 
        __m128i white1 = _mm_load_si128((__m128i *) &w0[white_pov_index*LAYER1_SIZE+i]);

        __m128i black1 = _mm_load_si128((__m128i*) &w0[black_pov_index*LAYER1_SIZE+i]);

        __m128i act_white = _mm_load_si128((__m128i *) &a1_white[i]);
        __m128i act_black = _mm_load_si128((__m128i *) &a1_black[i]);
        
        act_white = _mm_sub_epi16(act_white, white1);
        act_black = _mm_sub_epi16(act_black, black1);

        _mm_store_si128((__m128i *) &a1_white[i], act_white);
        _mm_store_si128((__m128i *) &a1_black[i], act_black);
    }

    for (size_t i = 0; i < 8; i += 4) {
        __m128i w_psqt_wts = _mm_load_si128((__m128i *) &psqt_wts[white_pov_index*8+i]);
        __m128i b_psqt_wts = _mm_load_si128((__m128i *) &psqt_wts[black_pov_index*8+i]);
        
        __m128i act_white = _mm_load_si128((__m128i *) &wps[i]);
        __m128i act_black = _mm_load_si128((__m128i *) &bps[i]);
        
        act_white = _mm_sub_epi32(act_white, w_psqt_wts);
        act_black = _mm_sub_epi32(act_black, b_psqt_wts);

        _mm_store_si128((__m128i *) &wps[i], act_white);
        _mm_store_si128((__m128i *) &bps[i], act_black);
    }
}

NNUE::~NNUE() {
    if (ready) { 
        delete [] w0;
        delete [] psqt_wts;
        delete [] wps;
        delete [] bps;
        delete [] ps;
        delete [] b1;
        delete [] a1_white;
        delete [] a1_white_with_bias;
        delete [] a1_black;
        delete [] a1_black_with_bias;
        delete [] w1;
        delete [] b2;
        delete [] a2;
        delete [] w2;
        delete [] b3;
        delete [] a3;
        delete [] w3;
        delete [] b4;
    }
}

NNUE::NNUE() {

}


#ifndef TESTING
NNUE::NNUE(Side current_to_move, emscripten_fetch_t* fetch) {
    w0 = new (std::align_val_t(16)) int16_t[INPUT_SIZE*LAYER1_SIZE];
    psqt_wts = new (std::align_val_t(16)) int32_t[INPUT_SIZE*8];
    wps = new (std::align_val_t(16)) int32_t[8];
    bps = new (std::align_val_t(16)) int32_t[8];
    ps = new (std::align_val_t(16)) int32_t[8];
    b1 = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    a1_white = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    a1_black = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    a1_white_with_bias = new (std::align_val_t(16)) int8_t[LAYER1_SIZE];
    a1_black_with_bias = new (std::align_val_t(16)) int8_t[LAYER1_SIZE];
    w1 = new (std::align_val_t(16)) int8_t[8*2*LAYER1_SIZE*LAYER2_SIZE];
    b2 = new (std::align_val_t(16)) int32_t[8*LAYER2_SIZE];
    a2 = new (std::align_val_t(16)) int8_t[LAYER2_SIZE];
    
    w2 = new (std::align_val_t(16)) int8_t[8*LAYER2_SIZE*LAYER3_SIZE];
    b3 = new (std::align_val_t(16)) int32_t[8*LAYER3_SIZE];
    a3 = new (std::align_val_t(16)) int8_t[LAYER3_SIZE];
    
    w3 = new (std::align_val_t(16)) int8_t[8*LAYER3_SIZE];
    b4 = new (std::align_val_t(16)) int32_t[8];

    
    size_t idx = 0;
    // multiply by 2 as the weights are stored as 16 bit integers
    std::memcpy((void*) w0, &fetch->data[idx], INPUT_SIZE*LAYER1_SIZE*2);
    idx += INPUT_SIZE*LAYER1_SIZE*2;
    // multiply by 4 as these weights are stored as 32 bit integers
    std::memcpy(psqt_wts, &fetch->data[idx], INPUT_SIZE*8*4);
    idx += INPUT_SIZE*8*4;
    // these biases are also 16 bit
    std::memcpy(b1, &fetch->data[idx], LAYER1_SIZE*2);
    idx += LAYER1_SIZE*2;

    size_t temp_idx = idx;
    for (int i = 0; i < 8; ++i) {
        // weights are stored as int8_t so no multiplication needed for the size
        std::memcpy(&w1[i*2*LAYER1_SIZE*LAYER2_SIZE], &fetch->data[temp_idx], 2*LAYER1_SIZE*LAYER2_SIZE);
        temp_idx += 2*LAYER1_SIZE*LAYER2_SIZE;

        std::memcpy(&b2[i*LAYER2_SIZE], &fetch->data[temp_idx], LAYER2_SIZE*4);
        temp_idx += LAYER2_SIZE*4;
        
        std::memcpy(&w2[i*LAYER2_SIZE*LAYER3_SIZE], &fetch->data[temp_idx], LAYER2_SIZE*LAYER3_SIZE);
        temp_idx += LAYER2_SIZE*LAYER3_SIZE;

        std::memcpy(&b3[i*LAYER3_SIZE], &fetch->data[temp_idx], LAYER3_SIZE*4);
        temp_idx += LAYER3_SIZE*4;
        
        std::memcpy(&w3[i*LAYER3_SIZE], &fetch->data[temp_idx], LAYER3_SIZE);
        temp_idx += LAYER3_SIZE;

        std::memcpy(&b4[i], &fetch->data[temp_idx], 4);
        temp_idx += 4;

        // temp_idx = idx;
    }
    std::cout << temp_idx << std::endl;
    ready = true;
}
#endif

NNUE::NNUE(const NNUE &nnue2) {
    w0 = new (std::align_val_t(16)) int16_t[INPUT_SIZE*LAYER1_SIZE];
    std::memcpy(w0, nnue2.w0, INPUT_SIZE*LAYER1_SIZE*2);

    psqt_wts = new (std::align_val_t(16)) int32_t[INPUT_SIZE*8];
    std::memcpy(psqt_wts, nnue2.psqt_wts, INPUT_SIZE*8*4);
    
    wps = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(wps, nnue2.wps, 8*4);

    bps = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(bps, nnue2.bps, 8*4);
    
    ps = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(ps, nnue2.ps, 8*4);

    b1 = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    std::memcpy(b1, nnue2.b1, LAYER1_SIZE*2);

    a1_white = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    std::memcpy(a1_white, nnue2.a1_white, LAYER1_SIZE*2);

    a1_black = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    std::memcpy(a1_black, nnue2.a1_black, LAYER1_SIZE*2);

    a1_white_with_bias = new (std::align_val_t(16)) int8_t[LAYER1_SIZE];
    std::memcpy(a1_white_with_bias, nnue2.a1_white_with_bias, LAYER1_SIZE);
    
    a1_black_with_bias = new (std::align_val_t(16)) int8_t[LAYER1_SIZE];
    std::memcpy(a1_black_with_bias, nnue2.a1_black_with_bias, LAYER1_SIZE);
    
    w1 = new (std::align_val_t(16)) int8_t[8*2*LAYER1_SIZE*LAYER2_SIZE];
    std::memcpy(w1, nnue2.w1, LAYER1_SIZE*LAYER2_SIZE*8*2);

    b2 = new (std::align_val_t(16)) int32_t[8*LAYER2_SIZE];
    std::memcpy(b2, nnue2.b2, LAYER2_SIZE*4*8);
    
    a2 = new (std::align_val_t(16)) int8_t[LAYER2_SIZE];
    std::memcpy(a2, nnue2.a2, LAYER2_SIZE);
    
    w2 = new (std::align_val_t(16)) int8_t[8*LAYER2_SIZE*LAYER3_SIZE];
    std::memcpy(w2, nnue2.w2, LAYER2_SIZE*LAYER3_SIZE*8);
    
    b3 = new (std::align_val_t(16)) int32_t[8*LAYER3_SIZE];
    std::memcpy(b3, nnue2.b3, LAYER3_SIZE*4*8);
    
    a3 = new (std::align_val_t(16)) int8_t[LAYER3_SIZE];
    std::memcpy(a3, nnue2.a3, LAYER3_SIZE);
    
    w3 = new (std::align_val_t(16)) int8_t[8*LAYER3_SIZE];
    std::memcpy(w3, nnue2.w3, LAYER3_SIZE*8);
    
    b4 = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(b4, nnue2.b4, 4*8);

    ready = nnue2.ready;
}

NNUE& NNUE::operator=(const NNUE &nnue2) {
    if (this == &nnue2) {
        return *this;
    }
    if (ready) { 
        delete [] w0;
        delete [] psqt_wts;
        delete [] wps;
        delete [] bps;
        delete [] ps;
        delete [] b1;
        delete [] a1_white;
        delete [] a1_white_with_bias;
        delete [] a1_black;
        delete [] a1_black_with_bias;
        delete [] w1;
        delete [] b2;
        delete [] a2;
        delete [] w2;
        delete [] b3;
        delete [] a3;
        delete [] w3;
        delete [] b4;
    }
    
    w0 = new (std::align_val_t(16)) int16_t[INPUT_SIZE*LAYER1_SIZE];
    std::memcpy(w0, nnue2.w0, INPUT_SIZE*LAYER1_SIZE*2);

    psqt_wts = new (std::align_val_t(16)) int32_t[INPUT_SIZE*8];
    std::memcpy(psqt_wts, nnue2.psqt_wts, INPUT_SIZE*8*4);
    
    wps = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(wps, nnue2.wps, 8*4);

    bps = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(bps, nnue2.bps, 8*4);
    
    ps = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(ps, nnue2.ps, 8*4);

    b1 = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    std::memcpy(b1, nnue2.b1, LAYER1_SIZE*2);

    a1_white = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    std::memcpy(a1_white, nnue2.a1_white, LAYER1_SIZE*2);

    a1_black = new (std::align_val_t(16)) int16_t[LAYER1_SIZE];
    std::memcpy(a1_black, nnue2.a1_black, LAYER1_SIZE*2);

    a1_white_with_bias = new (std::align_val_t(16)) int8_t[LAYER1_SIZE];
    std::memcpy(a1_white_with_bias, nnue2.a1_white_with_bias, LAYER1_SIZE);
    
    a1_black_with_bias = new (std::align_val_t(16)) int8_t[LAYER1_SIZE];
    std::memcpy(a1_black_with_bias, nnue2.a1_black_with_bias, LAYER1_SIZE);
    
    w1 = new (std::align_val_t(16)) int8_t[8*2*LAYER1_SIZE*LAYER2_SIZE];
    std::memcpy(w1, nnue2.w1, LAYER1_SIZE*LAYER2_SIZE*8*2);

    b2 = new (std::align_val_t(16)) int32_t[8*LAYER2_SIZE];
    std::memcpy(b2, nnue2.b2, LAYER2_SIZE*4*8);
    
    a2 = new (std::align_val_t(16)) int8_t[LAYER2_SIZE];
    std::memcpy(a2, nnue2.a2, LAYER2_SIZE);
    
    w2 = new (std::align_val_t(16)) int8_t[8*LAYER2_SIZE*LAYER3_SIZE];
    std::memcpy(w2, nnue2.w2, LAYER2_SIZE*LAYER3_SIZE*8);
    
    b3 = new (std::align_val_t(16)) int32_t[8*LAYER3_SIZE];
    std::memcpy(b3, nnue2.b3, LAYER3_SIZE*4*8);
    
    a3 = new (std::align_val_t(16)) int8_t[LAYER3_SIZE];
    std::memcpy(a3, nnue2.a3, LAYER3_SIZE);
    
    w3 = new (std::align_val_t(16)) int8_t[8*LAYER3_SIZE];
    std::memcpy(w3, nnue2.w3, LAYER3_SIZE*8);
    
    b4 = new (std::align_val_t(16)) int32_t[8];
    std::memcpy(b4, nnue2.b4, 4*8);
    this->ready = nnue2.ready;
    return *this;
}
