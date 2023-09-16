#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define BOARD_WIDTH 64
#define BOARD_HEIGHT BOARD_WIDTH / 4
#define REFRESH_TIME_X 60000
#define REFRESH_TIME_Y REFRESH_TIME_X * 1.5
// #define REFRESH_TIME_X 1200000
// #define REFRESH_TIME_Y 1200000

const char SNAKE_BODY = 'O';
const char EMPTY_SPACE = ' ';
const char BORDER = '#';
const char FOOD = '*';

typedef enum
{
    EXIT,
    UP,
    DOWN,
    LEFT,
    RIGHT
} GameAction;

typedef struct {
    int x;
    int y;
} SnakePixel;

char getInputNonBlocking()
{
    struct termios oldTerminal, newTerminal;
    char pressedButton;
    int oldFileDescription;

    tcgetattr(STDIN_FILENO, &oldTerminal);
    newTerminal = oldTerminal;

    newTerminal.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminal);
    oldFileDescription = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldFileDescription | O_NONBLOCK);

    pressedButton = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminal);
    fcntl(STDIN_FILENO, F_SETFL, oldFileDescription);

    return pressedButton;
}

char **generateBoard(int width, int height)
{
    char **board = malloc(height * sizeof(char *));
    if (!board)
    {
        exit(1);
    }
    for (int i = 0; i < height; i++)
    {
        board[i] = malloc(width * sizeof(char));
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (i == 0 || i == height - 1 || j == 0 || j == width - 1)
            {
                board[i][j] = BORDER;
            }
            else
            {
                board[i][j] = EMPTY_SPACE;
            }
        }
    }

    return board;
}

void printBoard(char **board, int width, int height)
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            printf("%c", board[i][j]);
        }
        printf("\n");
    }
}

GameAction getGameActionFromInput(char input)
{
    switch (input)
    {
    case 'x':
        return EXIT;
    case 'a':
        return LEFT;
    case 'd':
        return RIGHT;
    case 'w':
        return UP;
    case 's':
        return DOWN;
    default:
        return -1;
    }
}

int randomInRange(int min, int max)
{
    return (min + rand() % (max - min + 1));
}

void generateNewFood(char **board)
{
    int newY = randomInRange(1, BOARD_HEIGHT - 2);
    int newX = randomInRange(1, BOARD_WIDTH - 2);
    while (board[newY][newX] == SNAKE_BODY) {
        newY = randomInRange(1, BOARD_HEIGHT - 2);
        newX = randomInRange(1, BOARD_WIDTH - 2);
    }
    board[newY][newX] = FOOD;
}

void growSnake(SnakePixel** snake, int* snakeSize) {
    *snakeSize += 1;
    *snake = realloc(*snake, (*snakeSize) * sizeof(SnakePixel));
    (*snake)[*snakeSize-1].x = (*snake)[*snakeSize-2].x;
    (*snake)[*snakeSize-1].y = (*snake)[*snakeSize-2].y;
}

void handleFoodPos(char **board, int x, int y, int *score, SnakePixel** snake, int* snakeSize)
{
    if (board[y][x] == FOOD)
    {
        (*score)++;
        growSnake(snake, snakeSize);
        generateNewFood(board);
    }
}

void validatePos(char **board, bool *isRunning, SnakePixel snake[])
{
    bool hitsWall = !(snake[0].x > 0 && snake[0].x < BOARD_WIDTH - 1 && snake[0].y > 0 && snake[0].y < BOARD_HEIGHT - 1);
    bool hitsSnakePixel = board[snake[0].y][snake[0].x] == SNAKE_BODY;
    if (hitsWall || hitsSnakePixel)
    {
        *isRunning = false;
    }
}

bool actionIsValid(GameAction action, GameAction validActions[], int size)
{
    for (int i = 0; i < size; i++)
    {
        if (action == validActions[i])
        {
            return true;
        }
    }
    return false;
}

void moveSnakeBody(SnakePixel snake[], int snakeSize) {
    for (int i = snakeSize-1; i > 0; i--) {
        snake[i] = snake[i-1];
    }
}

void drawSnake(char **board, SnakePixel snake[], int snakeSize) {
    for (int i = 0; i < snakeSize; i++) {
        board[snake[i].y][snake[i].x] = SNAKE_BODY;
    }
}

void handleGameActions(
    GameAction action,
    GameAction allowedActions[],
    int allowedActionsSize,
    GameAction *lastAction,
    bool *isRunning,
    int *refreshTime,
    SnakePixel snake[]
    )
{
    if (actionIsValid(action, allowedActions, allowedActionsSize))
    {
        *lastAction = action;
    }

    switch (*lastAction)
    {
    case EXIT:
        *isRunning = false;
        break;
    case LEFT:
        *refreshTime = REFRESH_TIME_X;
        snake[0].x--;
        break;
    case RIGHT:
        *refreshTime = REFRESH_TIME_X;
        snake[0].x++;
        break;
    case UP:
        *refreshTime = REFRESH_TIME_Y;
        snake[0].y--;
        break;
    case DOWN:
        *refreshTime = REFRESH_TIME_Y;
        snake[0].y++;
        break;
    }
}

void processFrame (char **board, SnakePixel snake[], int snakeSize, int refreshTime) {
    system("clear");
    drawSnake(board, snake, snakeSize);
    printBoard(board, BOARD_WIDTH, BOARD_HEIGHT);
    usleep(refreshTime);
}

SnakePixel* initSnake (int* size) {
    int snakeSize = 1;
    SnakePixel* snake = malloc(snakeSize * sizeof(SnakePixel));
    if (!snake) {
        exit(1);
    }
    snake[0].x = 1;
    snake[0].y = 1;
    *size = snakeSize;
    return snake;
}

void cleanUpTail (char **board, SnakePixel snake[], int snakeSize) {
    board[snake[snakeSize-1].y][snake[snakeSize-1].x] = EMPTY_SPACE;
}

int main()
{
    srand(time(NULL));

    char **board = generateBoard(BOARD_WIDTH, BOARD_HEIGHT);
    generateNewFood(board);
    int snakeSize;
    SnakePixel* snake = initSnake(&snakeSize);

    GameAction allowedActions[] = {EXIT, UP, DOWN, LEFT, RIGHT};
    int allowedActionsSize = sizeof(allowedActions) / sizeof(allowedActions[0]);
    GameAction lastAction = RIGHT;

    int refreshTime = REFRESH_TIME_X;
    bool isRunning = true;
    int score = 0;

    processFrame(board, snake, snakeSize, refreshTime);
    while (isRunning)
    {   
        cleanUpTail(board, snake, snakeSize);
        moveSnakeBody(snake, snakeSize);

        char inputChar = getInputNonBlocking();
        GameAction action = getGameActionFromInput(inputChar);
        handleGameActions(action, allowedActions, allowedActionsSize, &lastAction, &isRunning, &refreshTime, snake);

        validatePos(board, &isRunning, snake);
        handleFoodPos(board, snake[0].x, snake[0].y, &score, &snake, &snakeSize);

        processFrame(board, snake, snakeSize, refreshTime);
    }

    for (int i = 0; i < BOARD_HEIGHT; i++)
    {
        free(board[i]);
    }
    free(board);
    free(snake);
    board = NULL;
    snake = NULL;

    system("clear");
    printf("Game Over!\n");
    printf("Your score is: %d\n", score);
    return 0;
}
