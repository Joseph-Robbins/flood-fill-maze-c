#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#pragma warning(disable: 4996)

#define north 0
#define east 1
#define south 2
#define west 3

#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

struct oldCell
{
    unsigned short int score;
    bool leftWall;
    bool topWall;
    bool rightWall;
    bool bottomWall;
};

struct cell
{
    unsigned char score; //Floodfill score of the maze
    unsigned char walls; //To store info abt walls of the cell
    //unsigned char coords; //Coordinates of the cell in the maze
    bool visited; //Stores whether the maze has been visited (Required in simulation only)
};


struct robot
{
    char orientation;
    struct cell location;
    int xPos;
    int yPos;
};

struct stack
{
    int top;
    unsigned char entries[4];
};

void moveToAdjCell(struct robot* robot, unsigned char direction);
unsigned char setCoords(unsigned char xPos, unsigned char yPos);
void scoreMaze(struct cell* maze);
void scoreCurrentCell(struct cell* maze, struct robot robot, unsigned char* nextCellDirection);
void initMaze(struct cell* maze);

char readSensors(char charMaze[33][66], struct robot robot);
void moveForward(struct robot* robot);
void turnLeft(struct robot* robot);
void turnRight(struct robot* robot);
void turn180(struct robot* robot);
void push(struct stack* stack, unsigned char item);
void pop(struct stack* stack);
void readCharMaze(FILE* filePointer, char* charMaze);
void printCharMaze(char charMaze[33][66]);
void scoreCharMaze(struct cell maze[16][16]);
void printScoredMaze(char charMaze[33][66], struct cell maze[16][16]);
void printVisitedMaze(char charMaze[33][66], struct cell maze[16][16]);
void printScoredAndVisitedMazes(char charMaze[33][66], struct cell maze[16][16]);
void showVisited(struct cell maze[16][16]);
void initCells(char charMaze[33][66], struct cell* maze);


void main()
{
    FILE* filePointer;
    filePointer = fopen("mazes/Maze3.txt", "r");

    if (filePointer == NULL)
    {
        printf("Error! opening file");
    }

    else
    { 
        char charMaze[33][66]; //Create an array to hold all the characters from the maze text file
        struct cell maze[16][16]; //Create a 16x16 array to hold maze elements
        unsigned char nextCellDirection;

        //Put zero into the finish cells
        //Fill the rest of the cells with the most optimistic distance in cells to the goal (i.e. pretend there are no walls until found otherwise.)
        //Also set other variables to their starting values
        initMaze(&maze);

        //Print initial scored maze
        printf("Initial Maze: ");
        readCharMaze(filePointer, &charMaze);
        printScoredMaze(charMaze, maze);

        for (int r = 0; r < 4; r++) //Set number of runs to 3
        {
            unsigned int cellsVisited = 0;

            struct robot robot1; //Initialise a robot to go around the maze
            robot1.orientation = north;
            robot1.xPos = 0;
            robot1.yPos = 0;

            //Set each cells' visited value to its starting value after each run (not needed in actual dspic program)
            for (int i = 0; i < 16; i++)
            {
                for (int j = 0; j < 16; j++)
                    maze[i][j].visited = 0;
            }

            while (cellsVisited < 256) //While the robot has not visited all the cells
            {
                //Check if current cell is a goal cell
                if (maze[robot1.yPos][robot1.xPos].score == 0)
                    break;

                maze[robot1.yPos][robot1.xPos].walls = readSensors(charMaze, robot1); //Read sensors to gather information about the walls surrounding this cell
                scoreCurrentCell(maze, robot1, &nextCellDirection); //Give the current cell a score based on its accessible neighbours' scores
                scoreMaze(&maze); //Update the scores for all the maze's cells
                if (maze[robot1.yPos][robot1.xPos].visited == 0) //If this cell has notbeen visited before
                    cellsVisited++; 
                maze[robot1.yPos][robot1.xPos].visited = 1; //The current cell has now been visited
                moveToAdjCell(&robot1, nextCellDirection); //Move the robot to the next cell
            }

            printf("Run %d: ", r + 1); //Print what run we are currently on
            printScoredAndVisitedMazes(charMaze, maze); //Print the scored maze and the maze with the robots path next to each other
        }
        printf("Size of maze: %d", sizeof(maze));
    }           
}

unsigned char setCoords(unsigned char xPos, unsigned char yPos)
{
    return ((xPos << 4) + yPos);
}

unsigned char readXPos(unsigned char coords)
{
    return coords >> 4;
}

unsigned char readYPos(unsigned char coords)
{
    return coords % 16;
}

