#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <termios.h> //termios, TCSANOW, ECHO, ICANON
#include <ctype.h>
#include <pthread.h>
//client

struct termios menicVstupuTerminalu;

bool konec = false;
pthread_mutex_t mut;

void StartNonstopKeyStream() {      //umoznuje pisanie bez toho aby sa automaticky vypisoval na obrazovku + netreba stlačiť enter
    tcgetattr(STDIN_FILENO, &menicVstupuTerminalu);
    menicVstupuTerminalu.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &menicVstupuTerminalu);
}
void StopNonstopKeyStream() {       //vyplnutie tohto režimu do povodneho stavu
    tcgetattr(STDIN_FILENO, &menicVstupuTerminalu);
    menicVstupuTerminalu.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &menicVstupuTerminalu);
}

void * userInput(void * data) {
    int sockfd = *(int *)data;
    char c;
    int vymaztoto = 0;
    char buffer[256];
    int n;
    bzero(buffer, 256);
    while (1) {

        if (read(STDIN_FILENO, &c, 1) > 0) {
            printf("Key pressed: %c\n", c);

            c = tolower(c);     //zmena na male pismeno
            buffer[0] = c;
            vymaztoto++;
            if (vymaztoto == 5) {
                buffer[1] = 'E';        //koniec hry (narazenie jedneho z hracov)
                c = 'r';
            }
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0) {
                perror("Error writing to socket\n");
                return NULL;
            }
            if (c == 'r') {
                pthread_mutex_lock(&mut);
                konec = true;
                pthread_mutex_unlock(&mut);
            }

            bzero(buffer, 256);
        }
        pthread_mutex_lock(&mut);
        if (konec) {
            pthread_mutex_unlock(&mut);
            break;
        }
        pthread_mutex_unlock(&mut);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    char buffer[256];

    if (argc < 3)
    {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        return 1;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char*)server->h_addr,
            (char*)&serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        return 3;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting to socket");
        return 4;
    }

    StartNonstopKeyStream();
    pthread_mutex_init(&mut, NULL);
    pthread_t thr;
    pthread_create(&thr, NULL, userInput, &sockfd);



    //printf("Please enter a message: ");
    //fgets(buffer, 255, stdin);

    while(true) {
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        if (n < 0) {
            perror("Error reading from socket\n");
            return 6;
        }
        //printf("pred pp %c %c\n", buffer[1], buffer[2]);
        switch (buffer[2]) {
            case 'w':
            case 'a':
            case 's':
            case 'd':
                //printf("znak %c\n", buffer[2]);       //spomaluje moc
                break;
            default: break;
        }
        switch (buffer[1]) {
            case 'E':
                //printf("Prisiel znak E\n");
                pthread_mutex_lock(&mut);
                konec = true;
                pthread_mutex_unlock(&mut);
                break;
            default: break;
        }

        pthread_mutex_lock(&mut);
        if (konec) {
            pthread_mutex_unlock(&mut);
            break;
        }
        pthread_mutex_unlock(&mut);
    }

    pthread_detach(thr);
    pthread_mutex_destroy(&mut);
    StopNonstopKeyStream();



    printf("Finalny koniec hry\n");

    close(sockfd);

    return 0;
}