#ifndef SUDOKU_H
#define SUDOKU_H

const bool DEBUG_MODE = false;
enum { ROW=9, COL=9, N = 81, NEIGHBOR = 20 };
const int NUM = 9;
const int TaskNum=5000000;

extern int neighbors[N][NEIGHBOR];

void init_neighbors();
void input(const char in[N],int board[N],int spaces[N],int nspaces);
bool solved(int board[N]);
bool solved();

bool available(int guess, int cell);

bool solve_sudoku_dancing_links(int unused,int board[N],int spaces[N],int nspaces);
#endif
