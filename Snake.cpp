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

int vymaz = 0;
int vymaz2 = 0;

Snake::Snake(const char *hostname, int port) : hostname(hostname), port(port), koniec(false)
{
    pthread_mutex_init(&mut, NULL);
    pthread_mutex_init(&mutDirection, NULL);
    pthread_mutex_init(&mutDirectionEnemy, NULL);
    pthread_mutex_init(&mutFruitEnemy, NULL);
    pthread_mutex_init(&mutEndEnemy, NULL);
    pthread_mutex_init(&mutColision, NULL);
    pthread_mutex_init(&mutMove, NULL);
    pthread_cond_init(&conMove, NULL);
    pthread_cond_init(&conWait, NULL);

    //AKTUALNY HRAC
    direction = 'w';    //ini hlavy hraca
    head.col = 5;
    head.row = 5;
    fruit.col = 0;
    fruit.row = 0;
    for (int i = 0; i < 4; ++i) //ini tela hraca
    {
        Policko policko;
        policko.row = head.row + (i + 1);
        policko.col = head.col;
        body.push_back(policko);
    }

    //NEPRIATEL
    directionEnemy = 'w';    //ini hlavy nepriatela
    headEnemy.col = 5;
    headEnemy.row = 5;
    fruitEnemy.col = 0;
    fruitEnemy.row = 0;
    endEnemy = false;
    for (int i = 0; i < 4; ++i) //ini tela nepriatela
    {
        Policko polickoEnemy;
        polickoEnemy.row = headEnemy.row + (i + 1);
        polickoEnemy.col = headEnemy.col;
        bodyEnemy.push_back(polickoEnemy);
    }

    colision = false;
    gameIteration = false;
    score = 0;

    //Pripojenie na server
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
    pthread_mutex_destroy(&mutDirectionEnemy);
    pthread_mutex_destroy(&mutEndEnemy);
    pthread_mutex_destroy(&mutFruitEnemy);
    pthread_mutex_destroy(&mutMove);
    pthread_cond_destroy(&conMove);
    close(sockfd);
}

