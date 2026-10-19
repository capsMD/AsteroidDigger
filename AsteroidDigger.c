/*
Carlos Pinheiro, 2016
HAW Hamburg – Hamburg University of Applied Sciences
Course: Software Construction 1 (SO1/SOL1)
Lecturer: Prof. Dr. Ulrich Sauvagerd
*/



#define _CRT_SECURE_NO_WARNINGS
#define WIDTH 32
#define HEIGHT 16
#define MAPEXTENSION 200
#define INTERVALEXTENSION 25



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <string.h>

static struct termios orig_term;

static void restoreTerminal(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

static int kbhit(void)
{
	struct timeval tv = { 0, 0 };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

static int getch(void)
{
	unsigned char c;
	return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}

static int nowMs(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#define CLS_DELAY_MS 45

static int portSystem(const char *cmd)
{
	if (strcmp(cmd, "cls") == 0)
	{
		usleep(CLS_DELAY_MS * 1000);
		return system("clear");
	}
	if (strcmp(cmd, "pause") == 0)
	{
		printf("Press any key to continue . . .");
		getch();
		return 0;
	}
	return system(cmd);
}

__attribute__((constructor)) static void portInit(void)
{
	struct termios t;
	setvbuf(stdout, NULL, _IONBF, 0);
	tcgetattr(STDIN_FILENO, &orig_term);
	atexit(restoreTerminal);
	t = orig_term;
	t.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

#define system portSystem
#define clock nowMs
#define Sleep(ms) usleep((ms) * 1000)

void getKeyboardInput(unsigned char *, char[]);
void fire(unsigned char *, char[], char[], unsigned char *, char *, char[][WIDTH], char *);
void terrainGenerator(char[][WIDTH], int[], char *, char *, char *, short int *, short int *, int *);
void terrainMover(char[][WIDTH], int[], short int *);
void renderScreen(char[][WIDTH], int[], char[], char *, short int *, char *, short int *, int *, char[], unsigned char *, char[][WIDTH], char *);
void checkCollision(char[][WIDTH], char[], char *, unsigned char *, char[], unsigned char *);
void startScreen();
void EndScreen(short int *);
void displayStartTimer();
void displayCrash();



clock_t start, end;
double elapsed;






void main()
{

	//arrays
	char terrainArray[HEIGHT][WIDTH] = { 0 };
	int dimensions[] = { HEIGHT, WIDTH };
	char firePosition[] = { HEIGHT, WIDTH };
	char shipPosition[] = { HEIGHT / 2, WIDTH / 2 };
	char frameBuffer[HEIGHT][WIDTH] = { 0 };

	char pause = 0, play = 1, draw = 0, lives = 3, level = 1, horizonLineHeight = HEIGHT / 2, ammo = 50, fireMove = 0;
	unsigned char fireButton = 0, fireOn = 0;
	short int levelDistance = 0, score = 0, timerInt = 0, interval = 10, speedMultiplier = 1;

	int start, end, deltaTimeInt = 0;

	double elapsed;

	//-----------------------------------------------------------------------------------------------------------------------


	//introduction screen





	startScreen();


	//run until has lives basically
	while (lives != 0)
	{
		//get the value of time when starting frame
		start = clock();


		getKeyboardInput(&fireButton, shipPosition);



		//generate terrain
		terrainGenerator(terrainArray, dimensions, &horizonLineHeight, &level, &draw, &timerInt, &speedMultiplier, &deltaTimeInt);
		if (fireButton)
		{
			if (ammo>0)
			{
				fire(&fireButton, shipPosition, firePosition, &fireOn, &ammo, terrainArray, &fireMove);
			}
		}


		//check collision
		checkCollision(terrainArray, shipPosition, &lives, &fireOn, firePosition, &fireButton);




		if (timerInt > (interval * 4))
		{
			//game control
			levelDistance++;
			//score 
			score++;
		}

		//render
		renderScreen(terrainArray, dimensions, shipPosition, &lives, &score, &level, &timerInt, &deltaTimeInt, firePosition, &fireOn, frameBuffer, &ammo);



		//for every level, there is an intro(dont draw terrain), the playable level itself(draw terrain), and an end (dont draw terrain)
		//these are used to pass level, by the distance traveled and depending on intervals, determine section of level


		if (levelDistance < INTERVALEXTENSION)
		{
			draw = 0;
		}

		else if ((levelDistance > INTERVALEXTENSION) && (levelDistance < MAPEXTENSION))
		{
			draw = 1;
		}

		else if ((levelDistance > MAPEXTENSION) && (levelDistance < (MAPEXTENSION + INTERVALEXTENSION)))
		{
			draw = 0;
		}

		else if (levelDistance >(MAPEXTENSION + INTERVALEXTENSION))
		{

			level++;
			levelDistance = 0;
		}




		//get final time after running one frame
		end = clock();

		//time per frame
		elapsed = ((double)(end - start));

		//time in int value
		deltaTimeInt = ((int)elapsed);
		//printf("%d", timerInt);
		//printf("%d", pause);
		timerInt += deltaTimeInt;

		//if(level >5)
		//{
		//}


	}

	// end of game, clear screen and bring end level/score screen up
	system("cls");

	EndScreen(&score);

	system("pause");
}

void getKeyboardInput(unsigned char * fireButton, char shipPosition[])
{

	char keyPressed;


	//get input from keyboard
	fflush(stdin);

	//move up, down, left, right
	if (kbhit())
	{

		keyPressed = getch();
		switch (keyPressed)
		{
		case 'a':
		{	if (shipPosition[1]>0)
			shipPosition[1]--;
		break;
		}
		case 'd':
		{
			if (shipPosition[1]<WIDTH - 1)
				shipPosition[1]++;
			break;
		}
		case 'w':
		{
			if (shipPosition[0]>0)
				shipPosition[0]--;
			break;
		}
		case 's':
		{
			if (shipPosition[0]<HEIGHT - 1)
				shipPosition[0]++;
			break;
		}
		case ' ':
		{
			if (!(*fireButton))
			{
				*fireButton = 1;
			}
			break;
		}




		}

	}

}

void fire(unsigned char * fireButton, char shipPosition[], char firePosition[], unsigned char * fireOn, char * ammo, char terrainArray[HEIGHT][WIDTH], char * fireMove)
{
	int i;

	if ((*ammo)>0)
	{


		if (!(*fireOn))
		{
			//as soon as fire button pressed and checked conditions to fire
			*fireOn = 1;
			*firePosition = *shipPosition;
			*(firePosition + 1) = *(shipPosition + 1) + 1;
			*fireMove = 0;
			//
			for (i = *(firePosition + 1); i < WIDTH; i++)
			{
				if (terrainArray[*firePosition][i] == '#')
				{
					*fireMove = i;
					break;
				}
			}


			(*ammo)--;
		}
		else
		{


			if (firePosition[1] < WIDTH - 2)
			{
				(*(firePosition + 1))++;

				if (*fireMove != 0)
				{

					if (terrainArray[*firePosition][*(firePosition + 1)] == '#')
					{


						terrainArray[*firePosition][*(firePosition + 1)] = '.';
						*fireOn = 0;
						*fireButton = 0;
						*firePosition = *shipPosition;

					}


				}
			}
			else
			{
				*fireOn = 0;
				*fireButton = 0;
				*firePosition = *shipPosition;
			}
		}

	}
}

void terrainGenerator(char terrainArray[HEIGHT][WIDTH], int dimensions[], char * horizonLineHeight, char * level, char * draw, short int *timerInt, short int * speedMultiplier, int * deltaTime)
{
	int counterRow;
	int counterColumn;
	float slopeIncrement = 0;
	char terrainTextureArray[] = { '#', '*', '0', '.', '8', '0', '>' };
	int tunelRadius = HEIGHT / 4;
	int interval = 120;


	//*timerInt += *deltaTime;

	if (*timerInt > interval)
	{
		//*timerInt = 0;

		if (*draw)
		{

			//generate unique column (first vertical line from array) of terrain.

			//generating the slope variation:

			//create random signed numbers
			slopeIncrement = (float)sin(((4 * atan(1)) / (0.001 + (rand() % 4) / 2)));

			//terrain will vary by the randomness of these numbers put togehter, always incrementing/decrementing 1 and -1
			*horizonLineHeight += (int)(slopeIncrement / fabs(slopeIncrement));

			//set max and min values
			*horizonLineHeight = ((*horizonLineHeight) < (HEIGHT * 0.25)) ? (HEIGHT / 3) : *horizonLineHeight;
			*horizonLineHeight = ((*horizonLineHeight) > (HEIGHT * 0.75)) ? (HEIGHT * 0.75) : *horizonLineHeight;


			//Set values on terrain array for height of next iteration (always the last columns of array).

			for (counterRow = 0; counterRow < dimensions[1]; counterRow++)
			{
				//set radius for tunel
				tunelRadius = HEIGHT / (*level + 1);


				//set texture sky
				if ((counterRow >(*horizonLineHeight) - tunelRadius) && (counterRow < (*horizonLineHeight) + tunelRadius))

					terrainArray[counterRow][WIDTH - 1] = *(terrainTextureArray + 3);

				//set texture ground
				else
					terrainArray[counterRow][WIDTH - 1] = *(terrainTextureArray);



				//remove spikes
				//n-2 element is 'sky'									n-1 is 'rock'
				if (((terrainArray[counterRow][WIDTH - 3]) == *(terrainTextureArray + 3)) && (terrainArray[counterRow][WIDTH - 2] == *(terrainTextureArray)))
				{
					//if current element is 'sky' again, there is a spike, remove it
					if (terrainArray[counterRow][WIDTH - 1] == *(terrainTextureArray + 3))
					{
						terrainArray[counterRow][WIDTH - 3] = *(terrainTextureArray);
					}
				}


			}
		}

		else {
			for (counterRow = 0; counterRow < dimensions[1]; counterRow++)
			{
				//set texture sky
				terrainArray[counterRow][WIDTH - 1] = *(terrainTextureArray + 3);
			}

		}

		for (counterColumn = 0; counterColumn < dimensions[1] - 1; counterColumn++)
		{
			for (counterRow = 0; counterRow < dimensions[0]; counterRow++)
			{

				*(*(terrainArray + counterRow) + counterColumn) = *(*(terrainArray + counterRow) + counterColumn + 1);
			}

		}

		//shiftAgain--;
		//terrainGenerator(terrainArray, dimensions, &horizonLineHeight, &level, &draw, &timerInt, &speedMultiplier, shiftAgain);






	}
}

void renderScreen(char terrainArray[HEIGHT][WIDTH], int dimensions[], char shipPosition[], char * lives, short int * score, char * level, short int * timerInt, int * deltaTime, char firePosition[], unsigned char * fireOn, char frameBuffer[HEIGHT][WIDTH], char * ammo)
{
	int counterRow = 0;
	int counterColumn = 0;
	int interval = 120;

	system("cls");



	//write terrain to frame buffer
	for (counterRow = 0; counterRow < dimensions[0]; counterRow++)
	{
		//terrain
		for (counterColumn = 0; counterColumn < dimensions[1]; counterColumn++)
		{
			frameBuffer[counterRow][counterColumn] = terrainArray[counterRow][counterColumn];
		}
	}

	//write missle on frameBuffer
	if (*fireOn)
	{
		frameBuffer[firePosition[0]][firePosition[1]] = '-';
	}

	//write ship on frameBuffer
	frameBuffer[shipPosition[0]][shipPosition[1]] = '>';


	//print all
	printf("\n\n\n");

	printf("\t\tLives:%d Ammo:%d Level:%d SCORE:%d", *lives, *ammo, *level, *score);
	for (counterRow = 0; counterRow < dimensions[0]; counterRow++)
	{
		printf("\n");
		printf("\t\t");

		for (counterColumn = 0; counterColumn < dimensions[1]; counterColumn++)
		{
			printf("%c", *(*(frameBuffer + counterRow) + counterColumn));
		}
	}


	if (*timerInt > interval)
	{
		*timerInt = 0;
	}
}

void checkCollision(char terrainArray[HEIGHT][WIDTH], char shipPosition[], char * lives, unsigned char * fireOn, char firePosition[], unsigned char * fireButton)
{
	int i;
	int j;
	int count = 0;

	/*
	if(*fireOn)
	{
	printf("%d", firePosition[1]);
	if(terrainArray[firePosition[0]][firePosition[1]] == '#')
	{
	for(i=0; i< 8; i++)
	{
	if(terrainArray[firePosition[0]][firePosition[1]-i] == '#')
	terrainArray[firePosition[0]][firePosition[1]-i] = '.';

	else
	terrainArray[firePosition[0]][firePosition[1]] = '.';
	}
	*firePosition = *shipPosition;
	*fireButton = 0;
	*fireOn = 0;
	}
	}

	*/

	// if the ship's position in the terrain matrix has a "ground", collided
	if (terrainArray[shipPosition[0]][shipPosition[1]] == '#')
	{
		(*lives)--;
		printf("\a");
		//* ppause = 1;


		displayCrash();
		//reset ship position, half tunel's height
		for (i = 0; i< HEIGHT - 1; i++)
		{
			if (terrainArray[i][shipPosition[1]] == '.')
			{
				for (j = 0; terrainArray[i + j][shipPosition[1]] == '.'; j++)

					shipPosition[0] = (i + ((int)(j / 2)));
				break;
			}
		}
	}
}

void displayCrash()
{
	fflush(stdin);
	system("cls");
	printf("\n\n\n\n\n");
	printf("\t\t\tYOU CRASHED");
	Sleep(2000);
	//displayStartTimer();
}

void startScreen()
{

	fflush(stdin);

	printf("\t=========================================================");
	printf("\n");

	printf("\n                                                                                    ");
	printf("\n        FLY THROUGH THE ASTEROID CAVES, AVOID COLLISIONS AND IF NECESSARY           ");
	printf("\n                             FIRE YOUR WAY THROUGH!                                 ");

	printf("\n\n");
	printf("\n                                 CONTROLS (case sensitive):                         ");
	printf("\n\n");
	printf("\n                                 w - > move up                                      ");
	printf("\n                                 s - > move down                                    ");
	printf("\n                                 a - > move left                                    ");
	printf("\n                                 d - > move right                                   ");
	printf("\n                         space bar - > fire torpedo                                 ");
	printf("\n\n");
	printf("                                PRESS KEY TO START                             ");

	printf("\n\n");
	printf("\t=========================================================");


	while (!kbhit())
	{
	}





}

void EndScreen(short int * score)
{


	system("cls");



	printf("\t=========================================================");
	printf("\n\n\n\n\n");


	printf("\n                                 GAME OVER!                               ");

	printf("\n\n\n\n");
	printf("\n                                 YOUR SCORE:");
	printf("\n\n");
	printf("\n\n                                    %d", *score);
	printf("\n\n");
	printf("\n\n");
	printf("\n\n");
	printf("\n\n");
	printf("\t=========================================================");

	fflush(stdin);
	while (!kbhit())
	{
	}



}
