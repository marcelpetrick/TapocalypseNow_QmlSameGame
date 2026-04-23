# Tapocalypse Rules

This file explains the rules of **Tapocalypse**.

It has two jobs:

- be easy enough for a **10-year-old**
- be clear enough that programmers can build the **same game logic every time**

## 1. What Kind of Game Is This?

Tapocalypse is a **SameGame-style** puzzle game.

That means:

- you click groups of matching tiles
- the whole group disappears
- tiles above fall down
- empty columns slide left

Important:

- some old SameGame versions allow removing groups of **2**
- **Tapocalypse does not**
- in Tapocalypse, a legal group must have **3 or more** tiles

So this game is **SameGame-inspired**, but with a strict **3-or-more rule**.

## 2. The Goal

Get the **highest score you can** before there are **no legal moves left**.

## 3. The Board

- The board is a grid with `rows` and `cols`.
- Each cell contains either:
  - a colored tile
  - or nothing (an empty cell)
- The top-left cell is row `0`, column `0`.
- Row numbers go **downward**.
- Column numbers go **to the right**.

## 4. Colors

- A tile has one color.
- Two tiles only match if they have the **exact same color**.

## 5. What Counts as Connected?

Two tiles are connected only if they touch:

- up
- down
- left
- right

Diagonal touching does **not** count.

## 6. What Is a Group?

A **group** is all tiles that:

- have the same color
- are connected by up/down/left/right steps

This is important:

- if one red tile touches another red tile, and that one touches another red tile, they are all in the **same group**
- a group can bend or snake around
- you never remove just part of a connected group

## 7. When a Click Is Legal

A click is legal only if the clicked tile belongs to a group of size **3 or more**.

That means:

- size `1` group: illegal
- size `2` group: illegal
- size `3+` group: legal

If the click is illegal:

- no tiles are removed
- score does not change
- move counter does not change

## 8. What Happens on a Legal Click

When the player clicks a legal tile:

1. Find the full connected group of matching tiles.
2. Remove **all** tiles in that group.
3. Add score.
4. Increase move count by `1`.
5. Collapse the board.
6. Check whether any legal move is left.

## 9. Scoring

If the removed group size is `N`, the player scores:

`N × N`

Examples:

- `3` tiles = `9` points
- `4` tiles = `16` points
- `5` tiles = `25` points

There is currently:

- no combo multiplier
- no chain bonus
- no bonus for clearing the whole board
- no penalty for leftover tiles

## 10. Collapse Rules

Collapse always happens in exactly **two steps**.

### Step 1: Fall Down

Inside each column:

- all non-empty tiles fall straight down
- their left/right order does not change
- empty cells end up at the top of that column

There must never be a tile with an empty cell directly below it after this step.

### Step 2: Shift Left

After all columns have finished falling:

- every completely empty column is removed from the middle
- all non-empty columns slide left
- the order of non-empty columns stays the same

After this step:

- there must be no empty column between two non-empty columns
- empty columns, if any, may exist only at the far right

## 11. Reproducible Board Rule

To make implementations consistent, the board after a move must be the same as if you do this:

1. Look at each original column from bottom to top.
2. Keep only the non-empty tiles from that column.
3. Write those kept tiles back from bottom upward.
4. Skip columns that became fully empty.
5. Pack the remaining non-empty columns leftward in their original order.
6. Fill all remaining cells with empty.

That is the expected reference behavior.

## 12. When the Game Ends

The game ends when there is **no group of 3 or more** anywhere on the board.

The board may still contain tiles.
The game is over because none of those tiles belong to a legal group anymore.

## 13. Highscores

When the game ends:

- the final score may enter the highscore table
- only the best **10** scores are kept
- if two scores are equal, the **older** one stays ahead

Player names:

- are trimmed
- are limited to `20` characters

## 14. Things That Must Never Happen

These are bugs:

- removing only part of a connected group
- counting diagonal neighbors as connected
- allowing a group of `2`
- leaving a tile floating above an empty cell after collapse
- leaving an empty column between non-empty columns
- changing the order of tiles within a column during falling
- changing the order of non-empty columns during left shift

## 15. Small Example

Start board:

```text
R R B
R G B
Y G B
```

If the player clicks a blue tile:

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

Nothing moves down here because every remaining tile already has support.

After shift-left:

```text
R R
R G
Y G
```

The empty right column is gone from the used play area.

## 16. Simple Version

If you want the short version:

- click a group of **3 or more**
- remove the **whole group**
- score = **group size squared**
- tiles fall **down**
- empty columns slide **left**
- game ends when no group of **3 or more** is left
