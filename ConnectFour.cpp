/*
  ConnectFour.cpp

  Purpose: For each turn, print the move selected, estimated wins, estimated probability this is the best move, and print the board position

  Version: 2.4.1

  Version History (And Goals):
  0.1)   Created c4Board class
  0.2)   Operator overload for <<
  0.3)   Changed internal storage type for c4Board (using vector<vector<int>>)
  0.4)   Fixed some errors from version 0.3
  0.5)   create dropToken() and ammended int main() to make use of it
  0.5.1) Figured out what was wrong with dropToken(), fixed player turns and board state updating
  0.6)   Added gameOver() to detect a final board state
  0.6.1) Fixed issue that prevented players from dropping tokens in the last column
  1.0)   IT WORKS!!!
  1.1)   Started defining Node class and member functions
  1.2)   Finished isPossible() and getGameState()
  1.3)   Finished getChildNode()
  1.4)   Defined movesLeft()
  1.5)   Most of the funcitonality for Node::makeMove() and Node::sampleNodePath() is there
  1.5.1) sampleNodePath() iterates to a final gameState the given number of times
  1.6)   Fixed an issue with c4Board and Node class getGameState() functions
  1.6.1) Made changes to Node::makeMove() & Node::sampleNodePath() to actually provide feedback on best choice
  -----------------------------
  2.0)   Added AI
  2.1)   Fixed sampleNodePath()
  2.2)   Added Probability of Winning calculation
  2.3)   Gave MCTS full control of board. TODO: Tweak selection criteria, still choosing columns that are full
  2.4)   Tweaked selection criteria.
  2.4.1) Fixed an issue where column 0 was never chosen. Literally didn't do anything, but the problem is gone now...

  Program Algorithm:
  1. Initialize an instance of c4Board for a new game
  2. Get move from player (or AI)
  3. If move is legal, print new move to console
  4. When a player has 4 tokens in a row, print "Win" message to console
    4a) If no player wins, print "Draw" message

  Solution algorithm:
  1. Based on current gameState, see what plays are possible. (for columns 0-6)
  2. For each possible move, determine immediate counter moves
  3. For each possible counter move, randomly select moves until the game is over
    3a) Keep track of wins, loses, and draws
    3b) If a node has all subsequent states calculated, write that information to a text file
    3c) If a stored node shows up in a different search some time, don't evaluate it again
  4. Backpropogate w/l/d data, as well as the number of nodes tested.
  5. Choose the move with the highest number of wins to draws or losses (confidence = number of nodes tried)

  TODO LIST:
  - Play Again option (of play a specified number of games) (goto & lbl?)
  - implement "learning" ability
*/

#include <iostream>		// Does I/O stuff
#include <fstream>		// Allows file stuff TODO: output useful nodes to a .txt
#include <array>      // Allows creation and manipulation of arrays (mostly so I can use .size() & == on arrarys)
#include <vector>     // Allows use of vectors (and push_back)
#include <random>     // Allows use of subtract_with_carry_engine (fastest PRNG in C++11)
#include <chrono>     // Allows access to system clock

// Allows test cases
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

using namespace std;

// Define some class data structures
class c4Board {
    /*
    0 |  -1  -1  -1  -1  -1  -1  -1
    1 |  -1  -1  -1  -1  -1  -1  -1
    2 |  -1  -1  -1  -1  -1  -1  -1
    3 |  -1  -1  -1  -1  -1  -1  -1
    4 |  -1  -1  -1  -1  -1  -1  -1
    5 |  -1  -1  -1  -1  -1  -1  -1
    --|----------------------------
      |   0   1   2   3   4   5   6
    */
  public:
    array<array<int, 7>, 6> tileSpaces;   // [rows][columns] -> [0-5][0-6]

    c4Board *previousMove;    // Dereference pointer to previous node
    c4Board *nextMove;        // Dereference pointer to next node

    int playerJustMoved;    // The player who just played (1 is Red Player & 2 is Yellow) TODO: change this to currentPlayer
    bool endOfGame;         // Is true if no more moves are possible or necessary
    int winningPlayer;      // The player number of the player that won

