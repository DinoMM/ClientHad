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
    void moveSnakeEnemy();
    void displaySnake();
    void displaySnakeEnemy();
    void generateFruit();
    void generateFruitEnemy();

private:
    void startNonstopKeyStream();
    void stopNonstopKeyStream();
    static void *userInput(void *data);
    //static void *consoleOutput(void *data);
    static void *serverInput(void *data);
    void processServerResponse(char *buffer);
    void displayScore();

    const char *hostname;
    int port;
    int sockfd;
    bool koniec;
    pthread_mutex_t mut;
    pthread_mutex_t mutDirection;
    pthread_mutex_t mutDirectionEnemy;
    pthread_mutex_t mutFruitEnemy;
    pthread_mutex_t mutEndEnemy;
    pthread_mutex_t mutColision;
    pthread_mutex_t mutMove;
    pthread_cond_t conWait;
    pthread_cond_t conMove;

    pthread_mutex_t mutTEst;
    bool daco = false; 

    struct Policko
    {
        int row;
        int col;
    };

    char direction;  // 'w' hore, 'a' do lava 's' dole, 'd' do prava
    char prevDirection;
    Policko head;
    std::vector<Policko> body;
    Policko fruit;
    char board[SIRKA_PLOCHY][VYSKA_PLOCHY];
    bool colision; // ?
    int score;
    bool gameIteration;

    char directionEnemy;  // 'w' hore, 'a' do lava 's' dole, 'd' do prava
    Policko headEnemy;
    std::vector<Policko> bodyEnemy;
    Policko fruitEnemy;
    char boardEnemy[SIRKA_PLOCHY][VYSKA_PLOCHY];
    bool endEnemy;
};

#endif // SNAKE_H