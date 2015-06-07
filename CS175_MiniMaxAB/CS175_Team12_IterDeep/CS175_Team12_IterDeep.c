/*______________________________________________________________________________

----------> name: simple checkers with enhancements
----------> author: martin fierz
----------> purpose: platform independent checkers engine
----------> version: 1.15
----------> date: 4th february 2011
----------> description: simplech.c contains a simple but fast checkers engine
and some routines to interface to checkerboard. simplech.c contains three
main parts: interface, search and move generation. these parts are
separated in the code.

/*----------> includes */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#define OCCUPIED 0
#define WHITE 1
#define BLACK 2
#define MAN 4
#define KING 8
#define FREE 16
#define CHANGECOLOR 3
#define MAXDEPTH 21
#define MAXMOVES 28
#define KING_WEIGHT 15
#define PIECE_WEIGHT 10

/*----------> compile options  */
#undef MUTE
#undef VERBOSE
#define STATISTICS


/* return values */
#define DRAW 0
#define WIN 1
#define LOSS 2
#define UNKNOWN 3

/*----------> structure definitions  */
struct move2
{
	short n;
	int m[8];
};
struct coor             /* coordinate structure for board coordinates */
{
	int x;
	int y;
};

struct CBmove            	/* all the information you need about a move */
{
	int ismove;          /* kind of superfluous: is 0 if the move is not a valid move */
	int newpiece;        /* what type of piece appears on to */
	int oldpiece;        /* what disappears on from */
	struct coor from, to; /* coordinates of the piece - in 8x8 notation!*/
	struct coor path[12]; /* intermediate path coordinates of the moving piece */
	struct coor del[12]; /* squares whose pieces are deleted after the move */
	int delpiece[12];    /* what is on these squares */
} GCBmove;
/*----------> function prototypes  */
/*----------> part I: interface to CheckerBoard: CheckerBoard requires that
at getmove and enginename are present in the dll. the
functions help, options and about are optional. if you
do not provide them, CheckerBoard will display a
MessageBox stating that this option is in fact not an option*/

int comparisons;
clock_t start;
int passedMaxTime(double maxTime);
int locationVal[46] = { 0, 0, 0, 0, 0, 4, 4, 4, 4, 0,
3, 3, 3, 4, 4, 2, 2, 3, 0, 3,
1, 2, 4, 4, 2, 1, 3, 0, 3, 2,
2, 4, 4, 3, 3, 3, 0, 4, 4, 4,
4, 0, 0, 0, 0, 0 };

/* required functions */
int  WINAPI getmove(int b[8][8], int color, double maxtime, char str[255], int *playnow, int info, int unused, struct CBmove *move);

/*----------> part II: search */
int  checkers(int b[46], int color, double maxtime, char *str);
void doMove(int b[46], struct move2 move);
void undoMove(int b[46], struct move2 move);
int masterIterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str);
int iterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str, int depth, int quit[1]);

/*----------> part III: move generation */

int  generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color);
int  generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color);
void blackmancapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int square);
void blackkingcapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int square);
void whitemancapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int square);
void whitekingcapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int square);
int  testcapture(int b[46], int color);

/*********** Heuristics */

int evaluation(int board[46]);
int numMen(int board[46], int color);
int numKings(int board[46], int color);
int totalPieces(int board[46], int color);
int piecePositionScore(int board[46], int color);

/*-------------- PART 1: dll stuff -------------------------------------------*/

BOOL WINAPI DllEntryPoint(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
	/* in a dll you used to have LibMain instead of WinMain in windows programs, or main
	in normal C programs
	win32 replaces LibMain with DllEntryPoint.*/

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		/* dll loaded. put initializations here! */
		break;
	case DLL_PROCESS_DETACH:
		/* program is unloading dll. put clean up here! */
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}

