// Snake.h

#ifndef SNAKE_H
#define SNAKE_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <termios.h>
#include <ctype.h>

#define MSG_LEN 16

class Snake
{
public:
    Snake(const char *hostname, int port);
    ~Snake();

    void run();

private:
    void startNonstopKeyStream();
    void stopNonstopKeyStream();
    static void *userInput(void *data);
    static void *consoleOutput(void *data);
    void processServerResponse(char *buffer);

    const char *hostname;
    int port;
    int sockfd;
    bool konec;
    pthread_mutex_t mut;
};

#endif // SNAKE_H