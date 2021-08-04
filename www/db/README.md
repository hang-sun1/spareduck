### Puzzle database:

A stripped down version of [lichess's puzzle database](https://database.lichess.org/#puzzles). The original is not committed to save space. To change any of the fields of the db you should download the lichess db locally then edit and run `strip.py`.

#### Puzzle selection params:

**`count`**: number of positions

**`type`**: type of positions. one of `'popular', rating (number), 'advantage', 'endgame', 'opening', 'short','crushing', 'exposedKing', 'long', 'middlegame', 'skewer','advancedPawn', 'bishopEndgame','hangingPiece','defensiveMove','master', 'masterVsMaster','superGM','attraction','discoveredAttack', 'kingsideAttack','capturingDefender','deflection','mateIn3','mate']`. and im sure there are more documented somewhere.

#### Format:

To avoid problems with csv the output is a js file.

Example:

```js
{
    // Metadata
    num_records: 100,
    type: 'popular',
    // Positions
    records:[
        {
            fen:'5rk1/1p3ppp/pq3b2/8/8/1P1Q1N2/P4PPP/3R2K1 w - - 2 27',
            moves:['d3d6', 'f8d8', 'd6d8', 'f6d8'],
            rating:'1550',
            themes:['advantage', 'endgame', 'short']
        },
        //...
    ]
}
```
