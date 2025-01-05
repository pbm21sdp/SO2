#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

// definim caracterele din tabela extinsa ASCII de care vom avea nevoie pentru a desena patratul
const char *colt_stanga_sus = "┌";
const char *colt_dreapta_sus = "┐";
const char *colt_stanga_jos = "└";
const char *colt_dreapta_jos = "┘";
const char *orizontala = "─";
const char *verticala = "│";
const char *separator_sus = "┬";
const char *separator_jos = "┴";
const char *separator_stanga = "├";
const char *separator_central = "┼";
const char *separator_dreapta = "┤";

// tabla initial goala de 3x3 sub forma de matrice pentru a putea plasa X sau 0
char tabla[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};

void print_orizontala() // functie care va printa o linie orizontala corespunzatoare unei singure casute din patrat
{
    printf("%s%s%s%s%s", orizontala, orizontala, orizontala, orizontala, orizontala);
}

void print_linie_sus() // functie care printeaza latura de sus a patratului cu colturi si delimitari intre primele 3 casute
{
    printf("     A     B     C\n"); // adaugam literele pentru coloane
    printf("  %s", colt_stanga_sus);
    print_orizontala();
    printf("%s", separator_sus);
    print_orizontala();
    printf("%s", separator_sus);
    print_orizontala();
    printf("%s\n", colt_dreapta_sus);
}

void print_linie_mijloc() // functie care printeaza latura de mijloc a patratului cu colturi si delimitari intre cele 3 casute
{
    printf("  %s", separator_stanga);
    print_orizontala();
    printf("%s", separator_central);
    print_orizontala();
    printf("%s", separator_central);
    print_orizontala();
    printf("%s\n", separator_dreapta);
}

void print_linie_jos() // functie care printeaza latura de jos a patratului cu colturi si delimitari intre ultimele 3 casute
{
    printf("  %s", colt_stanga_jos);
    print_orizontala();
    printf("%s", separator_jos);
    print_orizontala();
    printf("%s", separator_jos);
    print_orizontala();
    printf("%s\n", colt_dreapta_jos);
}

void print_celule(int rand) // functie care printeaza laturile verticale ale patratului la distanta de o "casuta" si primeste ca parametru un intreg reprezentand numarul randului din tabla
{
    printf("%d %s  %c  %s  %c  %s  %c  %s\n", rand + 1, verticala, tabla[rand][0], verticala, tabla[rand][1], verticala, tabla[rand][2], verticala);
}

void print_tabla()
{
    print_linie_sus();
    print_celule(0);
    print_linie_mijloc();
    print_celule(1);
    print_linie_mijloc();
    print_celule(2);
    print_linie_jos();
}

void update_tabla(char *buffer)
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (buffer[i * 3 + j] == '0')
                tabla[i][j] = ' ';
            else if (buffer[i * 3 + j] == '1')
                tabla[i][j] = 'X';
            else if (buffer[i * 3 + j] == '2')
                tabla[i][j] = 'O';
        }
    }
}

void play_game(int sock)
{
    char buffer[1024] = {0};
    char move[10];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        int valread = recv(sock, buffer, 9, 0);
        if (valread <= 0)
        {
            printf("Connection closed by server.\n");
            break;
        }

        update_tabla(buffer);
        print_tabla();

        memset(buffer, 0, sizeof(buffer));
        valread = recv(sock, buffer, sizeof(buffer), 0);
        if (valread <= 0)
        {
            printf("Connection closed by server.\n");
            break;
        }
        printf("%s", buffer);

        // printf("\nDEBUG\n");
        // printf("%s", buffer);
        // printf("%ld - len\n", strlen(buffer));
        // printf("\nDEBUG\n");

        // Check if the game has ended
        if (strstr(buffer, "win") || strstr(buffer, "draw") || strstr(buffer, "lose"))
        {
            break;
        }

        // Only prompt for input if it is this player's turn
        if (strstr(buffer, "Your move"))
        {
            printf("Enter your move (A1, B2, etc.): ");
            fgets(move, sizeof(move), stdin);
            send(sock, move, strlen(move), 0);
        }
    }
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed\n");
        return -1;
    }

    printf("Connected to the server. Waiting for opponent...\n");
    play_game(sock);

    close(sock);
    return 0;
}
