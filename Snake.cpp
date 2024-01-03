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

Snake::Snake(const char *hostname, int port) : hostname(hostname), port(port), koniec(false)
{
    pthread_mutex_init(&mut, NULL);
    pthread_mutex_init(&mutDirection, NULL);
    pthread_mutex_init(&mutColision, NULL);

    direction = 'w';    //ini hlavy
    head.col = 5;
    head.row = 5;

    colision = false;

    for (int i = 0; i < 2; ++i)
    {
        SnakeSegment segment;
        segment.row = head.row + (i + 1);
        segment.col = head.col; // Adjust the col position based on the initial length
        body.push_back(segment);
    }

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
    pthread_mutex_destroy(&mutColision);
    pthread_mutex_destroy(&mutDirection);
    close(sockfd);
}

void Snake::run()
{
    startNonstopKeyStream();

    pthread_t tUserIn;
    pthread_create(&tUserIn, NULL, userInput, this);
    pthread_t tServerIn;
    pthread_create(&tServerIn, NULL, serverInput, this);


    runSnake();


    pthread_detach(tUserIn);
    pthread_detach(tServerIn);
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

            switch (c) {
                case 'w':
                case 'a':
                case 's':
                case 'd':
                    pthread_mutex_lock(&client->mutDirection);
                    client->direction = c;                          //nastavenie pohybu hraca
                    pthread_mutex_unlock(&client->mutDirection);
                    break;
                default:
                    break;
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
                client->koniec = true;
                pthread_mutex_unlock(&client->mut);
            }

            bzero(buffer, MSG_LEN);
        }

        pthread_mutex_lock(&client->mutColision);
        if (client->colision)           // kontrola kolizie hraca
        {
            buffer[1] = 'E';
            break;
        }
        pthread_mutex_unlock(&client->mutColision);

        pthread_mutex_lock(&client->mut);
        if (client->koniec)
        {
            pthread_mutex_unlock(&client->mut);
            break;
        }
        pthread_mutex_unlock(&client->mut);
    }

    n = write(client->sockfd, buffer, MSG_LEN);
    if (n < 0)
    {
        perror("Error writing to socket\n");
        return NULL;
    }


    return NULL;
}

void *Snake::serverInput(void *data) {
    Snake *client = static_cast<Snake *>(data);

    while (true)
    {
        char buffer[MSG_LEN];
        bzero(buffer, MSG_LEN);
        int n = read(client->sockfd, buffer, MSG_LEN);

        if (n < 0)
        {
            perror("Error reading from socket\n");
            exit(6);
        }

        client->processServerResponse(buffer);

        pthread_mutex_lock(&client->mut);
        if (client->koniec)
        {
            pthread_mutex_unlock(&client->mut);
            break;
        }
        pthread_mutex_unlock(&client->mut);
    }
    return NULL;
}

void Snake::runSnake() {

    initscr();      //nastavenie okna pre zobrazenie hry
    noecho();
    curs_set(FALSE);

    while (true) {
        moveSnake();        //logicky pohyb hada
        displaySnake();     //zobrazenie na terminaly
        refresh();

        pthread_mutex_lock(&mut);       //kontrola konca hry
        if (koniec) {
            pthread_mutex_unlock(&mut);
            break;
        }
        pthread_mutex_unlock(&mut);

        usleep(1000 * 1000); // Adjust the delay based on your game speed
    }

    endwin();       //nastavanie default nastaveni

    printf("\nkoniec zobrazovania snake\n");

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
            koniec = true;
            pthread_mutex_unlock(&mut);
            break;
        default:
            break;
    }
}

void Snake::moveSnake()
{
    int lastRow = head.row;
    int lastCol = head.col;
    switch (direction)
    {
        case 'w':
            head.row--;
            break;
        case 'a':
            head.col--;
            break;
        case 's':
            head.row++;
            break;
        case 'd':
            head.col++;
            break;
        default:
            perror("bad direction in moveSnake\n");
            break;
    }

    if ( head.row <= 0 || head.col <= 0 || head.row >= SIRKA_PLOCHY - 1 || head.col >= VYSKA_PLOCHY - 1) {
        pthread_mutex_lock(&mutColision);
        colision = true;
        pthread_mutex_unlock(&mutColision);

        pthread_mutex_lock(&mut);
        koniec = true;
        pthread_mutex_unlock(&mut);
    }

    //update body
    int tmpRow;
    int tmpCol;
    for (auto &segment : body)
    {
        tmpRow = lastRow;
        tmpCol = lastCol;
        lastRow = segment.row;
        lastCol = segment.col;
        segment.row = tmpRow;
        segment.col = tmpCol;

    }
}

void Snake::displaySnake()
{
    // Clear the previous state of the board
    for (int i = 0; i < SIRKA_PLOCHY; i++)
    {
        for (int j = 0; j < VYSKA_PLOCHY; j++)
        {
            board[i][j] = ' ';
        }
    }

    // Display the snake head
    board[head.row][head.col] = 'X';
//    mvprintw(head.row, head.col * 2, "O ");

    // Display the snake body
    for (const auto &segment : body)
    {
        board[segment.row][segment.col] = 'O';
//        mvprintw(segment.row, segment.col * 2, "o ");
    }

    // Display the board
    for (int i = 0; i < SIRKA_PLOCHY; i++)
    {
        for (int j = 0; j < VYSKA_PLOCHY; j++)
        {
            if (i == 0 || i == SIRKA_PLOCHY - 1 || j == 0 || j == VYSKA_PLOCHY - 1)
            {
                mvprintw(i, j * 2, "# ");
            }
            else
            {
                mvprintw(i, j * 2, "%c ", board[i][j]);
            }
        }
    }

    refresh();
}
