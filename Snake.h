// Snake.h

#ifndef SNAKE_H
#define SNAKE_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <curses.h>
#include <termios.h>
#include <ctype.h>
#include <vector>
#include <string>

#define MSG_LEN 16
#define SIRKA_PLOCHY 10
#define VYSKA_PLOCHY 10

class Snake
{
public:
    Snake(const char *hostname, int port);
    ~Snake();

    void run();
    void runSnake();
    void moveSnake();
    void displaySnake();
    void generateFruit();

private:
    void startNonstopKeyStream();
    void stopNonstopKeyStream();
    static void *userInput(void *data);
    static void *consoleOutput(void *data);
    static void *serverInput(void *data);
    void processServerResponse(char *buffer);
    void displayScore();

    const char *hostname;
    int port;
    int sockfd;
    bool koniec;
    pthread_mutex_t mut;
    pthread_mutex_t mutDirection;
    pthread_mutex_t mutColision;
    pthread_mutex_t mutMove;
    pthread_cond_t conWait;
    pthread_cond_t conMove;

    pthread_mutex_t mutTEst;
    bool daco = false;

    struct SnakeSegment
    {
        int row;
        int col;
    };

    char direction; // 'w' for up, 'a' for left, 's' for down, 'd' for right
    char prevDirection;
    SnakeSegment head;
    std::vector<SnakeSegment> body;
    SnakeSegment fruit;
    char board[SIRKA_PLOCHY][VYSKA_PLOCHY];
    bool colision;
    int score;
    bool gameIteration;
};

#endif // SNAKE_H