#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>

#define MAC_BOARD_X 120
#define MAC_BOARD_Y 60
#define MAX_NUMBER_OF_PLAYERS 4
#define MAX_BULLETS 200
#define MAC_EXPLOSION_LENGTH 6

static int BOARD_X = MAC_BOARD_X;
static int BOARD_Y = MAC_BOARD_Y;
static int EXPLOSION_LENGTH = MAC_EXPLOSION_LENGTH;

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

enum EXPLOSION_SQUARE_TYPE {
	HEAVY_EXPLOSTION,
	LIGHT_EXPLOSTION,
	LIGHT_DELAYED1_EXPLOSION,
	LIGHT_DELAYED2_EXPLOSION,
	LIGHT_DELAYED3_EXPLOSION
};

struct ExplosionSquare {
	enum EXPLOSION_SQUARE_TYPE explosionType;
	int currentStage;
};

struct GameState{
	unsigned char numberOfPlayers;
	int gameBoard[MAC_BOARD_Y][MAC_BOARD_X];
	struct ExplosionSquare explosionMap[MAC_BOARD_Y][MAC_BOARD_X];
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

static char MAP_SYMBOL[3] = {'`', 'X', '%'};
static _Bool MAP_OBSTRUCTION[3] = {0, 1, 1};
static int MAP_OBSTRUCTION_EXPLOSION_SIZE[3] = {0, 1, 3};

enum EXPLOSION_SQUARE_PHASE {
	NO_EXPLOSION,
	HOTTEST,
	REALLY_HOT,
	HOT,
	SMOKE,
	LIGHT_SMOKE
};
const char* explosionSymbols[6] = {
    " ",                          // Space
    "\033[37m@\033[0m",           // White @
    "\033[33m@\033[0m",           // Orange @ (yellow in ANSI)
    "\033[31m@\033[0m",           // Red @
    "\033[90m@\033[0m",           // Gray @
    "\033[90mC\033[0m"            // Gray C
};

static int EXPLOSION_STEP[5][MAC_EXPLOSION_LENGTH] = {
	{HOTTEST, REALLY_HOT, HOT, SMOKE, LIGHT_SMOKE, NO_EXPLOSION},
	{REALLY_HOT, HOT, SMOKE, LIGHT_SMOKE, NO_EXPLOSION, NO_EXPLOSION},
	{NO_EXPLOSION, REALLY_HOT, HOT, SMOKE, LIGHT_SMOKE, NO_EXPLOSION},
	{NO_EXPLOSION, NO_EXPLOSION, REALLY_HOT, HOT, SMOKE, LIGHT_SMOKE},
	{NO_EXPLOSION, NO_EXPLOSION, NO_EXPLOSION, HOT, SMOKE, LIGHT_SMOKE}
};

void triggerExplosion(struct GameState* gameState, int mapTerrainNum, int y, int x) {
	int size = MAP_OBSTRUCTION_EXPLOSION_SIZE[mapTerrainNum];
	if (size && size == 1) {
		gameState->explosionMap[y][x] = (struct ExplosionSquare){LIGHT_EXPLOSTION, 1};
	} else if (size > 1) {
		gameState->explosionMap[y][x] = (struct ExplosionSquare){HEAVY_EXPLOSTION, 1};
	}
}


void clear6Square(int gameBoard[MAC_BOARD_Y][MAC_BOARD_X], int bttmY, int bttmX) {
	for (int y=bttmY; y<bttmY+7; y++) {
		for (int x=bttmX; x<bttmX+7; x++) {
			gameBoard[y][x] = 0;
		}
	}
}

void animateExplosions(struct GameState* gameState) {
	for (int y=0; y<BOARD_Y; y++) {
		for (int x=0; x<BOARD_X; x++) {
			struct ExplosionSquare* expSqr= &gameState->explosionMap[y][x];
			if (expSqr->currentStage > 0) {
				expSqr->currentStage += 1;
			}
			if (expSqr->currentStage > 6) {
				expSqr->currentStage = 0;
			}
		}
	}
}

void initGameBoard(int gameBoard[MAC_BOARD_Y][MAC_BOARD_X]) {
	for (int y=0; y < BOARD_Y; y++) {
		for (int x=0; x < BOARD_X; x++) {
			int randomNum = rand() % 40;
			if (randomNum == 0) {
				gameBoard[y][x] = 1;
			} else if (randomNum == 1) {
				gameBoard[y][x] = 2;
			} else {
				gameBoard[y][x] = 0;
			}
		}
	}
	// Clean up the spawn area
	int endIndexOffset = 7;
	clear6Square(gameBoard, 0, 0);
	clear6Square(gameBoard, 0, BOARD_X - endIndexOffset);
	clear6Square(gameBoard, BOARD_Y - endIndexOffset, BOARD_X - endIndexOffset);
	clear6Square(gameBoard, BOARD_Y - endIndexOffset, 0);
}

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
		} else if (y < 0 || turretY < 0) {
			return 0;
		} else if (y >= (BOARD_Y) || turretY >= (BOARD_Y)) {
			return 0;
		} else if (x >= (BOARD_X) || turretX >= (BOARD_X)) {
			return 0;
		} else if (MAP_OBSTRUCTION[gameState->gameBoard[y][x]]) {
			return 0;
		} else if (MAP_OBSTRUCTION[gameState->gameBoard[turretY][turretX]]) {
			return 0;
		}
		return 1;
}

struct Bullet* getBulletAt(struct GameState* gamestate, int y, int x) {
	struct Bullets* bs = &gamestate->bullets;
	for (int i=0; i<bs->maxBullets; i++) {
		struct Bullet* crtB = &bs->bullet[i];
		if (crtB->active && crtB->y == y && crtB->x == x) {
			return crtB;
		}
	}
	return NULL;
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
		.player = {player1},
		.bullets.maxBullets = MAX_BULLETS,
		.bullets.i = 0
	};
	initGameBoard(gameState.gameBoard);

	struct termios old;
	configureTerminal(&old);

	while (isGameOn) {
		system("clear");

		for (int y=BOARD_Y - 1; y >= 0; y--) {
			for (int x=0; x<BOARD_X; x++) {
				char* symbol = NULL;
				struct ExplosionSquare explosionSquare = gameState.explosionMap[y][x];
				getItemAtXY(&symbol, &gameState, y, x);
				int* mapTerrainNum = &gameState.gameBoard[y][x];
				struct Bullet* bullet = getBulletAt(&gameState, y, x);
				if (explosionSquare.currentStage > 0) {
					printf("%s", explosionSymbols[1]);
				} else if (bullet) {
					if (MAP_OBSTRUCTION[*mapTerrainNum]) {
						triggerExplosion(&gameState, *mapTerrainNum, y, x);
						*mapTerrainNum = 0;
						bullet->active = 0;
						printf("%c", MAP_SYMBOL[*mapTerrainNum]);
					} else {
						printf("%c", bulletSymbol);
					}
				} else if (symbol) {
					printf("%s", symbol);
				} else {
					printf("%c", MAP_SYMBOL[*mapTerrainNum]);
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
		animateExplosions(&gameState);
		usleep(33000); // Sleep for 100ms to reduce CPU usage
	}
	return 0;
}

