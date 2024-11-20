#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

#define MAC_BOARD_X 27
#define MAC_BOARD_Y 20
#define MAX_NUMBER_OF_PLAYERS 4

static int BOARD_X = MAC_BOARD_X;
static int BOARD_Y = MAC_BOARD_Y;

struct Location {
	unsigned char y;
	unsigned char x;
};

struct Player {
	int x;
	int y;
	unsigned char dir;
};

struct GameState{
	unsigned char numberOfPlayers;
	unsigned char gameBoard[MAC_BOARD_Y][MAC_BOARD_X];
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
static _Bool MAP_OBSTRUCTION[3] = {1, 0, 1};

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

_Bool checkNoPlayer1Collision(int y, int x, struct GameState gameState) {
		struct Player * plyr = &(gameState.player[0]);
		struct GunDir * gunDir = &(gunDirs[plyr->dir]);
		int turretX = x + gunDir->xOffset;
		int turretY = y + gunDir->yOffset;
		if (x < 0 || turretX < 0) {
			return 0;
		} else if (y < 0 || turretX < 0) {
			return 0;
		} else if (y >= (BOARD_Y - 1) || turretY >= (BOARD_Y - 1)) {
			return 0;
		} else if (x >= (BOARD_X - 1) || turretX >= (BOARD_X - 1)) {
			return 0;
		} else if (MAP_OBSTRUCTION[gameState.gameBoard[y][x]]) {
			return 0;
		} else if (MAP_OBSTRUCTION[gameState.gameBoard[turretY][turretX]]) {
			return 0;
		}
		return 1;
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
			{0, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
			{0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
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
			int* pY = &gameState.player[0].y;
			int* pX = &gameState.player[0].x;
			if (ch == ';') {
				isGameOn = 0;
			} else if (ch == 'w') {
				if (checkNoPlayer1Collision(*pY + 1, *pX, gameState)) {
					*pY += 1;
				}
			} else if (ch == 's') {
				if (checkNoPlayer1Collision(*pY - 1, *pX, gameState)) {
					*pY -= 1;
				}
			} else if (ch == 'd') {
				if (checkNoPlayer1Collision(*pY, *pX + 1, gameState)) {
					*pX += 1;
				}
			} else if (ch == 'a') {
				if (checkNoPlayer1Collision(*pY, *pX - 1, gameState)) {
					*pX -= 1;
				}
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