    // Default Class Constructor (Only called initially)
    c4Board(){
      for (int i = 0; i <= 7; i++) {     // For each row
        for (int j = 0; j <= 6; j++) {    // For each column
          this->tileSpaces[i][j] = -1;      // write as default value
        }
      }

      // Initialize to default values
      this->previousMove = nullptr;
      this->nextMove = nullptr;
      this->playerJustMoved = -1;
      this->endOfGame = false;        // Default to false, only changed by gameOver()
    }

    // Class Constructor for creating a next Node (with link to previous)
    c4Board(c4Board* previousBoard, int playerNum){    // pointer to previous board, number of current player
      this->tileSpaces = previousBoard->tileSpaces;  // Copy

      this->previousMove = previousBoard;   // link previousMove to the given pointer
      this->nextMove = nullptr;             // Dereference the class pointer for next node
      previousBoard->nextMove = this;      // link nextMove of previousBoard to this

      this->playerJustMoved = playerNum;    // playerNum is the current player (of previous board), stored to playerJustMoved
      this->endOfGame = false;              // Default to false, only changed by gameOver()
      this->winningPlayer = -1;             // Will remain negative one until either Red or Yellow wins (does not change for Draw)
    }

    // Class Copy Constrcutor for creating a new instance from a given instance
    c4Board (const c4Board &givenBoard) {
      this->tileSpaces = givenBoard.tileSpaces;

      this->previousMove = givenBoard.previousMove;
      this->nextMove = givenBoard.nextMove;

      this->playerJustMoved = givenBoard.playerJustMoved;
      this->endOfGame = givenBoard.endOfGame;
    }

    friend ostream &operator<<(ostream &output, const c4Board& obj) {
      string rows = "012345";
      string columns = "0123456";
      for (int i = 0; i < obj.tileSpaces.size(); i++) {        // For each row
        output << rows[i] << " |";
        for (int j = 0; j < obj.tileSpaces[i].size(); j++) {   // For each column
          if (obj.tileSpaces[i][j] == -1) {
            output << setw(4) << ".";
          }
          else if (obj.tileSpaces[i][j] == 1) {
            output << setw(4) << "R";
          }
          else if (obj.tileSpaces[i][j] == 2) {
            output << setw(4) << "Y";
          }
        }
        output << endl;
      }
      output << "--|----------------------------" << endl << "  |";
      for (int i = 0; i < columns.length(); i++){
        output << setw(4) << columns[i];
      }
      output << endl;
      return output;
    }

    // TODO: does c4Board need this or just Node?
    friend bool operator==(const c4Board& lhs, const c4Board& rhs) {       // p1.operator==(p2)
      return lhs.tileSpaces == rhs.tileSpaces;
    }

    c4Board dropToken(int colNum) {
      /*
      Returns a new instance of c4Board with the latest player's move and a pointer make to the previous state
      Only allows legal moves (column selection and tokens in a column)
      */
      bool validMove = false;     // Changes if a valid move is found
      int currentPlayer;
      if (playerJustMoved == -1){
        currentPlayer = 1;
      }
      else {
          currentPlayer = 3 - this->playerJustMoved;   // Get the number of the player currently playing
      }

      c4Board tmp(this, currentPlayer);   // Create a new board with a pointer back to the current state and the currentPlayer

      // Iterate through tileSpaces[i][colNum] backwards until tileSpaces[i][i] == -1, then tileSpaces[rowNum][colNum] = currentPlayer
      for (int i = 5; i >= 0; i--) {   // Starting at the bottom of the row and working to the top
        //cout << i << ", " << colNum << ": " << tmp.tileSpaces[i][colNum] << endl;   // For test purposes
        if (tmp.tileSpaces[i][colNum] == -1) {   // If the current tile has no token yet
          tmp.tileSpaces[i][colNum] = currentPlayer;
          validMove = true;
          break;
        }
      }
      if (!validMove) {
        cout << "This column is already full. Choose a different move" << endl;
        return *this;   // Return the same instance the function was called from
      }

      return tmp;
    }

