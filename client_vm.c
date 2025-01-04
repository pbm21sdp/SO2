#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <ctype.h>

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

// functie care realizeaza conversia coordonatelor in indicii matricei

int coordonate_la_indici(char *coordonate, int *rand, int *coloana)
{
    if (strlen(coordonate) != 2)
    {
        return 0; // coordonate invalide
    }

    char litera = toupper(coordonate[0]); // transformam litera in majuscula
    char cifra = coordonate[1];

    if ((litera < 'A') || (litera > 'C') || (cifra < '1') || (cifra > '3'))
    {
        return 0; // coordonate invalide
    }

    *rand = cifra - '1';     // convertim '1', '2', '3' in 0, 1, 2
    *coloana = litera - 'A'; // convertim 'A', 'B', 'C' in 0, 1, 2

    return 1; // coordonate invalide
}

int main()
{
    int sock;
    struct sockaddr_in server;
    char buffer[1024];
    char coordonate[3];
    char nume[50];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "A aparut o eroare la crearea socket-ului.\n");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0)
    {
        printf("Adresa IP invalida.\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        fprintf(stderr, "A aparut o eroare la conexiune.\n");
        exit(2);
    }

    printf("Conectat la server.\n");
    printf("Introduceti numele de utilizator: ");
    scanf("%s", nume);
    send(sock, nume, strlen(nume), 0);

    close(sock);
    return 0;
}
