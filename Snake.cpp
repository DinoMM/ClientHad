// Snake.cpp

#include "Snake.h"
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

Snake::Snake(const char *hostname, int port) : hostname(hostname), port(port), konec(false)
{
    pthread_mutex_init(&mut, NULL);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("Error creating socket");
        exit(3);
    }

    struct hostent *server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        exit(2);
    }

    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting to socket");
        exit(4);
    }
}

Snake::~Snake()
{
    pthread_mutex_destroy(&mut);
    close(sockfd);
}

void Snake::run()
{
    startNonstopKeyStream();

    pthread_t thr;
    pthread_create(&thr, NULL, userInput, this);
    pthread_t tConsole;
    pthread_create(&tConsole, NULL, consoleOutput, this);

    while (true)
    {
        char buffer[MSG_LEN];
        bzero(buffer, MSG_LEN);
        int n = read(sockfd, buffer, MSG_LEN);

        if (n < 0)
        {
            perror("Error reading from socket\n");
            exit(6);
        }

        processServerResponse(buffer);

        pthread_mutex_lock(&mut);
        if (konec)
        {
            pthread_mutex_unlock(&mut);
            break;
        }
        pthread_mutex_unlock(&mut);
    }

    pthread_detach(thr);
    pthread_join(tConsole, NULL);
    stopNonstopKeyStream();

    printf("Finalny koniec hry\n");
}

void Snake::startNonstopKeyStream()
{
    struct termios menicVstupuTerminalu;
    tcgetattr(STDIN_FILENO, &menicVstupuTerminalu);
    menicVstupuTerminalu.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &menicVstupuTerminalu);
}

void Snake::stopNonstopKeyStream()
{
    struct termios menicVstupuTerminalu;
    tcgetattr(STDIN_FILENO, &menicVstupuTerminalu);
    menicVstupuTerminalu.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &menicVstupuTerminalu);
}

void *Snake::userInput(void *data)
{
    Snake *client = static_cast<Snake *>(data);

    char c;
    int vymaztoto = 0;
    char buffer[MSG_LEN];
    int n;

    bzero(buffer, MSG_LEN);

    while (true)
    {
        if (read(STDIN_FILENO, &c, 1) > 0)
        {
            c = tolower(c);
            buffer[0] = c;
            vymaztoto++;

            if (vymaztoto == 5)
            {
                buffer[1] = 'E';
                c = 'r';
            }

            n = write(client->sockfd, buffer, MSG_LEN);

            if (n < 0)
            {
                perror("Error writing to socket\n");
                return NULL;
            }

            if (c == 'r')
            {
                pthread_mutex_lock(&client->mut);
                client->konec = true;
                pthread_mutex_unlock(&client->mut);
            }

            bzero(buffer, MSG_LEN);
        }

        pthread_mutex_lock(&client->mut);
        if (client->konec)
        {
            pthread_mutex_unlock(&client->mut);
            break;
        }
        pthread_mutex_unlock(&client->mut);
    }

    return NULL;
}

void *Snake::consoleOutput(void *data)
{
    Snake *client = static_cast<Snake *>(data);

    initscr();
    noecho();
    curs_set(FALSE);

    const int rows = 10;
    const int cols = 10;
    char board[rows][cols];

    // Initialize the board
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            board[i][j] = ' ';
        }
    }
    while (true)
    {
        clear();

        // Update the board based on server response or game logic
        // For now, just display a snake head at a fixed position
        board[5][5] = 'O';
        // Display the board
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (i == 0 || i == rows - 1 || j == 0 || j == cols - 1) {
                    // Display border for the edges of the board
                    mvprintw(i, j * 2, "# ");
                } else {
                    if (board[i][j] == ' ') {
                        mvprintw(i, j * 2, ". ");
                    } else {
                        mvprintw(i, j * 2, "%c ", board[i][j]);
                    }
                }
            }
        }
                refresh();
        sleep(2); // Adjust the delay based on your game speed

        pthread_mutex_lock(&client->mut);
        if (client->konec)
        {
            pthread_mutex_unlock(&client->mut);
            break;
        }
        pthread_mutex_unlock(&client->mut);
    }

    endwin();

    printf("\nkoniec writeu\n");

    return NULL;
}

void Snake::processServerResponse(char *buffer)
{
    switch (buffer[2])
    {
        case 'w':
        case 'a':
        case 's':
        case 'd':
            break;
        default:
            break;
    }

    switch (buffer[1])
    {
        case 'E':
            pthread_mutex_lock(&mut);
            konec = true;
            pthread_mutex_unlock(&mut);
            break;
        default:
            break;
    }
}