    bool continuePlaying() {
      /* Returns "false" to terminate game if winning move is detected, otherwise returns "true" if available moves are left */

      // If there are four vertical matching tokens
      for (int i = 0; i + 3 < this->tileSpaces.size(); i++) {     // For each row
        for (int j = 0; j < this->tileSpaces[i].size(); j++) {      // For each column
          // cout << "i: " << i << ", j: " << j << endl;  // For test purposes
          // cout << this->tileSpaces[i][j] << " " << this->tileSpaces[i + 1][j] << " " << this->tileSpaces[i + 2][j] << " " << this->tileSpaces[i + 3][j] << endl;
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i + 1][j] && this->tileSpaces[i][j] == this->tileSpaces[i + 2][j] && this->tileSpaces[i][j] == this->tileSpaces[i + 3][j]){
            this->winningPlayer = this->tileSpaces[i][j];
            return false;
          }
        }
      }

      // If there are four horizontal matching tokens
      for (int i = 0; i < this->tileSpaces.size(); i++) {     // For each row
        for (int j = 0; j + 3 < this->tileSpaces[i].size(); j++) {      // For each column
          // cout << "i: " << i << ", j: " << j << endl;  // For test purposes
          // cout << this->tileSpaces[i][j] << " " << this->tileSpaces[i][j + 1] << " " << this->tileSpaces[i][j + 2] << " " << this->tileSpaces[i][j + 3] << endl;
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i][j + 1] && this->tileSpaces[i][j] == this->tileSpaces[i][j + 2] && this->tileSpaces[i][j] == this->tileSpaces[i][j + 3]){
            this->winningPlayer = this->tileSpaces[i][j];
            return false;
          }
        }
      }

      // If there are four diagonal matching tokens L->R descending
      for (int i = 0; i + 3 < this->tileSpaces.size(); i++) {     // For each row
        for (int j = 0; j + 3 < this->tileSpaces[i].size(); j++) {      // For each column
          // cout << "i: " << i << ", j: " << j << endl;  // For test purposes
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i + 1][j + 1] && this->tileSpaces[i][j] == this->tileSpaces[i + 2][j + 2] && this->tileSpaces[i][j] == this->tileSpaces[i + 3][j + 3]){
            this->winningPlayer = this->tileSpaces[i][j];
            return false;
          }
        }
      }

      // If there are four diagonal matching tokens R->L descending
      for (int i = 0; i + 3 < this->tileSpaces.size(); i++) {     // For each row
        for (int j = tileSpaces[i].size(); j >= 3; j--) {      // For each column
          // cout << "i: " << i << ", j: " << j << endl;  // For test purposes
          // cout << this->tileSpaces[i][j] << " " << this->tileSpaces[i + 1][j - 1] << " " << this->tileSpaces[i + 2][j - 2] << " " << this->tileSpaces[i + 3][j - 3] << endl;
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i + 1][j - 1] && this->tileSpaces[i][j] == this->tileSpaces[i + 2][j - 2] && this->tileSpaces[i][j] == this->tileSpaces[i + 3][j - 3]){
            this->winningPlayer = this->tileSpaces[i][j];
            return false;
          }
        }
      }

      // If any empty spaces remain in the board
      for (int i = 0; i < this->tileSpaces.size(); i++) {        // For each row
        for (int j = 0; j < this->tileSpaces[i].size(); j++) {   // For each column
          if (this->tileSpaces[i][j] == -1){  // This will only execute if there are valid moves left to make
            return true;
          }
        }
      }

      // If none of the above returns are triggered
      return false;
    }
  };

class Node {
  /*
  Some member functions:
  makeMove()- chooses the best move to make by creating a test node (keeps track of the results for each possible move)
  X isPossible()- returns true if a token can be dropped in a given column
  X getChildren()- creates a new node based on a token dropped in the given column
  X getGameState()- returns -1 for a game in progress, 1 is player 1 has won, 2 if player 2 has won, and 3 if the game is a draw
  sampleNodePath()- randomly makes moves, checking each new node for final state by calling getGameState()
  movesLeft()- Returns the number of empty spaces still available on the board (used to calculate how many child nodes there should be)
  */
  public:
    int ni;   // Accumulator for number of nodes sampled (only count final gameStates)
    int wi;   // Accumulator for number of nodes that are wins (wi/ni is likelihood of winning)
    int di;   // Accumulator for number of nodes that are draws

    // Store a copy of the current game state
    array<array<int, 7>, 6> tileSpaces;   // [rows][columns]
    int playerJustMoved;      // The player who just played (1 is Red Player & 2 is Yellow)
    int winningPlayer;        // The player number of the player that won

