
#include "projects/crossroads/map.h"


#define ANSI_NONE "\033[0m"
#define ANSI_BLACK "\033[30m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

#define ON_ANSI_BLACK "\033[40m"
#define ON_ANSI_RED "\033[41m"
#define ON_ANSI_GREEN "\033[42m"
#define ON_ANSI_YELLOW "\033[43m"
#define ON_ANSI_BLUE "\033[44m"
#define ON_ANSI_MAGENTA "\033[45m"
#define ON_ANSI_CYAN "\033[46m"
#define ON_ANSI_WHITE "\033[47m"

#define clear() printf("\033[H\033[J")
#define gotoxy(y,x) printf("\033[%d;%dH", (y), (x))

//default map
const char map_draw_default[7][7] = {
	{'X', 'X', ' ', '-', ' ', 'X', 'X'}, // [0][0] [0][1] [0][2] [0][3] [0][4] [0][5] [0][6]
	{'X', 'X', ' ', '-', ' ', 'X', 'X'}, // [1][0] [1][1] [1][2] [1][3] [1][4] [1][5] [1][6]
	{' ', ' ', ' ', '-', ' ', ' ', ' '}, // [2][0] [2][1] [2][2] [2][3] [2][4] [2][5] [2][6]
	{'-', '-', '-', 'X', '-', '-', '-'}, // [3][0] [3][1] [3][2] [3][3] [3][4] [3][5] [3][6]
	{' ', ' ', ' ', '-', ' ', ' ', ' '}, // [4][0] [4][1] [4][2] [4][3] [4][4] [4][5] [4][6]
	{'X', 'X', ' ', '-', ' ', 'X', 'X'}, // [5][0] [5][1] [5][2] [5][3] [5][4] [5][5] [5][6]
	{'X', 'X', ' ', '-', ' ', 'X', 'X'}, // [6][0] [6][1] [6][2] [6][3] [6][4] [6][5] [6][6]
};


void map_draw(void)
{
	int i, j;

	clear();

	for (i=0; i<7; i++) {
		for (j=0; j<7; j++) {
			printf("%c ", map_draw_default[i][j]);
		}
		printf("\n");
	}
	printf("unit step: %d\n", crossroads_step);
	gotoxy(0, 0);
}

void map_draw_vehicle(char id, int row, int col)
{
	if (row >= 0 && col >= 0) {
		gotoxy(row + 1, col * 2 + 1);
		printf("%c ", id);
		gotoxy(0, 0);
	}
}

void map_draw_reset(void)
{
	clear();
}
