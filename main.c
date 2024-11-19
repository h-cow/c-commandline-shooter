#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

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
	char symbol;
};

struct GameState{
	unsigned char numberOfPlayers;
	unsigned char gameBoard[BOARD_Y][BOARD_X];
	struct Player player[MAX_NUMBER_OF_PLAYERS];
};

struct GunDir{
	char symbol;
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
	{'|', 1, 0},
	{'/', 1, 1},
	{'-', 0, 1},
	{'\\', -1, 1},
	{'|', -1, 0},
	{'/', -1, -1},
	{'-', 0, -1},
	{'\\', 1, -1}
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

void getItemAtXY(char * symbol, struct GameState gameState, int x, int y) {
	unsigned char * playerNumber = &(gameState.numberOfPlayers);
	for(int i=0; i < *playerNumber; i++) {
		struct Player * currentPlayer = &(gameState.player[i]);
		struct GunDir * gunDir = &(gunDirs[currentPlayer->dir]);
		if (currentPlayer->x == x && currentPlayer->y == y) {
			*symbol = currentPlayer->symbol;
		} else if ((gunDir->xOffset + currentPlayer->x) == x && (gunDir->yOffset + currentPlayer->y) == y) {
			*symbol = gunDir->symbol;
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
	player1.symbol = '0';
	

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
				char symbol = '\0';
				getItemAtXY(&symbol, gameState, x, y);
				if (symbol != '\0') {
					printf("%c", symbol);
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

