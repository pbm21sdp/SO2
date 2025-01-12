#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8080 // portul pe care va asculta conexiuni
#define MAX_CLIENTI 100 // numarul maxim de clienti
#define SIZE 3 // dimensiune tabla (3x3)

//structura pentru crearea unui joc
typedef struct {
    int jucator1;
    int jucator2;
    char tabla[SIZE][SIZE];
    int jucator_curent; // jucatorul care realizeaza mutarea
    int activitate; // starea jocului
} JOC;

//vector pentru stocarea jocurilor
JOC jocuri[MAX_CLIENTI / 2];
int coada_clienti[MAX_CLIENTI];
int inceput = 0, sfarsit = 0;

pthread_mutex_t coada_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t joc_mutex = PTHREAD_MUTEX_INITIALIZER;

//functie pentru initializarea tablei
void init_tabla(char tabla[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            tabla[i][j] = ' ';
}

//functie pentu resetarea jocului
void resetare_joc(JOC *joc) {
    joc->jucator1 = 0;
    joc->jucator2 = 0;
    init_tabla(joc->tabla);
    joc->jucator_curent = 0;
    joc->activitate = 0;
}

// functie care transforma tabla de joc in string pentru a putea fi trimisa jucatorilor
void afisare_tabla(char tabla[SIZE][SIZE], char *buffer) {
    int index = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (tabla[i][j] == ' ') buffer[index] = '0';
            else if (tabla[i][j] == 'X') buffer[index] = '1';
            else if (tabla[i][j] == 'O') buffer[index] = '2';
            index++;
        }
    }
    buffer[index] = 0;
}