int WINAPI enginecommand(char str[256], char reply[256])
{
	// answer to commands sent by CheckerBoard.
	// Simple Checkers does not answer to some of the commands,
	// eg it has no engine options.

	char command[256], param1[256], param2[256];

	sscanf(str, "%s %s %s", command, param1, param2);

	// check for command keywords 

	if (strcmp(command, "name") == 0)
	{
		sprintf(reply, "CS 175 Team 12 - Iterative Deepening w\ Minimax and Quiescence Search");
		return 1;
	}

	if (strcmp(command, "about") == 0)
	{
		sprintf(reply, "Team 12 - Iterative Deepening with Minimax and Quiescence Search\n\n2015 CS 175");
		return 1;
	}

	if (strcmp(command, "help") == 0)
	{
		sprintf(reply, "?");
		return 1;
	}

	if (strcmp(command, "set") == 0)
	{
		if (strcmp(param1, "hashsize") == 0)
		{
			sprintf(reply, "?");
			return 0;
		}
		if (strcmp(param1, "book") == 0)
		{
			sprintf(reply, "?");
			return 0;
		}
	}

	if (strcmp(command, "get") == 0)
	{
		if (strcmp(param1, "hashsize") == 0)
		{
			sprintf(reply, "?");
			return 0;
		}
		if (strcmp(param1, "book") == 0)
		{
			sprintf(reply, "?");
			return 0;
		}
		if (strcmp(param1, "protocolversion") == 0)
		{
			sprintf(reply, "2");
			return 1;
		}
		if (strcmp(param1, "gametype") == 0)
		{
			sprintf(reply, "21");
			return 1;
		}
	}
	sprintf(reply, "?");
	return 0;
}

int WINAPI getmove(int b[8][8], int color, double maxtime, char str[255], int *playnow, int info, int unused, struct CBmove *move)
{
	int i, value;
	int board[46];
	comparisons = 0;
	// Initialize the board
	for (i = 0; i < 46; i++)
		board[i] = OCCUPIED;
	for (i = 5; i <= 40; i++)
		board[i] = FREE;

	// Save the actual board positions
	board[5] = b[0][0]; board[6] = b[2][0]; board[7] = b[4][0]; board[8] = b[6][0];
	board[10] = b[1][1]; board[11] = b[3][1]; board[12] = b[5][1]; board[13] = b[7][1];
	board[14] = b[0][2]; board[15] = b[2][2]; board[16] = b[4][2]; board[17] = b[6][2];
	board[19] = b[1][3]; board[20] = b[3][3]; board[21] = b[5][3]; board[22] = b[7][3];
	board[23] = b[0][4]; board[24] = b[2][4]; board[25] = b[4][4]; board[26] = b[6][4];
	board[28] = b[1][5]; board[29] = b[3][5]; board[30] = b[5][5]; board[31] = b[7][5];
	board[32] = b[0][6]; board[33] = b[2][6]; board[34] = b[4][6]; board[35] = b[6][6];
	board[37] = b[1][7]; board[38] = b[3][7]; board[39] = b[5][7]; board[40] = b[7][7];

	// Ensure the empty positions are free
	for (i = 5; i <= 40; i++)
	if (board[i] == 0)
		board[i] = FREE;
	for (i = 9; i <= 36; i += 9)
		board[i] = OCCUPIED;

	// Calculate move
	value = checkers(board, color, maxtime, str);

	// Return all free positions back to empty
	for (i = 5; i <= 40; i++)
	if (board[i] == FREE)
		board[i] = 0;

	// Return the adjusted board
	b[0][0] = board[5]; b[2][0] = board[6]; b[4][0] = board[7]; b[6][0] = board[8];
	b[1][1] = board[10]; b[3][1] = board[11]; b[5][1] = board[12]; b[7][1] = board[13];
	b[0][2] = board[14]; b[2][2] = board[15]; b[4][2] = board[16]; b[6][2] = board[17];
	b[1][3] = board[19]; b[3][3] = board[20]; b[5][3] = board[21]; b[7][3] = board[22];
	b[0][4] = board[23]; b[2][4] = board[24]; b[4][4] = board[25]; b[6][4] = board[26];
	b[1][5] = board[28]; b[3][5] = board[29]; b[5][5] = board[30]; b[7][5] = board[31];
	b[0][6] = board[32]; b[2][6] = board[33]; b[4][6] = board[34]; b[6][6] = board[35];
	b[1][7] = board[37]; b[3][7] = board[38]; b[5][7] = board[39]; b[7][7] = board[40];

	if (color == BLACK)
	{
		if (value > 0)
			return WIN;
		else if (value < 0)
			return LOSS;
	}
	else if (color == WHITE)
	{
		if (value > 0)
			return LOSS;
		else if (value < 0)
			return WIN;
	}

	return UNKNOWN;
}

