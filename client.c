#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <ctype.h>

#define PORT 8080

// am inlocuit variabilele cu valori numerice pentru caracterele ASCII extinse cu variabile de mai jos pentru ca masina virtuala avea
// probleme cu gestionarea codurilor Unicode

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

// functii pentru desenarea tablei

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

void print_tabla() // functie care printeaza tabla in intregime
{
    print_linie_sus();
    print_celule(0);
    print_linie_mijloc();
    print_celule(1);
    print_linie_mijloc();
    print_celule(2);
    print_linie_jos();
}

// functie care actualizeaza tabla locala in functie de stringul (9 caractere) trimis de server
void actualizeaza_tabla(char *buffer)
{
    int i;
    int j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (buffer[i * 3 + j] == '0')
            {
                tabla[i][j] = ' ';
            }
            else if (buffer[i * 3 + j] == '1')
            {
                tabla[i][j] = 'X';
            }
            else if (buffer[i * 3 + j] == '2')
            {
                tabla[i][j] = '0';
            }
        }
    }
}

// functie principala de joc (bucla in care primim tabla, mesaje etc.)
void joaca(int sock)
{
    char buffer[1024] = {0};
    char move[10];

    // primim un mesaj de la server despre inceputul jocului
    memset(buffer, 0, sizeof(buffer));
    int valread = recv(sock, buffer, sizeof(buffer), 0);

    if (valread <= 0)
    {
        printf("Conexiunea a fost intrerupta de server.\n");
        exit(1);
    }
    // printam mesajul primit de la server
    printf("%s", buffer);

    while (1)
    {
        // primim tabla (9 caractere) de la server
        memset(buffer, 0, sizeof(buffer));
        valread = recv(sock, buffer, 9, 0);

        if (valread <= 0)
        {
            printf("Conexiunea a fost intrerupta de server.\n");
            break;
        }

        // actualizam tabla si o afisam
        actualizeaza_tabla(buffer);
        print_tabla();

        // primim un mesaj de la server despre mutari sau starea jocului
        memset(buffer, 0, sizeof(buffer));
        valread = recv(sock, buffer, sizeof(buffer), 0);

        if (valread <= 0)
        {
            printf("Conexiunea a fost intrerupta de server.\n");
            break;
        }
        // printam mesajul primit de la server
        printf("%s", buffer);

        // verificam daca jocul s-a terminat (prin cuvinte cheie)
        if (strstr(buffer, "castigat") ||
            strstr(buffer, "pierdut") ||
            strstr(buffer, "Remiza") ||
            strstr(buffer, "deconectat") ||
            strstr(buffer, "forfeit"))
        {
            break;
        }

        // printam aceasta linie doar daca este randul acestui jucator
        if (strstr(buffer, "Introduceti mutarea") || strstr(buffer, "randul tau"))
        {
            // citim mutarea de la tastatura
            memset(move, 0, sizeof(move));
            fgets(move, sizeof(move), stdin);

            // scoatem \n de la final (daca exista)
            move[strcspn(move, "\n")] = 0;

            // trimitem la server
            send(sock, move, strlen(move), 0);
        }
    }
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    // cream socketul
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Eroare la crearea socketului.\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Adresa invalida.\n");
        exit(1);
    }

    // conectarea clientului la server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Conexiunea a esuat.\n");
        exit(1);
    }

    // trimitem numele jucatorului imediat dupa ce conectare
    char name[50];
    printf("Introdu username-ul: ");
    fgets(name, sizeof(name), stdin);

    // scoatem \n de la final (daca exista)
    name[strcspn(name, "\n")] = 0;

    // trimitem numele catre server
    send(sock, name, strlen(name), 0);

    printf("Te-ai conectat la server ca \"%s\". Asteptam un oponent...\n", name);

    // jocul efectiv
    joaca(sock);

    // inchidem conexiunea
    close(sock);
    return 0;
}
