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
#define MAX_BULLETS 200

static int BOARD_X = MAC_BOARD_X;
static int BOARD_Y = MAC_BOARD_Y;

struct Location {
	unsigned char y;
	unsigned char x;
};

struct Player {
	int y;
	int x;
	int dir;
};

static char bulletSymbol = 'o';

struct Bullet {
	int y;
	int x;
	int yDir;
	int xDir;
	_Bool active;
};

struct Bullets {
	int maxBullets;
	int i;
	struct Bullet bullet[200];
};

struct GameState{
	unsigned char numberOfPlayers;
	unsigned char gameBoard[MAC_BOARD_Y][MAC_BOARD_X];
	struct Player player[MAX_NUMBER_OF_PLAYERS];
	struct Bullets bullets;
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

void getItemAtXY(char** symbol, struct GameState* gameState, int y, int x) {
	unsigned char * playerNumber = &(gameState->numberOfPlayers);
	for(int i=0; i < *playerNumber; i++) {
		struct Player * currentPlayer = &(gameState->player[i]);
		struct GunDir * gunDir = &(gunDirs[currentPlayer->dir]);
		if (currentPlayer->x == x && currentPlayer->y == y) {
			*symbol = playerSymbol[i];
		} else if ((gunDir->xOffset + currentPlayer->x) == x && (gunDir->yOffset + currentPlayer->y) == y) {
			*symbol = gunSymbol[i][currentPlayer->dir];
		}
	}
}

void addBullet (int plyrNum, struct GameState* gameState) {
	struct Player * player = &gameState->player[plyrNum];
	struct GunDir * gunDir = &gunDirs[player->dir];
	struct Bullets * bullets = &gameState->bullets;
	bullets->bullet[bullets->i] = (struct Bullet){player->y + gunDir->yOffset, player->x + gunDir->xOffset, gunDir->yOffset, gunDir->xOffset, 1};
	bullets->i += 1;
	if (bullets->i >= bullets->maxBullets) {
		bullets->i = 0;
	}
};

_Bool testNewPosPlayerCollision(int y, int x, int dir, struct GameState* gameState) {
		struct GunDir * gunDir = &(gunDirs[dir]);
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
		} else if (MAP_OBSTRUCTION[gameState->gameBoard[y][x]]) {
			return 0;
		} else if (MAP_OBSTRUCTION[gameState->gameBoard[turretY][turretX]]) {
			return 0;
		}
		return 1;
}

_Bool isBulletAt(struct GameState* gamestate, int y, int x) {
	struct Bullets* bs = &gamestate->bullets;
	for (int i=0; i<bs->maxBullets; i++) {
		struct Bullet* crtB = &bs->bullet[i];
		if (crtB->active && crtB->y == y && crtB->x == x) {
			return 1;
		}
	}
	return 0;
}

void moveBullets(struct GameState* gamestate) {
	struct Bullets* bs = &gamestate->bullets;
	for (int i=0; i<bs->maxBullets; i++) {
		struct Bullet* crtB = &bs->bullet[i];
		if (crtB->active) {
			crtB->y += crtB->yDir;
			crtB->x += crtB->xDir;
		}

		if (crtB->y < 0 || crtB->y >= BOARD_Y || crtB->x < 0 || crtB->x >= BOARD_X) {
			crtB->active = 0;
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
		.player = {player1},
		.bullets.maxBullets = MAX_BULLETS,
		.bullets.i = 0
	};

	struct termios old;
	configureTerminal(&old);

	while (isGameOn) {
		system("clear");

		for (int y=BOARD_Y - 1; y >= 0; y--) {
			for (int x=0; x<BOARD_X; x++) {
				char* symbol = NULL;
				getItemAtXY(&symbol, &gameState, y, x);
				_Bool isBullet = isBulletAt(&gameState, y, x);
				if (symbol) {
					printf("%s", symbol);
				} else if (isBullet) {
					printf("%c", bulletSymbol);
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
			int* pDir = &gameState.player[0].dir;
			if (ch == ';') {
				isGameOn = 0;
			} else if (ch == 'w') {
				if (testNewPosPlayerCollision(*pY + 1, *pX, *pDir, &gameState)) {
					*pY += 1;
				}
			} else if (ch == 's') {
				if (testNewPosPlayerCollision(*pY - 1, *pX, *pDir, &gameState)) {
					*pY -= 1;
				}
			} else if (ch == 'd') {
				if (testNewPosPlayerCollision(*pY, *pX + 1, *pDir, &gameState)) {
					*pX += 1;
				}
			} else if (ch == 'a') {
				if (testNewPosPlayerCollision(*pY, *pX - 1, *pDir, &gameState)) {
					*pX -= 1;
				}
			} else if (ch == 'k') {
				int newDir = (gameState.player[0].dir + 1) % 8;
				if (testNewPosPlayerCollision(*pY, *pX, newDir, &gameState)) {
					gameState.player[0].dir = newDir;
				}
			} else if (ch == 'j') {
				int newDir;
				if (*pDir == 0) {
					newDir = 7;
				} else {
					newDir = (*pDir - 1);
				}
				
				if (testNewPosPlayerCollision(*pY, *pX, newDir, &gameState)) {
					gameState.player[0].dir = newDir;
				}
			} else if (ch == ' ') {
				addBullet(0, &gameState);
			}
		}
		moveBullets(&gameState);
		usleep(33000); // Sleep for 100ms to reduce CPU usage
	}
	return 0;
}

