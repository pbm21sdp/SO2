#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8080       // portul pe care va asculta conexiuni
#define MAX_CLIENTI 100 // numarul maxim de clienti
#define SIZE 3          // dimensiune tabla (3x3)

// Structura pentru crearea unui joc
typedef struct
{
    int jucator1;
    int jucator2;
    char name1[50]; // numele jucatorului 1
    char name2[50]; // numele jucatorului 2
    char tabla[SIZE][SIZE];
    int jucator_curent; // jucatorul care realizeaza mutarea (1 sau 2)
    int activitate;     // starea jocului (0 = inactiv, 1 = activ)
} JOC;

// vom stoca in coada atat socket-ul, cat si numele jucatorului
typedef struct
{
    int socket;
    char name[50];
} Client;

JOC jocuri[MAX_CLIENTI / 2];       // vector pentru stocarea jocurilor
Client coada_clienti[MAX_CLIENTI]; // coada de asteptare a clientilor
int inceput = 0, sfarsit = 0;      // index de inceput si sfarsit in coada

pthread_mutex_t coada_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t joc_mutex = PTHREAD_MUTEX_INITIALIZER;

// functie pentru initializarea tablei
void init_tabla(char tabla[SIZE][SIZE])
{
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            tabla[i][j] = ' ';
}

// functie pentru resetarea (re-initializarea) unui joc
void resetare_joc(JOC *joc)
{
    joc->jucator1 = 0;
    joc->jucator2 = 0;
    joc->name1[0] = 0;
    joc->name2[0] = 0;
    init_tabla(joc->tabla);
    joc->jucator_curent = 0;
    joc->activitate = 0;
}

// functie care transforma tabla de joc in string pentru a putea fi trimisa jucatorilor
void afisare_tabla(char tabla[SIZE][SIZE], char *buffer)
{
    int index = 0;
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            if (tabla[i][j] == ' ')
                buffer[index] = '0';
            else if (tabla[i][j] == 'X')
                buffer[index] = '1';
            else if (tabla[i][j] == 'O')
                buffer[index] = '2';
            index++;
        }
    }
    buffer[index] = 0;
}

