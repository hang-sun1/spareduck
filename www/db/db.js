module.exports = {
  num_records: 20,
  type: 'mateIn1',
  records: [
    {
      fen: '6k1/2p2ppp/pnp5/B7/2P3PP/1P1bPPR1/r6r/3R2K1 b - - 1 29',
      moves: ['d3e2', 'd1d8'],
      rating: '607',
      themes: ['backRankMate', 'mate', 'mateIn1', 'middlegame', 'oneMove'],
    },
    {
      fen: '6k1/pb2r1pN/1n4Bp/3p4/1P2pR2/P7/3R1PPP/2r3K1 w - - 2 30',
      moves: ['d2d1', 'c1d1'],
      rating: '706',
      themes: [
        'backRankMate',
        'hangingPiece',
        'mate',
        'mateIn1',
        'middlegame',
        'oneMove',
      ],
    },
    {
      fen: '2k4r/pp3pp1/4pn2/2np2p1/8/1B1P1Pq1/PPPN1R2/R2Q3K w - - 6 20',
      moves: ['f2h2', 'g3h2'],
      rating: '754',
      themes: ['kingsideAttack', 'mate', 'mateIn1', 'middlegame', 'oneMove'],
    },
    {
      fen: '1r3rk1/2p1Nppb/p2nq3/1p2p1Pp/4Qn1P/2P1N3/PPB2P1K/3R2R1 b - - 5 28',
      moves: ['e6e7', 'e4h7'],
      rating: '865',
      themes: ['kingsideAttack', 'mate', 'mateIn1', 'middlegame', 'oneMove'],
    },
    {
      fen: '3kRr2/3n1B1p/2pP4/p1n5/Ppp5/8/1P3PPP/4R1K1 b - - 8 32',
      moves: ['f8e8', 'e1e8'],
      rating: '881',
      themes: ['endgame', 'mate', 'mateIn1', 'oneMove'],
    },
  ],
};
