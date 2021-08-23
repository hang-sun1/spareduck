#include "piece.hpp"

int piece_to_value(Piece p) {
  switch (p) {
    case PAWN:
        return 100;
    case KNIGHT:
        return 280;
    case BISHOP:
        return 310;
    case ROOK:
        return 500;
    case QUEEN:
        return 900;
    case KING:
        return 10000;
  }
}