/*-------------- PART II: SEARCH ---------------------------------------------*/


int  checkers(int b[46], int color, double maxtime, char *str)
{
	int eval, numOfMoves;

	start = clock();
	eval = masterIterativeDeepening(b, color, maxtime, start, str);

	return eval;
}

int masterIterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str)
{
	int nextDepth;
	int quitEarly[1] = { 0 };
	int eval = 0, value = 0, bestVal = -1, currentBestMove = 0, savedBestMove = 0;
	struct move2 best, completedBest;
	struct move2 movesList[MAXMOVES];
	int moveCount;

	// For all depths
	for (int currentDepth = 1; currentDepth < MAXDEPTH && quitEarly[0] == 0; currentDepth++)
	{
		// get the list of all capture moves
		moveCount = generatecapturelist(b, movesList, color);
		nextDepth = currentDepth;

		// If we don't have any forced captures, check for regular moves
		if (moveCount == 0)
		{
			moveCount = generatemovelist(b, movesList, color);
			--nextDepth;
		}

		// if we can't make any moves
		if (moveCount == 0)
		{
			// return that we lost
			if (color == BLACK)
			{
				return -1000;
			}
			else if (color == WHITE)
			{
				return 1000;
			}
		}
		else if (moveCount == 1)
		{
			sprintf(str, "Forced Move");
			best = movesList[0];
			quitEarly[0] = 1;
		}

		// for each move
		for (int i = 0; i < moveCount && quitEarly[0] == 0; i++)
		{
			// do the move
			doMove(b, movesList[i]);
			// get the value of the move
			value = iterativeDeepening(b, color^CHANGECOLOR, maxtime, start, str, nextDepth, quitEarly);
			// undo the move
			undoMove(b, movesList[i]);

			// If its the first move, save the best value
			if (i == 0)
			{
				bestVal = value;
				best = movesList[i];
				currentBestMove = 1;
			}
			else
			{
				// Check color
				if (color == BLACK)
				{
					if (value > bestVal)
					{
						bestVal = value;
						best = movesList[i];
						currentBestMove = i + 1;
					}
				}
				else if (color == WHITE)
				{
					if (value < bestVal)
					{
						bestVal = value;
						best = movesList[i];
						currentBestMove = i + 1;
					}
				}
			}
			comparisons++;
			char printColor[5];
			if (color == WHITE)
				sprintf(printColor, "White");
			else
				sprintf(printColor, "Black");
			sprintf(str, "MASTER %s - Comparisons: %i Depth: %i Move Count: %i Current Move: %i Value: %i BestVal: %i Saved Best: %i", printColor, comparisons, currentDepth, moveCount, (i + 1), value, bestVal, savedBestMove);
		}

		if (quitEarly[0] == 0 || currentDepth == 1)
		{
			savedBestMove = currentBestMove;
			completedBest = best;
		}
	}
	doMove(b, completedBest);

	return eval;
}

int iterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str, int depth, int quit[1])
{
	if (depth == 0)
		return evaluation(b);

	int value, bestVal = 0, moveCount, currentDepth = depth;
	struct move2 movesList[MAXMOVES];

	moveCount = generatecapturelist(b, movesList, color);

	if (moveCount == 0)
	{
		// generate all possible moves at this depth
		moveCount = generatemovelist(b, movesList, color);
		--currentDepth;
	}

	// If we can't make a move because we lost or we're "trapped"
	if (moveCount == 0)
	{
		if (color == BLACK)
		{
			return -1000;
		}
		else if (color == WHITE)
		{
			return 1000;
		}
	}

	// for each move
	for (int i = 0; i < moveCount && quit[0] == 0; i++)
	{
		doMove(b, movesList[i]);
		value = iterativeDeepening(b, color^CHANGECOLOR, maxtime, start, str, currentDepth, quit);
		undoMove(b, movesList[i]);
		comparisons++;

		if (i == 0)
			bestVal = value;
		else
		{
			if (color == BLACK)
			{
				if (value > bestVal)
					bestVal = value;
			}
			else if (color == WHITE)
			{
				if (value < bestVal)
					bestVal = value;
			}
		}

		if (passedMaxTime(maxtime) == 1)
		{
			quit[0] = 1;
			return bestVal;
		}
	}
	return bestVal;
}