    // Pointers for previous state and each possble next state
    Node *previousBoard;     // Should be nullptr when initially constructed (root is always the move under consideration)
    Node *nextMove1;
    Node *nextMove2;
    Node *nextMove3;

    // Default constructor
    Node() {
      // Fill board with placeholder values
      for (int i = 0; i < this->tileSpaces.size(); i++){
        for (int j = 0; j < this->tileSpaces[i].size(); j++){
          this->tileSpaces[i][j] = -1;
        }
      }

      // Initialize accumulators tp initial value (0)
      this->ni = 0;
      this->wi = 0;
      this->di = 0;

      // Set to placeholder values
      this->playerJustMoved = -1;
      this->winningPlayer = -1;

      // Set pointers to null value
      this->previousBoard = nullptr;
      this->nextMove1 = nullptr;
      this->nextMove2 = nullptr;
      this->nextMove3 = nullptr;
    }

    // Constructor called when an instance of Node is created from an instance of c4Board
    Node(c4Board currentBoard) {
      // Initialize accumulators to initial value (0)
      this->ni = 0;
      this->wi = 0;
      this->di = 0;

      // Copy data from currentBoard
      this->tileSpaces = currentBoard.tileSpaces;
      this->playerJustMoved = currentBoard.playerJustMoved;  // MCTS finds best possible move against this player
      this->winningPlayer = currentBoard.winningPlayer;      // Should be -1

      // Node being constructed is root, so previousBoard = nullptr
      this->previousBoard = nullptr;

      // Initialize to default values
      this->nextMove1 = nullptr;
      this->nextMove2 = nullptr;
      this->nextMove3 = nullptr;
    }

    // Constructor for creating children nodes
    Node(Node* currentBoard, int playerNum) {
      // Initialize accumulators to initial value (0)
      this->ni = 0;
      this->wi = 0;
      this->di = 0;

      // Copy data from currentBoard
      this->tileSpaces = currentBoard->tileSpaces;
      this->playerJustMoved = playerNum;
      this->winningPlayer = currentBoard->winningPlayer;      // Should be -1

      // Link to Node of previous game board
      this->previousBoard = currentBoard;

      // Initialize to default values
      this->nextMove1 = nullptr;
      this->nextMove2 = nullptr;
      this->nextMove3 = nullptr;
    }

    // TODO Class constructor or operator>> overload (for input from text file)
    // friend ifstream &operator>>(ifstream &input, Date& t){
    //   return input;
    // }

    friend ostream &operator<<(ostream &output, const Node& obj){
      string rows = "012345";
      string columns = "0123456";
      for (int i = 0; i < obj.tileSpaces.size(); i++) {        // For each row
        output << rows[i] << " |";
        for (int j = 0; j < obj.tileSpaces[i].size(); j++) {   // For each column
          if (obj.tileSpaces[i][j] == -1) {
            output << setw(4) << ".";
          }
          else if (obj.tileSpaces[i][j] == 1) {
            output << setw(4) << "R";
          }
          else if (obj.tileSpaces[i][j] == 2) {
            output << setw(4) << "Y";
          }
        }
        output << endl;
      }
      output << "--|----------------------------" << endl << "  |";
      for (int i = 0; i < columns.length(); i++){
        output << setw(4) << columns[i];
      }
      output << endl;
      return output;
    }

    bool operator==(const Node& rhs) {        // p1.operator==(p2)
      return this->tileSpaces == rhs.tileSpaces;
    }

    bool operator!=(const Node& rhs) {
      return not (this == &rhs);
    }

    Node& operator= (const Node& rhs) {       // p1 = p2
      this->ni = rhs.ni;
      this->wi = rhs.wi;
      this->di = rhs.di;

      // Copy data from currentBoard
      this->tileSpaces = rhs.tileSpaces;
      this->playerJustMoved = rhs.playerJustMoved;
      this->winningPlayer = rhs.winningPlayer;      // Should be -1

      // Link to Node of previous game board
      this->previousBoard = rhs.previousBoard;

      return *this;
    }