void initMaze(struct cell *maze)
{
    //Set the coordinates of each cell
    //for (unsigned char i = 0; i < 16; i++)
    //{
        //for (unsigned char j = 0; j < 16; j++)
            //(maze + i * 16 + j)->coords = setCoords(j, i);
    //}


    //Set each cells' walls value to its starting value
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
            (maze + i * 16 + j)->walls = 254;
    }

    //Set each cells' visited value to its starting value
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
            (maze + i * 16 + j)->visited = 0;
    }

    //Start at bottom left goal
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            (maze + (7 * 16) + 7 - j - i*16)->score = i + j;
        }
    }

    //Bottom right
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            (maze + (7 * 16) + 8 + j - i * 16)->score = i + j;
        }
    }

    //Top Left
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            (maze + (8 * 16) + 7 - j + i * 16)->score = j + i;
        }
    }

    //Top right
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            (maze + (8 * 16) + 8 + j + i * 16)->score = j + i;
        }
    }
}

void scoreMaze(struct cell* maze)
{
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++) //For each cell
        {
            struct stack neighbouringScores;
            neighbouringScores.top = -1;

            if ((maze + i * 16 + j)->walls != 254) //If wall data has been collected about this cell
            {
                if ((maze + i * 16 + j)->walls / 8 == 0) //If the left cell is accesible
                    push(&neighbouringScores, (maze + i * 16 + j - 1)->score); //Push the left cells' score on to the stack

                if (((maze + i * 16 + j)->walls / 4) % 2 == 0) //If the top cell is accesible
                    push(&neighbouringScores, (maze + i * 16 + j + 16)->score);

                if (((maze + i * 16 + j)->walls % 4) / 2 == 0) //If the right cell is accesible
                    push(&neighbouringScores, (maze + i * 16 + j + 1)->score);

                if ((maze + i * 16 + j)->walls % 2 == 0) //If the bottom cell is accesible
                    push(&neighbouringScores, (maze + i * 16 + j - 16)->score);
            }

            if (neighbouringScores.top > -1) //If the stack is not empty (if there are accessible neighbours)
            {
                //Find min value in stack
                unsigned short int min = neighbouringScores.entries[neighbouringScores.top];
                for (int k = 0; k < neighbouringScores.top + 1; k++) //For each value in the stack
                {
                    if (neighbouringScores.entries[k] < min) //If current score in stack is less than min
                        min = neighbouringScores.entries[k]; //Set min to current score in stack
                }

                if ((maze + i * 16 + j)->score != min + 1) //Is the cell's value one greater than the minimum value of its accessible neighbors?
                    (maze + i * 16 + j)->score = min + 1; //If no, change the cell's value to one greater than the minimum value of its accessible neighbors.          
            }
        }
    }
}

void scoreCurrentCell(struct cell* maze, struct robot robot, unsigned char* nextCellDirection)
{
    struct stack neighbouringScores;
    neighbouringScores.top = -1;

    unsigned char i = robot.yPos;
    unsigned char j = robot.xPos;

    if ((maze + i * 16 + j)->walls / 8 == 0) //If the west cell is accesible
        push(&neighbouringScores, (maze + i * 16 + j - 1)->score); //Push the left cells' score on to the stack

    if (((maze + i * 16 + j)->walls / 4) % 2 == 0) //If the north cell is accesible
        push(&neighbouringScores, (maze + i * 16 + j + 16)->score);

    if (((maze + i * 16 + j)->walls % 4) / 2 == 0) //If the east cell is accesible
        push(&neighbouringScores, (maze + i * 16 + j + 1)->score);

    if ((maze + i * 16 + j)->walls % 2 == 0) //If the south cell is accesible
        push(&neighbouringScores, (maze + i * 16 + j - 16)->score);

    if (neighbouringScores.top > -1) //If the stack is not empty (if there are accessible neighbours)
    {
        //Find min value in stack
        unsigned short int min = neighbouringScores.entries[neighbouringScores.top];
        for (int k = 0; k < neighbouringScores.top + 1; k++) //For each value in the stack
        {
            if (neighbouringScores.entries[k] < min) //If current score in stack is less than min
                min = neighbouringScores.entries[k]; //Set min to current score in stack
        }

        if ((maze + i * 16 + j)->score != min + 1) //Is the cell's value one greater than the minimum value of its accessible neighbors?
            (maze + i * 16 + j)->score = min + 1; //If no, change the cell's value to one greater than the minimum value of its accessible neighbors.

        //Check for walls again
        //Maybe push whole cells on to stack instead
        if ((maze + i * 16 + j + 16)->score == min && ((maze + i * 16 + j)->walls / 4) % 2 == 0) //If the cell north of the robot is the minimum score and is accessible
            *nextCellDirection = north; //Then move north next time to this cell

        else if ((maze + i * 16 + j + 1)->score == min && ((maze + i * 16 + j)->walls % 4) / 2 == 0) //If the east cell is the min
            *nextCellDirection = east;

        else if ((maze + i * 16 + j - 16)->score == min && (maze + i * 16 + j)->walls % 2 == 0) //South cell
            *nextCellDirection = south;

        else if ((maze + i * 16 + j - 1)->score == min && (maze + i * 16 + j)->walls / 8 == 0) //West cell
            *nextCellDirection = west;
    }

}

