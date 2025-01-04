#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8080

void reseteaza_tabla()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            tabla[i][j] = ' ';
        }
    }
}

int verificare_castig()
{
    for (int i = 0; i < 3; i++)
    {
        if (tabla[i][0] != ' ' && tabla[i][0] == tabla[i][1] && tabla[i][1] == tabla[i][2])
        {
            return 1;
        }
    }

    for (int j = 0; j < 3; j++)
    {
        if (tabla[0][j] != ' ' && tabla[0][j] == tabla[1][j] && tabla[1][j] == tabla[2][j])
        {
            return 1;
        }
    }

    if (tabla[0][0] != ' ' && tabla[0][0] == tabla[1][1] && tabla[1][1] == tabla[2][2])
    {
        return 1;
    }
    if (tabla[0][2] != ' ' && tabla[0][2] == tabla[1][1] && tabla[1][1] == tabla[2][0])
    {
        return 1;
    }

    return 0;
}


int main()
{
    int server_fd, client1, client2;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char jucator1[50], jucator2[50];

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("eroare la socket.");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("eroare la bind.");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0)
    {
        perror("eroare la listen");
        exit(EXIT_FAILURE);
    }

    printf("Asteptam jucatori...\n");

    if ((client1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("Jucatorul 1 nu a fost acceptat.");
        exit(EXIT_FAILURE);
    }
    recv(client1, jucator1, sizeof(jucator1), 0);
    printf("Jucator 1 conectat: %s\n", jucator1);

    if ((client2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("Jucatorul 2 nu a fost acceptat.");
        exit(EXIT_FAILURE);
    }
    recv(client2, jucator2, sizeof(jucator2), 0);
    printf("Jucator 2 conectat: %s\n", jucator2);

    close(client1);
    close(client2);
    close(server_fd);
    return 0;
}
