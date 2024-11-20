#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

#define BOARD_X 27
#define BOARD_Y 20
#define MAX_NUMBER_OF_PLAYERS 4

struct Location {
	unsigned char y;
	unsigned char x;
};

struct Player {
	unsigned char x;
	unsigned char y;
	unsigned char dir;
};

struct GameState{
	unsigned char numberOfPlayers;
	unsigned char gameBoard[BOARD_Y][BOARD_X];
	struct Player player[MAX_NUMBER_OF_PLAYERS];
};

struct GunDir{
	int yOffset;
	int xOffset;
};

/*
* Here are the turret directions
* 
* 7 0 1
* 6   2
* 5 4 3
 */
static struct GunDir gunDirs[8] = {
	{1, 0},
	{1, 1},
	{0, 1},
	{-1, 1},
	{-1, 0},
	{-1, -1},
	{0, -1},
	{1, -1}
};

static char* playerSymbol[4] = {
	"\033[34m0\033[0m",
	"\033[32m0\033[0m",
	"\033[33m0\033[0m",
	"\033[35m0\033[0m",
};

static char* gunSymbol[4][8] = {
    // Index 0: Blue symbols
    {
        "\033[34m|\033[0m",
        "\033[34m/\033[0m",
        "\033[34m-\033[0m",
        "\033[34m\\\033[0m",
        "\033[34m|\033[0m",
        "\033[34m/\033[0m",
        "\033[34m-\033[0m",
        "\033[34m\\\033[0m"
    },
    // Index 1: Green symbols
    {
        "\033[32m|\033[0m",
        "\033[32m/\033[0m",
        "\033[32m-\033[0m",
        "\033[32m\\\033[0m",
        "\033[32m|\033[0m",
        "\033[32m/\033[0m",
        "\033[32m-\033[0m",
        "\033[32m\\\033[0m"
    },
    // Index 2: Yellow symbols
    {
        "\033[33m|\033[0m",
        "\033[33m/\033[0m",
        "\033[33m-\033[0m",
        "\033[33m\\\033[0m",
        "\033[33m|\033[0m",
        "\033[33m/\033[0m",
        "\033[33m-\033[0m",
        "\033[33m\\\033[0m"
    },
    // Index 3: Magenta symbols
    {
        "\033[35m|\033[0m",
        "\033[35m/\033[0m",
        "\033[35m-\033[0m",
        "\033[35m\\\033[0m",
        "\033[35m|\033[0m",
        "\033[35m/\033[0m",
        "\033[35m-\033[0m",
        "\033[35m\\\033[0m"
    }
};

static char MAP_SYMBOL[3] = {'#', '`', '%'};

// Function to configure terminal for non-blocking input
void configureTerminal(struct termios *old) {
    struct termios new;
    tcgetattr(STDIN_FILENO, old); // Get current terminal attributes
    new = *old;
    new.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
}

// Function to restore terminal attributes
void restoreTerminal(struct termios *old) {
	tcsetattr(STDIN_FILENO, TCSANOW, old); // Restore old attributes
}

// Function to check if a key is pressed
int isKeyPressed() {
	struct timeval tv = {0, 0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

void getItemAtXY(char** symbol, struct GameState gameState, int x, int y) {
	unsigned char * playerNumber = &(gameState.numberOfPlayers);
	for(int i=0; i < *playerNumber; i++) {
		struct Player * currentPlayer = &(gameState.player[i]);
		struct GunDir * gunDir = &(gunDirs[currentPlayer->dir]);
		if (currentPlayer->x == x && currentPlayer->y == y) {
			*symbol = playerSymbol[i];
		} else if ((gunDir->xOffset + currentPlayer->x) == x && (gunDir->yOffset + currentPlayer->y) == y) {
			*symbol = gunSymbol[i][currentPlayer->dir];
		}
	}
}

int main() {
	_Bool isGameOn = 1;

	// Setup players
	struct Player player1;
	player1.y = 1;
	player1.x = 1;
	player1.dir = 0;
	

	struct GameState gameState = {
		.numberOfPlayers = 1,
		.gameBoard = {
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
		},
		.player = {player1}
	};

	struct termios old;
	configureTerminal(&old);

	while (isGameOn) {
		system("clear");

		for (int y=BOARD_Y - 1; y >= 0; y--) {
			for (int x=0; x<BOARD_X; x++) {
				char* symbol = NULL;
				getItemAtXY(&symbol, gameState, x, y);
				if (symbol) {
					printf("%s", symbol);
				} else {
					printf("%c", MAP_SYMBOL[gameState.gameBoard[y][x]]);
				}
			}
			printf("\n");
		}
		if (isKeyPressed()) {
			char ch = getchar();
			if (ch == ';') {
				isGameOn = 0;
			} else if (ch == 'w') {
				gameState.player[0].y++;
			} else if (ch == 's') {
				gameState.player[0].y--;
			} else if (ch == 'd') {
				gameState.player[0].x++;
			} else if (ch == 'a') {
				gameState.player[0].x--;
			} else if (ch == 'k') {
				gameState.player[0].dir = (gameState.player[0].dir + 1) % 8;
			} else if (ch == 'j') {
				if (gameState.player[0].dir == 0) {
					gameState.player[0].dir = 7;
				} else {
					gameState.player[0].dir = (gameState.player[0].dir - 1);
				}
			}
		}
		usleep(33000); // Sleep for 100ms to reduce CPU usage
	}
	return 0;
}

