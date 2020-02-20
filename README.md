# ConnectFour

## About the game:
The game of Connect Four is played between two players (Red and Yellow) using a vertical board with 7 columns, with 6 slots in each column. Red always plays first, with players alternating turns. Each drops a marker with their color into any column with an empty slot. Gravity then causes that token to drop to the lowest unoccupied slot. Once a piece is played, it cannot be moved or altered. Each player tries to get 4 markers of their color in a straight line, either horizontally, vertically, or diagonally. Play continues until one player wins or there are no more legal moves (which is a draw, scored as a half-win for both players). It is not possible to ‘pass’ or skip a turn; a player must make a play if there is any legal play to make.

## Goal:
Write a program that plays Connect Four using a Monte Carlo tree search. For each turn, print the move selected, estimated wins, estimated probability this is the best move, and print the board position. Use a lightweight playout.

## TODO:
Give the program the ability to learn from its play by storing some or all of the positions that come up during search and their win/loss information. In future games, if the position arises during a search, its information can be put to use so the results of previous searches need not be lost. This allows the program to search deeper, and more effectively.