    // Creates a node that is the result of a move made in currentNode (provides linking)
    Node getChildNode(int colNum) {
      /*
      Returns a Node with the most recent move (and a pointer to previous boards)
      */
      int currentPlayer;
      if (playerJustMoved == -1){
        currentPlayer = 1;
      }
      else {
          currentPlayer = 3 - this->playerJustMoved;   // Get the number of the player currently playing
      }

      Node child(this, currentPlayer);   // Create a new board with a pointer back to the current state and the currentPlayer

      // Figure out which child is being created
      if (this->nextMove1 == nullptr){
        this->nextMove1 = &child;
      }
      else if (this->nextMove2 == nullptr){
        this->nextMove2 = &child;
      }
      else if (this->nextMove3 == nullptr){
        this->nextMove3 = &child;
      }

      // Iterate through tileSpaces[i][colNum] backwards until tileSpaces[i][i] == -1, then tileSpaces[rowNum][colNum] = currentPlayer
      for (int i = 5; i >= 0; i--) {   // Starting at the bottom of the row and working to the top
        if (child.tileSpaces[i][colNum] == -1) {   // If the current tile has no token yet
          child.tileSpaces[i][colNum] = currentPlayer;
          break;  // Break out of loop when move has been made
        }
      }
      //cout << child;  // For test purposes
      return child;
    }

    // Checks the validity of making a move in a given column
    bool isPossible(int colNum){    // currentBoard.isPossible(int)
      /*
      Returns True the specified column still has empty spaces, returns False if it is full
      */
      for (int i = 0; i < this->tileSpaces.size(); i++){      // For each row
        if (this->tileSpaces[i][colNum] == -1){   // Executes as soon as an available move is found
          return true;
        }
      }

      return false;
    }