char readSensors(char charMaze[33][66], struct robot robot)
{
    //In the actual robot you would have to read different sensors depending on the orientation of the robot
    //E.g. if facing east you would have to read the left sensor to check for a north wall
    //Also the walls would still have to be in the same format (west wall == 8)
    char walls = 0;
    if (charMaze[32 - 2 * robot.yPos  - 1][robot.xPos * 4] == '|') //Check for west wall
        walls = walls + 8;
    if (charMaze[32 - 2 * robot.yPos - 2][2 + 4 * robot.xPos] == '-') //Check for north wall
        walls = walls + 4;
    if (charMaze[32 - 2 * robot.yPos - 1][4 + robot.xPos * 4] == '|') //Check for east wall
        walls = walls + 2;
    if (charMaze[32 - robot.yPos * 2][2 + 4 * robot.xPos] == '-') //Check for south wall
        walls++;

    return walls;
}

void moveToAdjCell(struct robot* robot, unsigned char direction)
{
    switch (direction - robot->orientation)
    {
    case -3:
        turnRight(robot);
        moveForward(robot);
        break;
    case -2:
        turn180(robot);
        moveForward(robot);
        break;
    case -1:
        turnLeft(robot);
        moveForward(robot);
        break;
    case 0:
        moveForward(robot);
        break;
    case 1:
        turnRight(robot);
        moveForward(robot);
        break;
    case 2:
        turn180(robot);
        moveForward(robot);
        break;
    case 3:
        turnLeft(robot);
        moveForward(robot);
        break;
    }
}

void moveForward(struct robot* robot)
{
    switch (robot->orientation)
    {
    case north:
        robot->yPos++;
        break;
    case east:
        robot->xPos++;
        break;
    case south:
        robot->yPos--;
        break;
    case west:
        robot->xPos--;
        break;
    }
}

void turnLeft(struct robot* robot)
{
    switch (robot->orientation)
    {
    case north:
        robot->orientation = west;
        break;
    case east:
        robot->orientation = north;
        break;
    case south:
        robot->orientation = east;
        break;
    case west:
        robot->orientation = south;
        break;
    }
}

void turnRight(struct robot* robot)
{
    switch (robot->orientation)
    {
    case north:
        robot->orientation = east;
        break;
    case east:
        robot->orientation = south;
        break;
    case south:
        robot->orientation = west;
        break;
    case west:
        robot->orientation = north;
        break;
    }
}

void turn180(struct robot* robot)
{
    switch (robot->orientation)
    {
    case north:
        robot->orientation = south;
        break;
    case east:
        robot->orientation = west;
        break;
    case south:
        robot->orientation = north;
        break;
    case west:
        robot->orientation = east;
        break;
    }
}

void push(struct stack* stack, unsigned char item)
{
    stack->top++;
    stack->entries[stack->top] = item;
}

void pop(struct stack* stack)
{
    stack->top--;
}

void readCharMaze(FILE* filePointer, char* charMaze)
{
    //Read the maze in to a character array
    for (int i = 0; i < 33; i++)
    {
        for (int j = 0; j < 66; j++)
        {
            char c = fgetc(filePointer);

            *(charMaze + 66 * i + j) = c;
        }
    }
}

void printCharMaze(char charMaze[33][66])
{
    for (int i = 0; i < 33; i++)
    {
        for (int j = 0; j < 66; j++)
        {
            printf("%c", charMaze[i][j]);
        }
    }
}

void scoreCharMaze(struct cell maze[16][16])
{
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            gotoxy(2 + j * 4, 32 - i * 2);
            printf("%d", maze[i][j].score);
        }
    }
}

void printScoredMaze(char charMaze[33][66], struct cell maze[16][16])
{
    printf("\033[2J");
    gotoxy(0, 0);
    printCharMaze(charMaze);
    scoreCharMaze(maze);
    printf("\033[2J");
    printf("\n\n");
}

void printVisitedMaze(char charMaze[33][66], struct cell maze[16][16])
{
    printf("\033[2J");
    gotoxy(0, 0);
    printCharMaze(charMaze);
    showVisited(maze);
    printf("\033[2J");
    printf("\n\n");
}

