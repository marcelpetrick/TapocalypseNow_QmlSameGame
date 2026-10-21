# Tapocalypse Rules

This file is the source of truth for Tapocalypse gameplay semantics.

Its purpose is to make the game:

- easy to understand
- precise enough that different implementations produce the same result

If UI text, animations, or README wording disagree with this file, this file wins.

## 1. Scope

This file defines:

- board structure
- connectivity and group rules
- legal and illegal clicks
- score calculation
- board collapse behavior
- end-of-game conditions
- highscore admission rules
- gameplay invariants

This file does not define:

- visual theme
- animation style or timing
- sound effects
- button placement
- rendering technology

Those are presentation details and must not change gameplay outcomes.

## 2. Definitions

- A **board** is a rectangular grid with `rows` and `cols`.
- A **cell** is one position on the board.
- A cell contains either:
  - one colored tile
  - or empty
- Coordinates are zero-based.
- The top-left cell is `(row=0, col=0)`.
- Row numbers increase downward.
- Column numbers increase to the right.
- Two tiles **match** only if they have exactly the same color.
- Two tiles are **adjacent** only if they touch up, down, left, or right.
- Diagonal contact does not count as adjacency.
- A **group** is the full set of same-colored tiles connected through adjacency.

## 3. Fixed Parameters

The current game rules use these fixed values:

- `minimumGroupSize = 3`
- `scoreForGroup(N) = N * N`
- `highscoreLimit = 10`
- `maxPlayerNameLength = 20`

If the product changes any of these values, this file must be updated.

## 4. Goal

The goal is to finish the game with the highest score possible.

## 5. Legal Move Rule

A click is legal only if the clicked tile belongs to a group whose size is at least `minimumGroupSize`.

With the current parameters:

- size `1` group: illegal
- size `2` group: illegal
- size `3+` group: legal

If a click is illegal:

- no tiles are removed
- score does not change
- move count does not change
- the board state does not change

## 6. Move Resolution

When the player makes a legal click, the game must resolve the move in this order:

1. Find the full connected group of the clicked tile.
2. Remove all tiles in that group.
3. Add score using `scoreForGroup(groupSize)`.
4. Increase the move count by `1`.
5. Collapse the board.
6. Check whether any legal move remains.
7. If no legal move remains, transition to game over.

Only the full connected group may be removed. Partial removal of a group is always a bug.

## 7. Scoring

If a removed group has size `N`, the score added for that move is:

`N * N`

Examples:

- `3` tiles -> `9` points
- `4` tiles -> `16` points
- `5` tiles -> `25` points

There is currently:

- no combo multiplier
- no chain bonus
- no board-clear bonus
- no penalty for leftover tiles

## 8. Collapse Rules

After every legal move, collapse must happen in exactly two phases.

### Phase 1: Fall Down

Each column is processed independently.

For each column:

- all non-empty tiles fall straight downward
- the vertical order of those non-empty tiles is preserved
- empty cells end up above the remaining tiles

After this phase, there must not be any tile with an empty cell directly below it.

### Phase 2: Shift Left

After all columns have finished falling:

- every completely empty column is removed from the used play area
- all non-empty columns shift left
- the left-to-right order of non-empty columns is preserved

After this phase:

- there must be no empty column between non-empty columns
- any remaining empty columns may exist only at the far right

## 9. Reference Collapse Algorithm

The board after a legal move must be identical to the board produced by this reference procedure:

1. Start with the board after removal.
2. For each original column, read tiles from bottom to top.
3. Keep only non-empty tiles from that column.
4. Write those kept tiles back into the same column from bottom upward.
5. Discard any column that became fully empty.
6. Pack the remaining non-empty columns leftward in their original order.
7. Fill every remaining unused cell with empty.

This is the normative collapse behavior.

## 10. End Condition

The game ends when no legal move exists anywhere on the board.

With the current parameters, that means:

- the board contains no group of size `3` or greater

The board may still contain tiles when the game ends.

### Ending a run early

The player may also end a run voluntarily ("End Run") while it is `playing`, after at least one legal move and
while no move is still resolving. The run then ends exactly as if no legal move remained: the board freezes, the
state becomes `gameOver`, and the current score is the final score.

## 11. Game State

The gameplay state machine should be interpreted as follows:

- `playing`: the board accepts clicks and legal moves may be resolved
- `gameOver`: no legal moves remain; the board must not accept further gameplay moves
- `submittingHighscore`: the run has ended and the player may enter a qualifying score

Required behavior:

- the board must not mutate after game over except through explicit game reset or new game flow
- one finished run may be submitted at most once
- a new game starts a fresh run with score `0` and move count `0`

Animations may delay presentation, but must not change the logical result of a move.

## 12. Highscore Rules

When a run ends, its final score may be considered for the highscore table.

The current highscore rules are:

- only the best `10` scores are kept
- higher scores rank ahead of lower scores
- if two scores are equal, the older stored entry stays ahead of the newer entry
- player names are trimmed
- player names are limited to `20` characters after trimming

Admission rules:

- if fewer than `10` scores exist, any positive final score qualifies
- if `10` scores already exist, a new score qualifies only if it is strictly greater than the current lowest stored score
- equal-to-lowest scores do not qualify
- a score of `0` does not qualify
- a run ended early with "End Run" (§10) is a finished run and qualifies under the same rules
- a run abandoned by starting a new game or closing the app is not submitted

## 13. Invariants

The following must always be true in a correct implementation:

- only orthogonal adjacency counts
- diagonal touching never counts as connected
- a legal click removes the whole connected group, not part of it
- a group smaller than `minimumGroupSize` is never removable
- illegal clicks never change score, move count, or board state
- after collapse, no tile floats above an empty cell
- after collapse, no empty column exists between non-empty columns
- the order of tiles within a column is preserved during falling
- the order of non-empty columns is preserved during left shift
- the same finished run cannot be submitted multiple times

## 14. Worked Examples

### Example A: Legal Group of 3

Start board:

```text
R R B
R G B
Y G B
```

If the player clicks any blue tile:

- the blue group size is `3`
- the move is legal
- all `3` blue tiles are removed

Board after removal:

```text
R R .
R G .
Y G .
```

After fall-down:

```text
R R .
R G .
Y G .
```

After shift-left:

```text
R R
R G
Y G
```

### Example B: Illegal Group of 2

Start board:

```text
R B G
Y B G
R Y R
```

If the player clicks a blue tile:

- the blue group size is `2`
- the move is illegal
- no tiles are removed
- score does not change
- moves do not change
- the board stays exactly the same

### Example C: Tiles Remain But Game Is Over

Start board:

```text
R G B
Y R G
B Y R
```

Every tile is isolated.

That means:

- no group of size `3` or greater exists
- the game is over even though tiles remain on the board

## 15. Summary

Short version:

- click a group of `3` or more matching tiles
- remove the whole connected group
- gain `N * N` points for a group of size `N`
- tiles fall down
- empty columns shift left
- the game ends when no legal move remains, or when you choose "End Run"