// verifica daca un jucator a castigat
int verifica_castigator(char tabla[SIZE][SIZE])
{
    for (int i = 0; i < SIZE; i++)
    {
        // verificare pe linii
        if (tabla[i][0] != ' ' && tabla[i][0] == tabla[i][1] && tabla[i][1] == tabla[i][2])
        {
            return 1;
        }
        // verificare pe coloane
        if (tabla[0][i] != ' ' && tabla[0][i] == tabla[1][i] && tabla[1][i] == tabla[2][i])
        {
            return 1;
        }
    }
    // verificare pe diagonale
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

// functie in caz de remiza
int remiza(char tabla[SIZE][SIZE])
{
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (tabla[i][j] == ' ')
                return 0;
    return 1;
}

// verifica daca mutarea trimisa este valida
int mutare_valida(JOC *joc, char *coordonate, int *rand, int *coloana)
{
    if (strlen(coordonate) != 2)
        return 0;

    char litera = toupper(coordonate[0]);
    char cifra = coordonate[1];

    if (litera < 'A' || litera > 'C' || cifra < '1' || cifra > '3')
        return 0;

    *rand = cifra - '1';     // randul 0,1,2
    *coloana = litera - 'A'; // coloana 0,1,2

    if (joc->tabla[*rand][*coloana] != ' ')
        return 0;

    return 1;
}

// thread pentru gestionarea jocului
void *joc_thread(void *arg)
{
    int joc_index = *(int *)arg;
    free(arg);

    JOC *joc = &jocuri[joc_index];
    char buffer[1024];
    char joc_str[10];
    char mutare[3];
    int rand, coloana;

    int mutare_invalida = 0;

    // initializam tabla (de fiecare data cand incepe un nou joc)
    init_tabla(joc->tabla);

    // afisam la inceput cine joaca: numele jucator1 (X) si jucator2 (O)
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "Incepe jocul intre %s (X) si %s (O)!\n", joc->name1, joc->name2);

    // trimitem ambele mesaje la ambii jucatori
    send(joc->jucator1, buffer, strlen(buffer), 0);
    send(joc->jucator2, buffer, strlen(buffer), 0);

    while (1)
    {
        // determinam jucatorul curent si pe celalalt
        int jucator_curent_fd = (joc->jucator_curent == 1) ? joc->jucator1 : joc->jucator2;
        int jucator_oponent_fd = (joc->jucator_curent == 1) ? joc->jucator2 : joc->jucator1;

        if (mutare_invalida)
        {
            // daca ultima mutare a fost invalida, trimitem un mesaj jucatorului curent
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "\nMutare invalida.\nIntroduceti mutarea (A1, B2, etc.): ");
            send(jucator_curent_fd, buffer, strlen(buffer), 0);
        }
        else
        {
            // trimitem tabla jucatorului curent
            memset(joc_str, 0, sizeof(joc_str));
            afisare_tabla(joc->tabla, joc_str);
            send(jucator_curent_fd, joc_str, strlen(joc_str), 0);

            // mesaj: e randul lui
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "\nEste randul tau. Introduceti mutarea (A1, B2, etc.): ");
            send(jucator_curent_fd, buffer, strlen(buffer), 0);

            // trimitem tabla si celuilalt jucator
            send(jucator_oponent_fd, joc_str, strlen(joc_str), 0);
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "\nSe asteapta mutarea oponentului...\n");
            send(jucator_oponent_fd, buffer, strlen(buffer), 0);
        }

        // asteptam mutarea jucatorului curent
        memset(mutare, 0, sizeof(mutare));
        if (recv(jucator_curent_fd, mutare, sizeof(mutare), 0) <= 0)
        {
            // daca jucatorul curent s-a deconectat, anuntam oponentul si oprim jocul
            memset(buffer, 0, sizeof(buffer));
            if (joc->jucator_curent == 1)
            {
                sprintf(buffer, "\nOponentul (%s) s-a deconectat. %s a castigat prin forfeit!\n",
                        joc->name1, joc->name2);
            }
            else
            {
                sprintf(buffer, "\nOponentul (%s) s-a deconectat. %s a castigat prin forfeit!\n",
                        joc->name2, joc->name1);
            }
            send(jucator_oponent_fd, buffer, strlen(buffer), 0);
            break;
        }
        mutare[2] = 0; // terminator de sir

        // verificam daca mutarea este valida
        if (!mutare_valida(joc, mutare, &rand, &coloana))
        {
            mutare_invalida = 1;
            continue;
        }

        mutare_invalida = 0;

        // protejam modificarea tablei cu un mutex
        pthread_mutex_lock(&joc_mutex);
        if (rand >= 0 && rand < SIZE && coloana >= 0 && coloana < SIZE && joc->tabla[rand][coloana] == ' ')
        {
            // efectuam mutarea
            joc->tabla[rand][coloana] = (joc->jucator_curent == 1) ? 'X' : 'O';

            // verificam daca a castigat
            if (verifica_castigator(joc->tabla))
            {
                // afisam tabla finala la ambii jucatori
                afisare_tabla(joc->tabla, joc_str);

                // jucatorul curent vede tabela + mesaj de castig
                send(jucator_curent_fd, joc_str, strlen(joc_str), 0);
                memset(buffer, 0, sizeof(buffer));

                if (joc->jucator_curent == 1)
                {
                    sprintf(buffer, "Felicitari, %s! Ai castigat!\n", joc->name1);
                    send(jucator_curent_fd, buffer, strlen(buffer), 0);

                    // oponentul vede tabla + mesaj de infrangere
                    send(jucator_oponent_fd, joc_str, strlen(joc_str), 0);
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "Ai pierdut, %s! :(\n", joc->name2);
                    send(jucator_oponent_fd, buffer, strlen(buffer), 0);
                }
                else
                {
                    sprintf(buffer, "Felicitari, %s! Ai castigat!\n", joc->name2);
                    send(jucator_curent_fd, buffer, strlen(buffer), 0);

                    // oponentul vede tabla + mesaj de infrangere
                    send(jucator_oponent_fd, joc_str, strlen(joc_str), 0);
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "Ai pierdut, %s! :(\n", joc->name1);
                    send(jucator_oponent_fd, buffer, strlen(buffer), 0);
                }
                pthread_mutex_unlock(&joc_mutex);
                break;
            }
            else if (remiza(joc->tabla))
            {
                // verificam daca e remiza
                afisare_tabla(joc->tabla, joc_str);
                send(jucator_curent_fd, joc_str, strlen(joc_str), 0);
                send(jucator_oponent_fd, joc_str, strlen(joc_str), 0);

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "Remiza, %s si %s!\n", joc->name1, joc->name2);
                send(jucator_curent_fd, buffer, strlen(buffer), 0);
                send(jucator_oponent_fd, buffer, strlen(buffer), 0);
                pthread_mutex_unlock(&joc_mutex);
                break;
            }
            // aaca nu s-a castigat si nici remiza, schimbam jucatorul
            joc->jucator_curent = (joc->jucator_curent == 1) ? 2 : 1;
        }
        pthread_mutex_unlock(&joc_mutex);
    }

    // inchidem conexiunile si resetam jocul
    close(joc->jucator1);
    close(joc->jucator2);
    resetare_joc(joc);
    return NULL;
}