void printScoredAndVisitedMazes(char charMaze[33][66], struct cell maze[16][16])
{
    printf("\033[2J");
    gotoxy(0, 0);
    printCharMaze(charMaze);
    scoreCharMaze(maze);

    gotoxy(0, 0);
    for (int i = 0; i < 33; i++)
    {
        for (int j = 0; j < 66; j++)
        {
            gotoxy(70 + j, i + 1);
            printf("%c", charMaze[i][j]);
        }
    }

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            gotoxy(71 + j * 4, 32 - i * 2);
            if (maze[i][j].visited == 1)
                printf(" x ");
        }
    }
   
    printf("\033[2J");
    printf("\n\n");
}

void showVisited(struct cell maze[16][16])
{
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            gotoxy(2 + j * 4, 32 - i * 2);
            if (maze[i][j].visited == 1)
                printf(" x ");
        }
    }
}

/*void initCells(char charMaze[33][66], struct cell *maze)
{
    //Initialise all cells of the maze
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            (maze)->score = 10;

            //Score the current cell
            if (abs(i - 7.5) > abs(j - 7.5))
                (maze + 16 * i + j)->score = abs(i - 7.5);
            else
                (maze + (i * 16) + j)->score = abs(j - 7.5);

            if (charMaze[(1 + 2 * i)][4 * j] == '|') //Check for left wall
                (maze + (i * 16) + j)->leftWall = 1;
            else
                (maze + (i * 16) + j)->leftWall = 0;

            if (charMaze[2 * i][2 + 4 * j] == '-') //Check for top wall
                (maze + (i * 16) + j)->topWall = 1;
            else
                (maze + (i * 16) + j)->topWall = 0;

            if (charMaze[1 + 2 * i][4 + j * 4] == '|') //Check for right wall
                (maze + (i * 16) + j)->rightWall = 1;
            else
                (maze + (i * 16) + j)->rightWall = 0;

            if (charMaze[2 + 2 * i][2 + 4 * j] == '-') //Check for bottom wall
                (maze + (i * 16) + j)->bottomWall = 1;
            else
                (maze + (i * 16) + j)->bottomWall = 0;
        }
    }
    printf("%d", sizeof(char));
}*/



/*//Initialise all cells of the maze
    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            //Score the current cell
            if (abs(i - 7.5) > abs(j - 7.5))
                maze[i][j].score = abs(i - 7.5);
            else
                maze[i][j].score = abs(j - 7.5);

            if (charMaze[(1 + 2 * i)][4 * j] == '|') //Check for left wall
                maze[i][j].leftWall = 1;
            else
                maze[i][j].leftWall = 0;

            if (charMaze[2 * i][2 + 4 * j] == '-') //Check for top wall
                maze[i][j].topWall = 1;
            else
                maze[i][j].topWall = 0;

            if (charMaze[1 + 2 * i][4 + j * 4] == '|') //Check for right wall
                maze[i][j].rightWall = 1;
            else
                maze[i][j].rightWall = 0;

            if (charMaze[2 + 2 * i][2 + 4 * j] == '-') //Check for bottom wall
                maze[i][j].bottomWall = 1;
            else
                maze[i][j].bottomWall = 0;
        }
    }*/

    /*
         struct cell2 maze2[16][16]; //Stack test
         maze2[4][4].score = 10;
         maze2[4][4].walls = 0b0101;
         maze2[1][2].score = 14;
         maze2[1][2].walls = 0b1111;
         push(&openList, maze2[4][4]);
         push(&openList, maze2[1][2]);
         pop(&openList); */

         /*//Gather information about the cell the robot is in
                     if (charMaze1[robot1.yPos * 2 + 1][robot1.xPos * 4] == '|') //Check for left wall
                         maze1[robot1.yPos][robot1.xPos].leftWall = 1;
                     else
                     {
                         maze1[robot1.yPos][robot1.xPos].leftWall = 0;
                         //Add maze1[ypos][xpos-1] to open list

                         //Score the adjacent cell with no wall inbetween 1 higher than the current cell
                         maze1[robot1.yPos][robot1.xPos - 1].score = maze1[robot1.yPos][robot1.xPos].score + 1;
                     }

                     if (charMaze1[robot1.yPos * 2][2 + 4 * robot1.xPos] == '-')
                         maze1[robot1.yPos][robot1.xPos].topWall = 1;
                     else
                         maze1[robot1.yPos][robot1.xPos].topWall = 0;

                     if (charMaze1[1 + 2 * robot1.yPos][4 + robot1.xPos * 4] == '|')
                         maze1[robot1.yPos][robot1.xPos].rightWall = 1;
                     else
                         maze1[robot1.yPos][robot1.xPos].rightWall = 0;

                     if (charMaze1[2 + 2 * robot1.yPos][2 + 4 * robot1.xPos] == '-')
                         maze1[robot1.yPos][robot1.xPos].bottomWall = 1;
                     else
                         maze1[robot1.yPos][robot1.xPos].bottomWall = 0;*/