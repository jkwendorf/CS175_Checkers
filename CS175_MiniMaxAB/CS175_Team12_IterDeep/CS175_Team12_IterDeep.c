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
/*----------> definitions */
#define OCCUPIED 0
#define WHITE 1
#define BLACK 2
#define MAN 4
#define KING 8
#define FREE 16
#define CHANGECOLOR 3
#define MAXDEPTH 99
#define MAXMOVES 28

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



/* required functions */
int  	WINAPI getmove(int b[8][8], int color, double maxtime, char str[255], int *playnow, int info, int unused, struct CBmove *move);

/*----------> part II: search */
int  checkers(int b[46], int color, double maxtime, char *str);
void doMove(int b[46], struct move2 move);
void undoMove(int b[46], struct move2 move);
int masterIterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str);
int iterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str, int depth);

/*----------> part III: move generation */

int  generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color);

/*********** Heuristics */

int numWhiteMen(int board[46]);
int numWhiteKings(int board[46]);
int numBlackMen(int board[46]);
int numBlackKings(int board[46]);
int totalWhitePieces(int board[46]);
int totalBlackPieces(int board[46]);

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
		sprintf(reply, "CS 175 Team 12 - Iterative Deepening");
		return 1;
	}

	if (strcmp(command, "about") == 0)
	{
		sprintf(reply, "Team 12 - Iterative Deepening\n\n2015 CS 175");
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
	clock_t start;
	int eval, numOfMoves;

	start = clock();
	eval = masterIterativeDeepening(b, color, maxtime, start, str);

	return eval;
}

int masterIterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str)
{
	int eval, value, bestVal = -1;
	struct move2 best;
	struct move2 movesList[MAXMOVES];
	int moveCount;

	// For all depths
	for (int currentDepth = 1; currentDepth < MAXDEPTH && clock() - start < maxtime; currentDepth++)
	{
		// get the list of all possible moves
		moveCount = generatemovelist(b, movesList, color);

		// if we can't make any moves
		if (moveCount == 0)
		{
			// return that we lost
			if (color == BLACK)
			{
				if (totalBlackPieces(b) == 0)
					return -1;
			}
			else if (color == WHITE)
			{
				if (totalWhitePieces(b) == 0)
					return 1;
			}
		}

		// for each move
		for (int i = 0; i < moveCount && clock() - start < maxtime; i++)
		{
			// do the move
			doMove(b, movesList[i]);
			// get the value of the move
			value = iterativeDeepening(b, color, maxtime, start, str, currentDepth - 1);
			// undo the move
			undoMove(b, movesList[i]);

			// If its the first move, save the best value
			if (i == 0)
			{
				bestVal = value;
				best = movesList[i];
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
					}
				}
				else if (color == WHITE)
				{
					if (value < bestVal)
					{
						bestVal = value;
						best = movesList[i];
					}
				}
			}
		}
	}

	doMove(b, best);

	return eval;
}

int iterativeDeepening(int b[46], int color, double maxtime, clock_t start, char *str, int depth)
{
	if (depth == 0)
		return totalBlackPieces(b) - totalWhitePieces(b);

	int value, bestVal, moveCount;
	struct move2 movesList[MAXMOVES];

	// generate all possible moves at this depth
	moveCount = generatemovelist(b, movesList, color);

	// If we can't make a move because we lost or we're "trapped"
	if (moveCount == 0)
	{
		if (color == BLACK)
		{
			if (totalBlackPieces(b) == 0)
				return -totalWhitePIeces(b);
		}
		else if (color == WHITE)
		{
			if (totalWhitePieces(b) == 0)
				return totalBlackPieces(b);
		}

		return iterativeDeepening(b, color^CHANGECOLOR, maxtime, start, str, depth-1);
	}

	// for each move
	for (int i = 0; i << moveCount && clock() - start < maxtime; i++)
	{
		doMove(b, movesList[i]);
		value = iterativeDeepening(b, color^CHANGECOLOR, maxtime, start, str, depth - 1);
		undoMove(b, movesList[i]);

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
	}
	return bestVal;
}

void doMove(int b[46], struct move2 move)
{
	
}

void undoMove(int b[46], struct move2 move)
{

}

/*-------------- PART III: MOVE GENERATION -----------------------------------*/

int generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color)
{
	int totalMoves = 0;

	// Generate all black moves
	if (color == BLACK)
	{
	//	for ()
	}
	// Generate all white moves
	else
	{
	
	}

	return totalMoves;
}

/******************** Heuristics */

int numWhiteMen(int board[46])
{
	int count = 0;
	for (int i = 0; i < 46; i++)
	{
		if (board[i] == WHITE | MAN)
			count++;
	}

	return count;
}

int numWhiteKings(int board[46])
{
	int count = 0;
	for (int i = 0; i < 46; i++)
	{
		if (board[i] == WHITE | KING)
			count++;
	}

	return count;
}

int numBlackMen(int board[46])
{
	int count = 0;
	for (int i = 0; i < 46; i++)
	{
		if (board[i] == BLACK | MAN)
			count++;
	}

	return count;
}

int numBlackKings(int board[46])
{
	int count = 0;
	for (int i = 0; i < 46; i++)
	{
		if (board[i] == BLACK | KING)
			count++;
	}

	return count;
}

int totalWhitePieces(int board[46])
{
	return numWhiteMen(board) + numWhiteKings(board);
}

int totalBlackPieces(int board[46])
{
	return numBlackMen(board) + numBlackKings(board);
}