void doMove(int b[46], struct move2 move)
/*----------> purpose: execute move on board
----------> version: 1.1
----------> date: 25th october 97 */
{
	int square, after;
	int i;

	for (i = 0; i<move.n; i++)
	{
		square = (move.m[i] % 256);
		after = ((move.m[i] >> 16) % 256);
		b[square] = after;
	}
}

void undoMove(int b[46], struct move2 move)
/*----------> purpose:
----------> version: 1.1
----------> date: 25th october 97 */
{
	int square, before;
	int i;

	for (i = 0; i<move.n; i++)
	{
		square = (move.m[i] % 256);
		before = ((move.m[i] >> 8) % 256);
		b[square] = before;
	}
}

/*-------------- PART III: MOVE GENERATION -----------------------------------*/

int generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color)
/*----------> purpose:generates all moves. no captures. returns number of moves
----------> version: 1.0
----------> date: 25th october 97 */
{
	int n = 0, m;
	int i;

	if (color == BLACK)
	{
		for (i = 5; i <= 40; i++)
		{
			if ((b[i] & BLACK) != 0)
			{
				if ((b[i] & MAN) != 0)
				{
					if ((b[i + 4] & FREE) != 0)
					{
						movelist[n].n = 2;
						if (i >= 32) m = (BLACK | KING); else m = (BLACK | MAN); m = m << 8;
						m += FREE; m = m << 8;
						m += i + 4;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (BLACK | MAN); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i + 5] & FREE) != 0)
					{
						movelist[n].n = 2;
						if (i >= 32) m = (BLACK | KING); else m = (BLACK | MAN); m = m << 8;
						m += FREE; m = m << 8;
						m += i + 5;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (BLACK | MAN); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
				}
				if ((b[i] & KING) != 0)
				{
					if ((b[i + 4] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (BLACK | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i + 4;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (BLACK | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i + 5] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (BLACK | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i + 5;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (BLACK | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i - 4] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (BLACK | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i - 4;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (BLACK | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i - 5] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (BLACK | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i - 5;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (BLACK | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
				}
			}
		}
	}
	else    /* color = WHITE */
	{
		for (i = 5; i <= 40; i++)
		{
			if ((b[i] & WHITE) != 0)
			{
				if ((b[i] & MAN) != 0)
				{
					if ((b[i - 4] & FREE) != 0)
					{
						movelist[n].n = 2;
						if (i <= 13) m = (WHITE | KING); else m = (WHITE | MAN); m = m << 8;
						m += FREE; m = m << 8;
						m += i - 4;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (WHITE | MAN); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i - 5] & FREE) != 0)
					{
						movelist[n].n = 2;
						if (i <= 13) m = (WHITE | KING); else m = (WHITE | MAN); m = m << 8;
						m += FREE; m = m << 8;
						m += i - 5;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (WHITE | MAN); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
				}
				if ((b[i] & KING) != 0)  /* or else */
				{
					if ((b[i + 4] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (WHITE | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i + 4;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (WHITE | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i + 5] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (WHITE | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i + 5;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (WHITE | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i - 4] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (WHITE | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i - 4;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (WHITE | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
					if ((b[i - 5] & FREE) != 0)
					{
						movelist[n].n = 2;
						m = (WHITE | KING); m = m << 8;
						m += FREE; m = m << 8;
						m += i - 5;
						movelist[n].m[1] = m;
						m = FREE; m = m << 8;
						m += (WHITE | KING); m = m << 8;
						m += i;
						movelist[n].m[0] = m;
						n++;
					}
				}
			}
		}
	}
	return(n);
}

int  generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color)
/*----------> purpose: generate all possible captures
----------> version: 1.0
----------> date: 25th october 97 */
{
	int n = 0;
	int m;
	int i;
	int tmp;

	if (color == BLACK)
	{
		for (i = 5; i <= 40; i++)
		{
			if ((b[i] & BLACK) != 0)
			{
				if ((b[i] & MAN) != 0)
				{
					if ((b[i + 4] & WHITE) != 0)
					{
						if ((b[i + 8] & FREE) != 0)
						{
							movelist[n].n = 3;
							if (i >= 28) m = (BLACK | KING); else m = (BLACK | MAN); m = m << 8;
							m += FREE; m = m << 8;
							m += i + 8;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (BLACK | MAN); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i + 4]; m = m << 8;
							m += i + 4;
							movelist[n].m[2] = m;
							blackmancapture(b, &n, movelist, i + 8);
						}
					}
					if ((b[i + 5] & WHITE) != 0)
					{
						if ((b[i + 10] & FREE) != 0)
						{
							movelist[n].n = 3;
							if (i >= 28) m = (BLACK | KING); else m = (BLACK | MAN); m = m << 8;
							m += FREE; m = m << 8;
							m += i + 10;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (BLACK | MAN); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i + 5]; m = m << 8;
							m += i + 5;
							movelist[n].m[2] = m;
							blackmancapture(b, &n, movelist, i + 10);
						}
					}
				}
				else /* b[i] is a KING */
				{
					if ((b[i + 4] & WHITE) != 0)
					{
						if ((b[i + 8] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (BLACK | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i + 8;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (BLACK | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i + 4]; m = m << 8;
							m += i + 4;
							movelist[n].m[2] = m;
							tmp = b[i + 4];
							b[i + 4] = FREE;
							b[i] = FREE;
							blackkingcapture(b, &n, movelist, i + 8);
							b[i + 4] = tmp;
							b[i] = BLACK | KING;
						}
					}
					if ((b[i + 5] & WHITE) != 0)
					{
						if ((b[i + 10] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (BLACK | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i + 10;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (BLACK | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i + 5]; m = m << 8;
							m += i + 5;
							movelist[n].m[2] = m;
							tmp = b[i + 5];
							b[i + 5] = FREE;
							b[i] = FREE;
							blackkingcapture(b, &n, movelist, i + 10);
							b[i + 5] = tmp;
							b[i] = BLACK | KING;
						}
					}
					if ((b[i - 4] & WHITE) != 0)
					{
						if ((b[i - 8] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (BLACK | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i - 8;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (BLACK | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i - 4]; m = m << 8;
							m += i - 4;
							movelist[n].m[2] = m;
							tmp = b[i - 4];
							b[i - 4] = FREE;
							b[i] = FREE;
							blackkingcapture(b, &n, movelist, i - 8);
							b[i - 4] = tmp;
							b[i] = BLACK | KING;
						}
					}
					if ((b[i - 5] & WHITE) != 0)
					{
						if ((b[i - 10] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (BLACK | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i - 10;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (BLACK | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i - 5]; m = m << 8;
							m += i - 5;
							movelist[n].m[2] = m;
							tmp = b[i - 5];
							b[i - 5] = FREE;
							b[i] = FREE;
							blackkingcapture(b, &n, movelist, i - 10);
							b[i - 5] = tmp;
							b[i] = BLACK | KING;
						}
					}
				}
			}
		}
	}
	else /* color is WHITE */
	{
		for (i = 5; i <= 40; i++)
		{
			if ((b[i] & WHITE) != 0)
			{
				if ((b[i] & MAN) != 0)
				{
					if ((b[i - 4] & BLACK) != 0)
					{
						if ((b[i - 8] & FREE) != 0)
						{
							movelist[n].n = 3;
							if (i <= 17) m = (WHITE | KING); else m = (WHITE | MAN); m = m << 8;
							m += FREE; m = m << 8;
							m += i - 8;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (WHITE | MAN); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i - 4]; m = m << 8;
							m += i - 4;
							movelist[n].m[2] = m;
							whitemancapture(b, &n, movelist, i - 8);
						}
					}
					if ((b[i - 5] & BLACK) != 0)
					{
						if ((b[i - 10] & FREE) != 0)
						{
							movelist[n].n = 3;
							if (i <= 17) m = (WHITE | KING); else m = (WHITE | MAN); m = m << 8;
							m += FREE; m = m << 8;
							m += i - 10;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (WHITE | MAN); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i - 5]; m = m << 8;
							m += i - 5;
							movelist[n].m[2] = m;
							whitemancapture(b, &n, movelist, i - 10);
						}
					}
				}
				else /* b[i] is a KING */
				{
					if ((b[i + 4] & BLACK) != 0)
					{
						if ((b[i + 8] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (WHITE | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i + 8;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (WHITE | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i + 4]; m = m << 8;
							m += i + 4;
							movelist[n].m[2] = m;
							tmp = b[i + 4];
							b[i + 4] = FREE;
							b[i] = FREE;
							whitekingcapture(b, &n, movelist, i + 8);
							b[i + 4] = tmp;
							b[i] = WHITE | KING;
						}
					}
					if ((b[i + 5] & BLACK) != 0)
					{
						if ((b[i + 10] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (WHITE | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i + 10;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (WHITE | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i + 5]; m = m << 8;
							m += i + 5;
							movelist[n].m[2] = m;
							tmp = b[i + 5];
							b[i + 5] = FREE;
							b[i] = FREE;
							whitekingcapture(b, &n, movelist, i + 10);
							b[i + 5] = tmp;
							b[i] = WHITE | KING;
						}
					}
					if ((b[i - 4] & BLACK) != 0)
					{
						if ((b[i - 8] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (WHITE | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i - 8;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (WHITE | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i - 4]; m = m << 8;
							m += i - 4;
							movelist[n].m[2] = m;
							tmp = b[i - 4];
							b[i - 4] = FREE;
							b[i] = FREE;
							whitekingcapture(b, &n, movelist, i - 8);
							b[i - 4] = tmp;
							b[i] = WHITE | KING;
						}
					}
					if ((b[i - 5] & BLACK) != 0)
					{
						if ((b[i - 10] & FREE) != 0)
						{
							movelist[n].n = 3;
							m = (WHITE | KING); m = m << 8;
							m += FREE; m = m << 8;
							m += i - 10;
							movelist[n].m[1] = m;
							m = FREE; m = m << 8;
							m += (WHITE | KING); m = m << 8;
							m += i;
							movelist[n].m[0] = m;
							m = FREE; m = m << 8;
							m += b[i - 5]; m = m << 8;
							m += i - 5;
							movelist[n].m[2] = m;
							tmp = b[i - 5];
							b[i - 5] = FREE;
							b[i] = FREE;
							whitekingcapture(b, &n, movelist, i - 10);
							b[i - 5] = tmp;
							b[i] = WHITE | KING;
						}
					}
				}
			}
		}
	}
	return(n);
}

void  blackmancapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int i)
{
	int m;
	int found = 0;
	struct move2 move, orgmove;

	orgmove = movelist[*n];
	move = orgmove;

	if ((b[i + 4] & WHITE) != 0)
	{
		if ((b[i + 8] & FREE) != 0)
		{
			move.n++;
			if (i >= 28) m = (BLACK | KING); else m = (BLACK | MAN); m = m << 8;
			m += FREE; m = m << 8;
			m += (i + 8);
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i + 4]; m = m << 8;
			m += (i + 4);
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			blackmancapture(b, n, movelist, i + 8);
		}
	}
	move = orgmove;
	if ((b[i + 5] & WHITE) != 0)
	{
		if ((b[i + 10] & FREE) != 0)
		{
			move.n++;
			if (i >= 28) m = (BLACK | KING); else m = (BLACK | MAN); m = m << 8;
			m += FREE; m = m << 8;
			m += (i + 10);
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i + 5]; m = m << 8;
			m += (i + 5);
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			blackmancapture(b, n, movelist, i + 10);
		}
	}
	if (!found) (*n)++;
}

void  blackkingcapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int i)
{
	int m;
	int tmp;
	int found = 0;
	struct move2 move, orgmove;

	orgmove = movelist[*n];
	move = orgmove;

	if ((b[i - 4] & WHITE) != 0)
	{
		if ((b[i - 8] & FREE) != 0)
		{
			move.n++;
			m = (BLACK | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i - 8;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i - 4]; m = m << 8;
			m += i - 4;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i - 4];
			b[i - 4] = FREE;
			blackkingcapture(b, n, movelist, i - 8);
			b[i - 4] = tmp;
		}
	}
	move = orgmove;
	if ((b[i - 5] & WHITE) != 0)
	{
		if ((b[i - 10] & FREE) != 0)
		{
			move.n++;
			m = (BLACK | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i - 10;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i - 5]; m = m << 8;
			m += i - 5;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i - 5];
			b[i - 5] = FREE;
			blackkingcapture(b, n, movelist, i - 10);
			b[i - 5] = tmp;
		}
	}
	move = orgmove;
	if ((b[i + 4] & WHITE) != 0)
	{
		if ((b[i + 8] & FREE) != 0)
		{
			move.n++;
			m = (BLACK | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i + 8;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i + 4]; m = m << 8;
			m += i + 4;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i + 4];
			b[i + 4] = FREE;
			blackkingcapture(b, n, movelist, i + 8);
			b[i + 4] = tmp;
		}
	}
	move = orgmove;
	if ((b[i + 5] & WHITE) != 0)
	{
		if ((b[i + 10] & FREE) != 0)
		{
			move.n++;
			m = (BLACK | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i + 10;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i + 5]; m = m << 8;
			m += i + 5;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i + 5];
			b[i + 5] = FREE;
			blackkingcapture(b, n, movelist, i + 10);
			b[i + 5] = tmp;
		}
	}
	if (!found) (*n)++;
}

void  whitemancapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int i)
{
	int m;
	int found = 0;
	struct move2 move, orgmove;

	orgmove = movelist[*n];
	move = orgmove;

	if ((b[i - 4] & BLACK) != 0)
	{
		if ((b[i - 8] & FREE) != 0)
		{
			move.n++;
			if (i <= 17) m = (WHITE | KING); else m = (WHITE | MAN); m = m << 8;
			m += FREE; m = m << 8;
			m += i - 8;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i - 4]; m = m << 8;
			m += i - 4;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			whitemancapture(b, n, movelist, i - 8);
		}
	}
	move = orgmove;
	if ((b[i - 5] & BLACK) != 0)
	{
		if ((b[i - 10] & FREE) != 0)
		{
			move.n++;
			if (i <= 17) m = (WHITE | KING); else m = (WHITE | MAN); m = m << 8;
			m += FREE; m = m << 8;
			m += i - 10;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i - 5]; m = m << 8;
			m += i - 5;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			whitemancapture(b, n, movelist, i - 10);
		}
	}
	if (!found) (*n)++;
}

void  whitekingcapture(int b[46], int *n, struct move2 movelist[MAXMOVES], int i)
{
	int m;
	int tmp;
	int found = 0;
	struct move2 move, orgmove;

	orgmove = movelist[*n];
	move = orgmove;

	if ((b[i - 4] & BLACK) != 0)
	{
		if ((b[i - 8] & FREE) != 0)
		{
			move.n++;
			m = (WHITE | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i - 8;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i - 4]; m = m << 8;
			m += i - 4;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i - 4];
			b[i - 4] = FREE;
			whitekingcapture(b, n, movelist, i - 8);
			b[i - 4] = tmp;
		}
	}
	move = orgmove;
	if ((b[i - 5] & BLACK) != 0)
	{
		if ((b[i - 10] & FREE) != 0)
		{
			move.n++;
			m = (WHITE | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i - 10;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i - 5]; m = m << 8;
			m += i - 5;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i - 5];
			b[i - 5] = FREE;
			whitekingcapture(b, n, movelist, i - 10);
			b[i - 5] = tmp;
		}
	}
	move = orgmove;
	if ((b[i + 4] & BLACK) != 0)
	{
		if ((b[i + 8] & FREE) != 0)
		{
			move.n++;
			m = (WHITE | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i + 8;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i + 4]; m = m << 8;
			m += i + 4;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i + 4];
			b[i + 4] = FREE;
			whitekingcapture(b, n, movelist, i + 8);
			b[i + 4] = tmp;
		}
	}
	move = orgmove;
	if ((b[i + 5] & BLACK) != 0)
	{
		if ((b[i + 10] & FREE) != 0)
		{
			move.n++;
			m = (WHITE | KING); m = m << 8;
			m += FREE; m = m << 8;
			m += i + 10;
			move.m[1] = m;
			m = FREE; m = m << 8;
			m += b[i + 5]; m = m << 8;
			m += i + 5;
			move.m[move.n - 1] = m;
			found = 1;
			movelist[*n] = move;
			tmp = b[i + 5];
			b[i + 5] = FREE;
			whitekingcapture(b, n, movelist, i + 10);
			b[i + 5] = tmp;
		}
	}
	if (!found) (*n)++;
}

int  testcapture(int b[46], int color)
/*----------> purpose: test if color has a capture on b
----------> version: 1.0
----------> date: 25th october 97 */
{
	int i;

	if (color == BLACK)
	{
		for (i = 5; i <= 40; i++)
		{
			if ((b[i] & BLACK) != 0)
			{
				if ((b[i] & MAN) != 0)
				{
					if ((b[i + 4] & WHITE) != 0)
					{
						if ((b[i + 8] & FREE) != 0)
							return(1);
					}
					if ((b[i + 5] & WHITE) != 0)
					{
						if ((b[i + 10] & FREE) != 0)
							return(1);
					}
				}
				else /* b[i] is a KING */
				{
					if ((b[i + 4] & WHITE) != 0)
					{
						if ((b[i + 8] & FREE) != 0)
							return(1);
					}
					if ((b[i + 5] & WHITE) != 0)
					{
						if ((b[i + 10] & FREE) != 0)
							return(1);
					}
					if ((b[i - 4] & WHITE) != 0)
					{
						if ((b[i - 8] & FREE) != 0)
							return(1);
					}
					if ((b[i - 5] & WHITE) != 0)
					{
						if ((b[i - 10] & FREE) != 0)
							return(1);
					}
				}
			}
		}
	}
	else /* color is WHITE */
	{
		for (i = 5; i <= 40; i++)
		{
			if ((b[i] & WHITE) != 0)
			{
				if ((b[i] & MAN) != 0)
				{
					if ((b[i - 4] & BLACK) != 0)
					{
						if ((b[i - 8] & FREE) != 0)
							return(1);
					}
					if ((b[i - 5] & BLACK) != 0)
					{
						if ((b[i - 10] & FREE) != 0)
							return(1);
					}
				}
				else /* b[i] is a KING */
				{
					if ((b[i + 4] & BLACK) != 0)
					{
						if ((b[i + 8] & FREE) != 0)
							return(1);
					}
					if ((b[i + 5] & BLACK) != 0)
					{
						if ((b[i + 10] & FREE) != 0)
							return(1);
					}
					if ((b[i - 4] & BLACK) != 0)
					{
						if ((b[i - 8] & FREE) != 0)
							return(1);
					}
					if ((b[i - 5] & BLACK) != 0)
					{
						if ((b[i - 10] & FREE) != 0)
							return(1);
					}
				}
			}
		}
	}
	return(0);
}

/******************** Heuristics */

int numMen(int board[46], int color)
{
	int count = 0;
	for (int i = 0; i < 46; i++)
	{
		if ((int)(board[i] ^ MAN) == color)
			count++;
	}

	return count;
}

int numKings(int board[46], int color)
{
	int count = 0;
	for (int i = 0; i < 46; i++)
	{
		if ((int)(board[i] ^ KING) == color)
		{
			count++;
		}
	}
	return count;
}

int totalPieces(int board[46], int color)
{
	return numMen(board, color) + numKings(board, color);
}

int evaluation(int board[46])
{
	int blackPieces = totalPieces(board, BLACK), whitePieces = totalPieces(board, WHITE);
	int moveVal = ((numKings(board, BLACK)*KING_WEIGHT) + (blackPieces*PIECE_WEIGHT) + piecePositionScore(board, BLACK)) - ((numKings(board, WHITE)*KING_WEIGHT) + (whitePieces*PIECE_WEIGHT) + piecePositionScore(board, WHITE));
	return moveVal;
}

int piecePositionScore(int board[46], int color)
{
	int score = 0;
	for (int i = 0; i < 46; i++)
	{
		if ((int)(board[i] ^ color) == KING || (int)(board[i] ^ color) == MAN)
		{
			score += locationVal[i];
		}
	}

	return score;
}

int passedMaxTime(double maxTime)
{
	double currentTime = (double)((clock() - start) / CLK_TCK);
	if (currentTime > maxTime)
		return 1;
	return 0;
}