// main.cpp

#include "Snake.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        return 1;
    }

    Snake client(argv[1], atoi(argv[2]));
    client.run();
    //dsadaddruha
    //skusaksoidnoaiwndoanwdoiawondoanwdoaiwndo

    return 0;
}