    // Returns a number indicating state of game (win/lose/draw/in play)
    int getGameState(){    // currentBoard.getGameState()
      /*
      Possible Return Values:
      -1: game in progress, no winner yet
      1: Red has won!
      2: Yellow has won!
      3: Game is a draw.
      */

      // If there are four vertical tokens
      for (int i = 0; i + 3 < this->tileSpaces.size(); i++) {     // For each row
        for (int j = 0; j < this->tileSpaces[i].size(); j++) {      // For each column
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i + 1][j] && this->tileSpaces[i][j] == this->tileSpaces[i + 2][j] && this->tileSpaces[i][j] == this->tileSpaces[i + 3][j]){
            return this->tileSpaces[i][j];
          }
        }
      }

      // If there are four horizontal tokens
      for (int i = 0; i < this->tileSpaces.size(); i++) {     // For each row
        for (int j = 0; j + 3 < this->tileSpaces[i].size(); j++) {      // For each column
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i][j + 1] && this->tileSpaces[i][j] == this->tileSpaces[i][j + 2] && this->tileSpaces[i][j] == this->tileSpaces[i][j + 3]){
            return this->tileSpaces[i][j];
          }
        }
      }

      // If there are four diagonal tokens L->R descending
      for (int i = 0; i + 3 < this->tileSpaces.size(); i++) {     // For each row
        for (int j = 0; j + 3 < this->tileSpaces[i].size(); j++) {      // For each column
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i + 1][j + 1] && this->tileSpaces[i][j] == this->tileSpaces[i + 2][j + 2] && this->tileSpaces[i][j] == this->tileSpaces[i + 3][j + 3]){
            return this->tileSpaces[i][j];
          }
        }
      }

      // If there are four diagonal tokens R->L descending
      for (int i = 0; i + 3 < this->tileSpaces.size(); i++) {     // For each row
        for (int j = tileSpaces[i].size(); j >= 3; j--) {           // For each column
          if (this->tileSpaces[i][j] != -1 && this->tileSpaces[i][j] == this->tileSpaces[i + 1][j - 1] && this->tileSpaces[i][j] == this->tileSpaces[i + 2][j - 2] && this->tileSpaces[i][j] == this->tileSpaces[i + 3][j - 3]){
            return this->tileSpaces[i][j];
          }
        }
      }

      // If board an empty spaces remain in the board
      for (int i = 0; i < this->tileSpaces.size(); i++) {        // For each row
        for (int j = 0; j < this->tileSpaces[i].size(); j++) {   // For each column
          if (this->tileSpaces[i][j] == -1){  // This will only execute if there are any valid moves left to make
            return -1;
          }
        }
      }

      // If none of the above returns are triggered
      return 3;
    }

    // Return the number of empty spaces in game board
    int movesLeft(){
      /*
      If movesLeft = 0, node is fully mapped.
      If ni = fact(movesLeft), node is fully mapped. (or some similar expression)
      */
      int acc = 0;    // Initialize accumulator to 0

      for (int i = 0; i < this->tileSpaces.size(); i++) {   // For each row
        for (int j = 0; i < tileSpaces[i].size(); j++) {      // For each column
          if (tileSpaces[i][j] == -1){      // If space is empty...
            acc ++;   // ...increment accumulator.
          }
        }
      }
      return acc;   // Return accumulator
    }

    // Randomly play through game specified number of times, updating win, loss, or draw for the instance of Node the function is called from;
    void sampleNodePath(int numSearches) {
      /*
      1. For the number of times specified, randomly select a number 0-6
      2. Created a new state based on that move (if possible)
      3. Check that state for w/l/d (by calling getGameState)
      4. Update root node's (and children) ni, and wi, li, di as necessary
      5. Check if the Node has been fully mapped yet (TODO: compare against .txt file)
      5. If current node is a final state, it has been fully mapped. TODO: Write to .txt file
      6. Go up a level until in the tree until a Node is found that hasn't been fully mapped. Else, repeat process
      */

      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();   // Use the current time to seed the psuedo-random number generator
      subtract_with_carry_engine<unsigned,24,10,24> generator (seed);

      int currentPlayer;
      if (playerJustMoved == -1){
        currentPlayer = 1;
      }
      else {
          currentPlayer = 3 - this->playerJustMoved;   // Get the number of the player currently playing
      }

      Node tmp;   // Creates default object (values are assigned later)
      int results = tmp.getGameState();   // Store the results of the gameState check

      // Do random Playthroughs numSearches number of times
      for (int i = 0; i <= numSearches; i++){
        tmp = *this;   // Give tmp the same starting paramters as current instance
        while (results == -1){   // While the game is in progress
          int colNum = generator()%7;   // Choose a random number between 0-6
          if (tmp.isPossible(colNum)){    // Check if that move is possible
              tmp = tmp.getChildNode(colNum);   // Update tmp to be new state
              results = tmp.getGameState();
          }
        }

        // Check new game state and increment accumulators as needed
        this->ni ++;    // A new possible endgame has been found
        if (results == 1 && tmp.playerJustMoved == 1) {     // Red wins!
          this->wi ++;  // A winning move has been found
        }

        else if (results == 2 && tmp.playerJustMoved == 2){   // Yellow wins!
            this->wi ++;  // A winning move has been found
          }
        else if (results == 3) {
          this->di ++;  // final state of this path was a draw
        }
      }
      // // For test purposes
      // cout << "Nodes searched: " << this->ni << endl;
      // cout << "Probability of winning: " << this->wi / this->ni << endl;
      // cout << "Number of winning Nodes: " << this->wi << endl;
      return;
    }

    // Plays through a sample game for each possible move
    int makeMove(){
      /*
      1. Check if a move is possible in each column
      2. If a move is possible, create a child Node for that move
      3. Do a Lightweight Playthrough of the child Node
      4. Return the number of the child Node with the best move
      */

      array<array<int, 7>, 6> emptyBoard;  // For comparison purposes
      for (int i = 0; i < emptyBoard.size(); i++){
        for (int j = 0; j < emptyBoard[i].size(); j++){
          emptyBoard[i][j] = -1;
        }
      }

      // If board is empty (this is first move), go middle column (proven to be the best choice)
      if (this->tileSpaces == emptyBoard){
        return 3;
      }

      // For all other moves
      else{
        vector<Node> childrenNodes;   // Create a list to keep track of each child Node
        Node placeholder;   // Default state for Node (used as placeholder in vectors and stuff)

        // Generate childrenNodes for all possible moves
        for (int i = 0; i < 7; i++){   // For each column
          if (this->isPossible(i)){    // Check if a move is possble
            Node tmp;   // Create a temporary Node obeject
            tmp = *this;    // Give tmp the same starting paramters as current instance
            tmp = tmp.getChildNode(i);   // Convert tmp into a child node
            childrenNodes.push_back(tmp);   // Append tmp to the list of child nodes

            // Sample child Node
            childrenNodes[i].sampleNodePath(50);    // Updates wi for each node

            // Update root node's accumulators
            this->ni += childrenNodes[i].ni;
            this->wi += childrenNodes[i].wi;
            this->di += childrenNodes[i].di;
          }
          else{
            childrenNodes.push_back(placeholder);  // If a move wasn't possible, give it a placeholder value since index of node is column of move
          }
        }

        // Find the best move based on Node sampling
        Node bestNode;  // Used to keep track of which child Node produced the best results
        int bestMove;   // Used to keep track of which column is the best move

        // Set bestNode and bestMove to first possible move from list
        for (int i = 0; i < childrenNodes.size(); i++){
          // cout << "childrenNodes[i] != placeholder: " << (childrenNodes[i] != placeholder) << endl;   // For test purposes
          if (this->isPossible(i) && childrenNodes[i] != placeholder){   // If child Node is both possible and not a placeholder
            bestNode = childrenNodes[i];
            bestMove = i;
          }
        }

        // See if a better move is possible
        for (int i = 0; i < childrenNodes.size(); i++){
          if (childrenNodes[i].wi > bestNode.wi){
            bestNode = childrenNodes[i];
            bestMove = i;
          }
        }

        cout << "Estimated number of wins: " << this->wi << endl;
        cout << "Probability of winning: " << static_cast<double>(this->wi) / static_cast<double>(this->ni) << endl;
        return bestMove;    // If the recommended is possible, make it
    }
  }
};