// thread pentru matchmaking: asociaza jucatorii in perechi
void *matchmaking_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&coada_mutex);
        // verificam daca avem cel putin 2 clienti in coada
        if (sfarsit - inceput >= 2)
        {
            // scoatem doi clienti din coada
            Client jucator1 = coada_clienti[inceput++];
            Client jucator2 = coada_clienti[inceput++];

            // gasim un joc inactiv
            int joc_index = -1;
            for (int i = 0; i < MAX_CLIENTI / 2; i++)
            {
                if (!jocuri[i].activitate)
                {
                    joc_index = i;
                    break;
                }
            }

            // daca s-a gasit un joc inactiv
            if (joc_index != -1)
            {
                // initializeaza jocul
                jocuri[joc_index].jucator1 = jucator1.socket;
                jocuri[joc_index].jucator2 = jucator2.socket;

                // salvam numele jucatorilor
                strcpy(jocuri[joc_index].name1, jucator1.name);
                strcpy(jocuri[joc_index].name2, jucator2.name);

                jocuri[joc_index].jucator_curent = 1; // primul jucator incepe
                jocuri[joc_index].activitate = 1;     // marcheaza jocul ca activ

                // cream un thread pentru jocul curent
                int *argIndex = malloc(sizeof(int));
                *argIndex = joc_index;
                pthread_t joc_tid;
                pthread_create(&joc_tid, NULL, joc_thread, argIndex);
            }
        }
        pthread_mutex_unlock(&coada_mutex);

        // asteapta 100ms inainte de a relua bucla
        usleep(100000);
    }
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    // crearea socketului pentru server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Eroare la socket().");
        exit(EXIT_FAILURE);
    }

    // configurarea socketului pentru a permite reutilizarea adresei
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Eroare la setsockopt().");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // configurarea adresei serverului
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // asociaza socketul cu adresa si portul specificat
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Eroare la bind().");
        exit(EXIT_FAILURE);
    }

    // punem serverul in modul de ascultare
    if (listen(server_fd, 3) < 0)
    {
        perror("Eroare la listen().");
        exit(EXIT_FAILURE);
    }

    // creeaza un thread separat pentru matchmaking
    pthread_t matchmaking_tid;
    pthread_create(&matchmaking_tid, NULL, matchmaking_thread, NULL);

    printf("Se asteapta clientii pe portul %d...\n", PORT);

    while (1)
    {
        // acceptam o conexiune noua
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Nu s-a realizat conexiunea.");
            exit(EXIT_FAILURE);
        }

        // primim numele jucatorului imediat dupa conectare
        char name[50];
        memset(name, 0, sizeof(name));
        if (recv(new_socket, name, sizeof(name), 0) <= 0)
        {
            // daca nu am primit nimic, inchidem
            close(new_socket);
            continue;
        }

        printf("S-a conectat un jucator: %s\n", name);

        // adaugam jucatorul in coada
        pthread_mutex_lock(&coada_mutex);
        coada_clienti[sfarsit].socket = new_socket;
        strcpy(coada_clienti[sfarsit].name, name);
        sfarsit++;
        pthread_mutex_unlock(&coada_mutex);
    }

    return 0;
}
