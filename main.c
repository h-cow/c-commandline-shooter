#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

#define BOARD_X 27
#define BOARD_Y 20
#define NUMBER_OF_PLAYERS 4

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
	unsigned char gameBoard[BOARD_Y][BOARD_X];
	struct Player player[NUMBER_OF_PLAYERS];
};

static char TURRET_SYMBOL[8] = {'|', '/', '-', '\\', '|', '/', '-', '\\'};
static char MAP_SYMBOL[3] = {'#', '`', '%'};
char gamePieces[20] = {'`', '8'};

const char* getMapEntity(unsigned char id) {
	switch(id) {
		case 0:
			return "`";
			//return "\033[32m`\033[0m";
			break;
		case 1:
			return "%";
			break;
		default:
			return "?";
	}
}
/*
* Here are the turret directions
* 
* 7 0 1
* 6   2
* 5 4 3
 */
void calcTurret(struct Location* location, unsigned char x, unsigned char y, unsigned char dir) {
	if (location != NULL) {
		if (dir == 0 || dir == 1 || dir == 7) {
			location->y = y + 1;
		} else if (dir == 5 || dir == 4 || dir == 3) {
			location->y = y - 1;
		} else if (dir == 6 || dir == 2) {
			location->y = y;
		}

		if (dir == 1 || dir == 2 || dir == 3) {
			location->x = x + 1;
		} else if (dir == 7 || dir == 6 || dir == 5) {
			location->x = x - 1;
		} else if (dir == 0 || dir == 4) {
			location->x = x;
		}
	}
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

int main() {
	_Bool isGameOn = 1;

	struct Player player1;
	player1.y = 1;
	player1.x = 1;
	player1.dir = 0;
	

	struct GameState gameState = {
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
	int renderNumber = 0;

	struct Location player1Turret;

	while (isGameOn) {
		system("clear");
		printf("player x: %u player y: %u\n", gameState.player[0].x, gameState.player[0].y);
		printf("%d\n", renderNumber);

		calcTurret(&player1Turret, gameState.player[0].x, gameState.player[0].y, gameState.player[0].dir);

		renderNumber++;
		for (int y=BOARD_Y - 1; y >= 0; y--) {
			for (int x=0; x<BOARD_X; x++) {
				if (x == gameState.player[0].x && y == gameState.player[0].y) {
					printf("\033[34m0\033[0m");
				} else if (x == player1Turret.x && y == player1Turret.y) {
					printf("\033[34m%c\033[0m", TURRET_SYMBOL[gameState.player[0].dir]);
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