void Snake::run()
{
    printf("Počkaj až sa pripojí druhý hráč.\n");
    char buffer[MSG_LEN];
    bzero(buffer, MSG_LEN);
    while (true) {
        int n = read(sockfd, buffer, MSG_LEN);
        if (n < 0)
        {
            perror("Error reading from socket\n");
            exit(6);
        }
        if (buffer[1] == 'S') {
            break;
        }
    }
    srand(time(NULL) ^ port);

    startNonstopKeyStream();

    pthread_mutex_init(&mutTEst, NULL);     //?

    pthread_t tUserIn;
    pthread_create(&tUserIn, NULL, userInput, this);
    pthread_t tServerIn;
    pthread_create(&tServerIn, NULL, serverInput, this);


    runSnake();


    pthread_detach(tUserIn);
    stopNonstopKeyStream();
    printf("Pred Koncom server\n");
    pthread_join(tServerIn, NULL);

    //Vyhodnotenie vysledkov
    printf("Pockaj kym dohra druhy hrac.\n");



        bool run = true;
        while(run) {        //poslanie score na server
            printf("Poslal som score\n");
            bzero(buffer, MSG_LEN);
            buffer[1] = 'H';
            buffer[3] = '0' + score;
            int n = write(sockfd, buffer, MSG_LEN);     //poslanie
            if (n < 0) {
                perror("Error writing to socket\n");
                return;
            }

            bzero(buffer, MSG_LEN);
            n = read(sockfd, buffer, MSG_LEN);      //cakanie na potvrdenie
            printf("Ziskal som potvrdenie ze server dostal moje score\n");
            if (n < 0) {
                perror("Error reading from socket\n");
                exit(6);
            }
            if (buffer[1] == 'Y') {
                run = false;
                printf("ukoncujem cyklus\n");
            }

        }



    int scoreEnemy = -1;        //cakanie na ziskanie score od servra druheho hraca
    run = true;
    while(run) {
        bzero(buffer, MSG_LEN);
        int n = read(sockfd, buffer, MSG_LEN);
        if (n < 0) {
            perror("Error reading from socket\n");
            exit(6);
        }
        switch (buffer[1]) {
            case 'V':
                    if (buffer[3] != '\0') {
                        scoreEnemy = buffer[3] - '0';
                        run = false;
                    } else {
                        printf("Zle");
                        perror("Bad result");
                    }
                break;
            default: break;

        }
    }
    std::string slovickoHrac1 = " ";
    std::string slovickoHrac2 = " ";
    if (score == 0 ||score >= 5) {
        slovickoHrac1 = "bodov";
    } else if (score == 1) {
        slovickoHrac1 = "bod";
    } else if (score > 1 && score < 5) {
        slovickoHrac1 = "body";
    }

    if (scoreEnemy == 0 || scoreEnemy >= 5) {
        slovickoHrac2 = "bodov";
    } else if (scoreEnemy == 1) {
        slovickoHrac2 = "bod";
    } else if (scoreEnemy > 1 && scoreEnemy < 5) {
        slovickoHrac2 = "body";
    }

    printf("\n---------------------\nScore\nAktualny hrac: %d %s\nSuper:         %d %s\n---------------------\n", score, slovickoHrac1.c_str(), scoreEnemy, slovickoHrac2.c_str());
    if (score > scoreEnemy) {
        printf("Vyhravas ty !!\n");
    } else if (score < scoreEnemy) {
        printf("Vyhrava super !!\n");
    } else {
        printf("Remiza !!\n");
    }
    printf("----------KONIEC HRY-----------\n\n");
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
    char buffer[MSG_LEN];
    int n;

    bzero(buffer, MSG_LEN);


    while (true)
    {
        if (read(STDIN_FILENO, &c, 1) > 0)      //fyzicke cakanie ked user stlaci klavesu
        {
            c = tolower(c);
            buffer[0] = c;

            switch (c)
            {
                case 'w':
                    if (client->prevDirection != 's') // Allow only if the previous direction was not 's'
                    {
                        pthread_mutex_lock(&client->mutDirection);
                        client->direction = c;
                        pthread_mutex_unlock(&client->mutDirection);
                    }
                    break;
                case 'a':
                    if (client->prevDirection != 'd') // Allow only if the previous direction was not 'd'
                    {
                        pthread_mutex_lock(&client->mutDirection);
                        client->direction = c;
                        pthread_mutex_unlock(&client->mutDirection);
                    }
                    break;
                case 's':
                    if (client->prevDirection != 'w') // Allow only if the previous direction was not 'w'
                    {
                        pthread_mutex_lock(&client->mutDirection);
                        client->direction = c;
                        pthread_mutex_unlock(&client->mutDirection);
                    } else {
                    }
                    break;
                case 'd':
                    if (client->prevDirection != 'a') // Allow only if the previous direction was not 'a'
                    {
                        pthread_mutex_lock(&client->mutDirection);
                        client->direction = c;
                        pthread_mutex_unlock(&client->mutDirection);
                    }
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
        /*
        pthread_mutex_lock(&client->mutColision);
        if (client->colision)
        {
            buffer[1] = 'E';
            break;
        }
        pthread_mutex_unlock(&client->mutColision);
*/
        pthread_mutex_lock(&client->mut);
        if (client->koniec)
        {
            pthread_mutex_unlock(&client->mut);
            break;
        }
        pthread_mutex_unlock(&client->mut);


        // Update the previous direction for the next iteration

    }
/*
    n = write(client->sockfd, buffer, MSG_LEN);
    if (n < 0)
    {
        perror("Error writing to socket\n");
        return NULL;
    }
*/
    return NULL;
}

void *Snake::serverInput(void *data) {
    Snake *client = static_cast<Snake *>(data);

    while (true) {
        char buffer[MSG_LEN];
        bzero(buffer, MSG_LEN);
        int n = read(client->sockfd, buffer, MSG_LEN);
        if (n < 0) {
            perror("Error reading from socket\n");
            exit(6);
        }

        client->processServerResponse(buffer);

        pthread_mutex_lock(&client->mut);
        if (client->koniec) {
            pthread_mutex_unlock(&client->mut);
            break;
        }
        pthread_mutex_unlock(&client->mut);
    }
    return NULL;
}

void Snake::runSnake() {
    //srand(time(NULL) ^ sockfd);
    initscr();      //nastavenie okna pre zobrazenie hry
    noecho();
    curs_set(FALSE);
    pthread_mutex_lock(&mutDirection);
    prevDirection = direction;
    pthread_mutex_unlock(&mutDirection);

    while (true) {
        moveSnake();        //logicky pohyb hada
        pthread_mutex_lock(&mutEndEnemy);
        if (!endEnemy) {
            moveSnakeEnemy();
        }
        pthread_mutex_unlock(&mutEndEnemy);


        //Doplnenie tela po zjdeni ovocia aktualnemu hracovi
        if (head.row == fruit.row && head.col == fruit.col) {       //kontrola zjedenia jedla
            fruit.row = 0;
            fruit.col = 0;
            score++;

            Policko newPolicko;
            Policko & last = body[body.size() - 1];
            Policko & preLast = body[body.size() - 2];

            newPolicko.row = last.row + (last.row - preLast.row);
            newPolicko.col = last.col + (last.col - preLast.col);

            body.push_back(newPolicko);
        }

        //Doplnenie tela po zjdeni ovocia enemy hracovi
        if (headEnemy.row == fruitEnemy.row && headEnemy.col == fruitEnemy.col) {       //kontrola zjedenia jedla
            fruitEnemy.row = 0;
            fruitEnemy.col = 0;

            Policko newPolickoEnemy;
            Policko & last = bodyEnemy[bodyEnemy.size() - 1];
            Policko & preLast = bodyEnemy[bodyEnemy.size() - 2];

            newPolickoEnemy.row = last.row + (last.row - preLast.row);
            newPolickoEnemy.col = last.col + (last.col - preLast.col);

            bodyEnemy.push_back(newPolickoEnemy);
        }

        displaySnake();     //zobrazenie na terminaly aktualny hrac
        displaySnakeEnemy();//zobrazenie na terminaly enemy

        displayScore();     //zobrazi aktualne skore

        refresh();

        pthread_mutex_lock(&mut);       //kontrola konca hry
        if (koniec) {
            pthread_mutex_unlock(&mut);
            break;
        }
        pthread_mutex_unlock(&mut);

        pthread_mutex_lock(&mutDirection);
        prevDirection = direction;
        pthread_mutex_unlock(&mutDirection);

        //usleep(1000 * 1000); // Adjust the delay based on your game speed

        pthread_mutex_lock(&mutMove);       //pohyb
        while (!gameIteration) {
            pthread_cond_wait(&conWait, &mutMove);
        }
        //printf("MoveM %d\n", ++vymaz2);
        gameIteration = false;
        pthread_cond_signal(&conMove);
        pthread_mutex_unlock(&mutMove);

    }

    endwin();       //nastavanie default nastaveni

    printf("\nkoniec zobrazovania snake\n");

}

void Snake::processServerResponse(char *buffer)
{
    int n;
    switch (buffer[2])
    {
        case 'w':
        case 'a':
        case 's':
        case 'd':
            pthread_mutex_lock(&mutDirectionEnemy);
            directionEnemy = buffer[2];
            pthread_mutex_unlock(&mutDirectionEnemy);
            break;
        case 'X':
            pthread_mutex_lock(&mutEndEnemy);
            endEnemy = true;
            pthread_mutex_unlock(&mutEndEnemy);

        default:
            break;
    }

    switch (buffer[1])
    {
        case 'E':
            //printf("Dostal som E\n");
            pthread_mutex_lock(&mut);
            koniec = true;
            pthread_mutex_unlock(&mut);

            //write server E
//            buffer[1] = 'E';
//            n = write(sockfd, buffer, MSG_LEN);     //poslanie
//            if (n < 0) {
//                perror("Error writing to socket\n");
//                return;
//            }
            //sam sebe move
            buffer[5] = 'M';
//            pthread_mutex_lock(&mutMove);
//            printf("SignalM bonus %d\n", ++vymaz);
//            gameIteration = true;
//            pthread_cond_signal(&conWait);
//            pthread_mutex_unlock(&mutMove);
            break;
            default:
            break;
    }
    switch (buffer[5])
    {
        case 'M':
            pthread_mutex_lock(&mutMove);
            while (gameIteration) {
                pthread_cond_wait(&conMove, &mutMove);
            }
            //printf("SignalM %d\n", ++vymaz);
            gameIteration = true;
            pthread_cond_signal(&conWait);
            pthread_mutex_unlock(&mutMove);
            break;
        default:
            break;
    }
    switch (buffer[6])
    {
        case 'F':
            pthread_mutex_lock(&mutFruitEnemy);
            fruitEnemy.row = (int)buffer[7] - '0';
            fruitEnemy.col = (int)buffer[8] - '0';
            pthread_mutex_unlock(&mutFruitEnemy);
            break;

        default: break;
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

        if (head.row == segment.row && head.col == segment.col) {
            pthread_mutex_lock(&mutColision);
            colision = true;
            pthread_mutex_unlock(&mutColision);

            pthread_mutex_lock(&mut);
            koniec = true;
            pthread_mutex_unlock(&mut);
        }
    }
}
void Snake::moveSnakeEnemy()
{
    int lastRow = headEnemy.row;
    int lastCol = headEnemy.col;
    switch (directionEnemy)
    {
        case 'w':
            headEnemy.row--;
            break;
        case 'a':
            headEnemy.col--;
            break;
        case 's':
            headEnemy.row++;
            break;
        case 'd':
            headEnemy.col++;
            break;
        default:
            perror("bad direction in moveSnake\n");
            break;
    }

//    if ( headEnemy.row <= 0 || headEnemy.col <= 0 || headEnemy.row >= SIRKA_PLOCHY - 1 || headEnemy.col >= VYSKA_PLOCHY - 1) {
//        pthread_mutex_lock(&mutColision);
//        colision = true;
//        pthread_mutex_unlock(&mutColision);
//
//        pthread_mutex_lock(&mut);
//        koniec = true;
//        pthread_mutex_unlock(&mut);
//    }

    //update body
    int tmpRow;
    int tmpCol;
    for (auto &policko : bodyEnemy)
    {
        tmpRow = lastRow;
        tmpCol = lastCol;
        lastRow = policko.row;
        lastCol = policko.col;
        policko.row = tmpRow;
        policko.col = tmpCol;

//        if (head.row == segment.row && head.col == segment.col) {
//            pthread_mutex_lock(&mutColision);
//            colision = true;
//            pthread_mutex_unlock(&mutColision);
//
//            pthread_mutex_lock(&mut);
//            koniec = true;
//            pthread_mutex_unlock(&mut);
//        }
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
    for (const auto &policko : body)
    {
        board[policko.row][policko.col] = 'O';
//        mvprintw(segment.row, segment.col * 2, "o ");
    }

    generateFruit();

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

void Snake::displaySnakeEnemy()
{
    // Clear the previous state of the board
    for (int i = 0 ; i < SIRKA_PLOCHY; i++)
    {
        for (int j = VYSKA_PLOCHY + 1; j < VYSKA_PLOCHY * 2 + 1; j++)
        {
            boardEnemy[i][j%VYSKA_PLOCHY] = ' ';
        }
    }

    // Display the snake head
    boardEnemy[headEnemy.row][headEnemy.col + 1] = 'X';
//    mvprintw(head.row, head.col * 2, "O ");

    // Display the snake body
    for (const auto &policko : bodyEnemy)
    {
        boardEnemy[policko.row][policko.col +1] = 'O';
//        mvprintw(segment.row, segment.col * 2, "o ");
    }

    generateFruitEnemy();

    // Display the board
    for (int i = 0; i < SIRKA_PLOCHY; i++)
    {
        for (int j = VYSKA_PLOCHY + 1; j < VYSKA_PLOCHY * 2 + 1; j++)
        {
            if (i == 0 || i == SIRKA_PLOCHY - 1 || j == VYSKA_PLOCHY + 1 || j == VYSKA_PLOCHY * 2)
            {
                mvprintw(i, j * 2, "# ");
            }
            else
            {
                mvprintw(i, j * 2, "%c ", boardEnemy[i][j%VYSKA_PLOCHY]);
            }
        }
    }

    refresh();
}
void Snake::displayScore() {
    mvprintw(SIRKA_PLOCHY + 1, 0, "Aktualne skore: %d", score);
}

void Snake::generateFruit() {
    // Ak sa nenachadza ziadne ovocie na boarde tak generujeme
    if (fruit.row == 0 || fruit.col == 0) {
        do {
            // random pozicia
            fruit.row = rand() % (SIRKA_PLOCHY - 2) + 1;
            fruit.col = rand() % (VYSKA_PLOCHY - 2) + 1;
        } while (board[fruit.row][fruit.col] != ' '); // Zabezpecujeme aby ovocie nebolo na policku kde je had
    }
    char msg[MSG_LEN];
    bzero(msg, MSG_LEN);
    msg[6] = 'F';
    msg[7] = fruit.row + '0';
    msg[8] = fruit.col + '0';
    int n = write(sockfd, msg, MSG_LEN);     //poslanie
    if (n < 0) {
        perror("Error writing to socket\n");
        return;
    }
    board[fruit.row][fruit.col] = 'F';  //nakreslit ovocie na boardu
}

void Snake::generateFruitEnemy() {
    // Ak sa nenachadza ziadne ovocie na boarde tak generujeme
    pthread_mutex_lock(&mutFruitEnemy);
    boardEnemy[fruitEnemy.row][fruitEnemy.col + 1] = 'F';
    pthread_mutex_unlock(&mutFruitEnemy);

}