//verifica daca un jucator a castigat sau e remiza
int varifica_castigator(char tabla[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        //verificare pe linii
        if (tabla[i][0] != ' ' && tabla[i][0] == tabla[i][1] && tabla[i][1] == tabla[i][2])
        {
            return 1;
        }
        if (tabla[0][i] != ' ' && tabla[0][i] == tabla[1][i] && tabla[1][i] == tabla[2][i])
        {
            return 1;
        }
    }
    //verficare pe diagonale
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

//functie in caz de remiza
int remiza(char tabla[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (tabla[i][j] == ' ')
            {
                return 0;
            }
    return 1;
}

//verifica daca mutarea trimisa este valida
int mutare_valida(JOC *joc ,char *coordonate, int *rand, int *coloana){
    if (strlen(coordonate) != 2)
        return 0;

    char litera = toupper(coordonate[0]);
    char cifra = coordonate[1];

    if (litera < 'A' || litera > 'C' || cifra < '1' || cifra > '3')
    {
        return 0;
    }

    *rand = cifra - '1';
    *coloana = litera - 'A';

    if (joc->tabla[*rand][*coloana] != ' ')
        return 0;

    return 1;
}

//creare thread pentru gestionarea jocului
void *joc_thread(void *arg) {
    int joc_index = *(int *)arg; //obtine indicele jocului din argument
    free(arg);

    JOC *joc = &jocuri[joc_index];
    char buffer[1024]; // buffer pentru mesaje
    char joc_str[10]; // reprezentarea tablei in format text
    char mutare[3]; // coordonatele mutarii (ex: "A1")
    int rand, coloana; // coordonatele interpretate (linie si coloana)

    int mutare_invalida = 0;

    init_tabla(joc->tabla);

    while (1) {
        // selecteaza jucatorii curenti
        int jucator1_fd = (joc->jucator_curent == 1) ? joc->jucator1 : joc->jucator2;
        int jucator2_fd = (joc->jucator_curent == 1) ? joc->jucator2 : joc->jucator1;
        
        
        if (mutare_invalida){
            // daca ultima mutare a fost invalida, trimite un mesaj corespunzator
            memset(buffer, 0, sizeof(buffer));

            sprintf(buffer, "\n Mutare invalida.\n");
            strcat(buffer, "\n Introduceti mutarea(A1, B2, etc.): ");

            send(jucator1_fd, buffer, strlen(buffer), 0);

        }
        else{
             // trimite tabla si cere mutare de la jucatorul curent
            memset(buffer, 0, sizeof(buffer));

            // Tabla player 1
            memset(joc_str, 0, sizeof(joc_str));
            afisare_tabla(joc->tabla, joc_str); // converteste tabla in text
            send(jucator1_fd, joc_str, strlen(joc_str), 0); // trimite tabla jucatorului 1

            // cerere miscare jucatorului 1
            sprintf(buffer, "\n Introduceti mutarea(A1, B2, etc.): ");
            send(jucator1_fd, buffer, strlen(buffer), 0);


             // trimite tabla jucatorului 2 si ii comunica ca asteapta mutarea oponentului
            send(jucator2_fd, joc_str, strlen(joc_str), 0);

            // cerere miscare jucatorului 2
            sprintf(buffer, "\n Se asteapta mutarea oponentului...\n");
            send(jucator2_fd, buffer, strlen(buffer), 0);
        }

        // asteapta mutarea jucatorului curent
        memset(mutare, 0, sizeof(mutare));
        if (recv(jucator1_fd, mutare, sizeof(mutare), 0) <= 0) {
            // daca jucatorul s-a deconectat, anunta oponentul si opreste jocul
            strcat(buffer, "\n Oponentul s-a deconectat. Ati castigat!\n");
            send(jucator2_fd, buffer, strlen(buffer), 0);
            break;
        }
        mutare[2] = 0; // asigura terminatorul de sir

        // verifica daca mutarea este valida
        if (!mutare_valida(joc, mutare, &rand, &coloana)) {
            mutare_inlavida= 1; // seteaza flag-ul pentru mutare invalida
            continue;
        }

        mutare_invalida = 0; // reseteaza flag-ul pentru mutare invalida

        // blocheaza mutex-ul pentru a proteja modificarea tablei
        pthread_mutex_lock(&joc_mutex);
        if (rand >= 0 && rand < SIZE && coloana >= 0 && coloana < SIZE && joc->tabla[rand][coloana] == ' ') {
            // efectueaza mutarea
            joc->tabla[rand][coloana] = (joc->jucator_curent == 1) ? 'X' : 'O';
            // verifica daca mutarea a dus la castig
            if (verifica_castigator(joc->board)) {
                afisare_tabla(joc->tabla, joc_str);
                send(jucator1_fd, joc_str, strlen(joc_str), 0);
                sprintf(buffer, "WINNER WINNER CHICKEN DINNER\n");
                send(jucator1_fd, buffer, strlen(buffer), 0);

                afisare_tabla(joc->tabla, joc_str);
                send(jucator2_fd, joc_str, strlen(joc_str), 0);
                sprintf(buffer, "Ati pierdut!:(\n");
                send(jucator2_fd, buffer, strlen(buffer), 0);
                pthread_mutex_unlock(&game_mutex);
                break;
            } else if (remiza(joc->tabla)) {
                 // verifica daca jocul este remiza
                afisare_tabla(joc->tabla, joc_str);
                send(jucator1_fd, joc_str, strlen(joc_str), 0);
                send(jucator2_fd, joc_str, strlen(joc_str), 0);

                sprintf(buffer, "Remiza!\n");
                send(jucator1_fd, buffer, strlen(buffer), 0);
                send(jucator2_fd, buffer, strlen(buffer), 0);
                pthread_mutex_unlock(&joc_mutex);
                break;
            }
            // schimba jucatorul curent
            joc->jucator_curent = (joc->jucator_curent == 1) ? 2 : 1;
        }
        pthread_mutex_unlock(&joc_mutex);
    }
    // inchide conexiunile si reseteaza jocul
    close(joc->jucator1);
    close(joc->jucator2);
    resetare_joc(joc);
    return NULL; // reseteaza starea jocului
}

//creare thread pentru matchmaking
void *matchmaking_thread(void *arg) {
    while (1) {
        // blocheaza mutex-ul pentru a proteja accesul la coada de clienti
        pthread_mutex_lock(&coada_mutex);
         // verifica daca exista cel putin doi clienti in coada
        if (sfarsit - inceput >= 2) {
            // scoate doi clienti din coada
            int jucator1 = coada_clienti[inceput++];
            int jucator2 = coada_clienti[inceput++];

            // gaseste un joc inactiv in lista de jocuri
            int joc_index = -1;
            for (int i = 0; i < MAX_CLIENTI / 2; i++) {
                if (!jocuri[i].activitate) { // verifica daca jocul este inactiv
                    joc_index = i;
                    break;
                }
            }

            // daca s-a gasit un joc inactiv
            if (joc_index != -1) {
                // initializeaza jocul cu cei doi jucatori
                jocuri[joc_index].jucator1 = jucator1;
                jocuri[joc_index].jucator2 = jucator2;
                jocuri[joc_index].juator_curent = 1; // primul jucator incepe
                jocuri[joc_index].activitate = 1; // marcheaza jocul ca activ

                // creeaza un thread pentru jocul curent
                int *arg = malloc(sizeof(int));
                *arg = joc_index;
                pthread_t joc_tid;
                pthread_create(&joc_tid, NULL, joc_thread, arg);
            }
        }
        
        // deblocheaza mutex-ul dupa procesarea cozii
        pthread_mutex_unlock(&coada_mutex);

        // asteapta 100ms inainte de a relua bucla
        usleep(100000);
    }
}

int main() {
    int server_fd, new_socket; //descriptorul de fisier pentru server si pentru noile conexiuni ale clientilor
    struct sockaddr_in address; // structura pentru adresa serverului
    int addrlen = sizeof(address); // dimensiunea structurii `address`
    int opt = 1; //optiunea pentru reutilizarea adresei (SO_REUSEADDR)

    // crearea socketului pentru server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("A aparut o eroare la socket.");
        exit(EXIT_FAILURE);
    }

    // configurarea socketului pentru a permite reutilizarea adresei
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("A aparut o eroare la setsockopt.");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // configurarea adresei serverului
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // accepta conexiuni pe orice adresa locala
    address.sin_port = htons(PORT); // portul pe care serverul asculta (convertit in retea)

    // asocierea socketului cu adresa si portul specificat
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("A aparut o eroare la bind.");
        exit(EXIT_FAILURE);
    }

    // setează socketul in modul de ascultare, permitand pana la 3 conexiuni simultane in coada de asteptare
    if (listen(server_fd, 3) < 0) {
        perror("A aparut o eroare la listen");
        exit(EXIT_FAILURE);
    }

    // creeaza un thread separat pentru matchmaking (asocierea jucatorilor in perechi)
    pthread_t matchmaking_tid;
    pthread_create(&matchmaking_tid, NULL, matchmaking_thread, NULL);

    printf("Se asteapta clientii pe portul %d\n", PORT);

    while (1) {
        // accepta o conexiune noua
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Nu s-a realizat conexiunea.");
            exit(EXIT_FAILURE);
        }

         // adauga descriptorul noii conexiuni in coada de clienti
        pthread_mutex_lock(&coada_mutex);
        coada_clienti[sfarsit++] = new_socket;
        pthread_mutex_unlock(&coada_mutex);
    }

    return 0;
}