// Main
int main() {
  int result = (new doctest::Context())->run();     // used for DocTest

  ofstream usefulNodes;   // Create a filestream to read and write nodes from/to
  usefulNodes.open("nodes.txt");    // Open the file (open and closed in main, but used by MCTS)

  bool keepPlaying = true;  // Used to play again

  while(keepPlaying){       // Allows the player to play multiple times
    // Define some stuff
    bool playingGame = true;  // Set to false when game is over
    char playAgain;   // User's selection

    c4Board currentBoard;   // Create an instance of c4Board (initializes to default state)

    while(playingGame){          // Loop continues until game is over, one way or another
      if (currentBoard.playerJustMoved == -1){        // Only occurs for new game
        cout << "Red's Turn" << endl;
      }
      else if (currentBoard.playerJustMoved == 2){    // Yellow just played
        cout << "Red's Turn" << endl;
      }
      else if (currentBoard.playerJustMoved == 1){    // Red just played
        cout << "Yellow's Turn" << endl;
      }

      // MCTS is implemented here
      Node currentNode(currentBoard);   // Create a new node tree with currentBoard as root
      int AIChoice = currentNode.makeMove();
      cout << "Selected move: " << AIChoice << endl;
      currentBoard = currentBoard.dropToken(AIChoice);        // Gives MCTS control of board

      // // This block of code gives the user control of board
      // cout << "Select a Column Number in which to drop your token: ";
      // cin >> playerChoice;  // Change this line for MCTS input (or make conditional for PvC)
      // currentBoard = currentBoard.dropToken(playerChoice);          // Create a new board based on player's choice

      cout << "Current Board: " << endl << currentBoard;
      playingGame = currentBoard.continuePlaying();    // continuePlaying() returns false if someone has won
    }

    // This bit only executes after the game is over
    if (currentBoard.winningPlayer == 1){
      cout << "Red has won!" << endl;
    }
    else if (currentBoard.winningPlayer == 2){
      cout << "Yellow has won!" << endl;
    }
    else if (currentBoard.winningPlayer == -1){
      cout << "The game is a draw." << endl;
    }

    // Modify this bit to change repeat functionality
    cout << "Play Again? Y/n: ";
    cin >> playAgain;

    if (playAgain == 'y' || playAgain == 'Y'){
      keepPlaying = true;
    }
    else if (playAgain == 'n' || playAgain == 'N'){
      keepPlaying = false;
    }
    else{
      keepPlaying = true;
    }

  }
    usefulNodes.close();  // Close ofstream
    return result;        // Returns the DocTest test cases
  }

TEST_CASE("c4Board Class Tests") {
  c4Board testBoard;
  CHECK(testBoard.previousMove == nullptr);   // Checks that the default class constructor defualts to nullptr
  CHECK(testBoard.nextMove == nullptr);       // Checks that the default class constructor defualts to nullptr
  CHECK(testBoard.playerJustMoved == -1);
  CHECK(testBoard.endOfGame == false);
}

// TEST_CASE("Test Playthrough") {
//   CHECK()
// }
