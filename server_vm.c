#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 8080

char tabla[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
int client1, client2;
int server_fd;

char game_state[10] = {0};

// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
// int connections_ready = 0;

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

void make_game_state(){

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            game_state[i + j] = tabla[i][j];
        }
    }
    game_state[9] = 0;
}

int mutare_valida(char *coordonate, int *rand, int *coloana)
{
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

    if (tabla[*rand][*coloana] != ' ')
        return 0;

    return 1;
}

void* asteptare_conexiuni(void* arg) {

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char jucator1[50], jucator2[50];

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("eroare la socket.");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("eroare la bind.");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 2) < 0) {
        perror("eroare la listen");
        exit(EXIT_FAILURE);
    }

    printf("Asteptam jucatori...\n");

    if ((client1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Jucatorul 1 nu a fost acceptat.");
        exit(EXIT_FAILURE);
    }
    recv(client1, jucator1, sizeof(jucator1), 0);
    printf("Jucator 1 conectat: %s\n", jucator1);

    if ((client2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Jucatorul 2 nu a fost acceptat.");
        exit(EXIT_FAILURE);
    }
    recv(client2, jucator2, sizeof(jucator2), 0);
    printf("Jucator 2 conectat: %s\n", jucator2);

    return NULL;
}

void* logica_programului(void* arg) {
    char buffer[128];
    int turn = 0; // 0 for player 1, 1 for player 2
    int linie, coloana;

    reseteaza_tabla();
    printf("Game start\n");

    while (1) {
        int current_client = (turn == 0) ? client1 : client2;
        memset(buffer, 0, sizeof(buffer));

        // Notify current client it's their turn
        send(current_client, "Turnul tau! Introdu miscarea (format: A1, B2, etc.): ", 128, 0);

        // Wait for move
        if (recv(current_client, buffer, sizeof(buffer), 0) <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        printf("Player %d move: %s\n", turn + 1, buffer);

        // Validate move
        if (!mutare_valida(buffer, &linie, &coloana)) {
            send(current_client, "Miscare invalida. Incearca din nou.\n", 128, 0);
            continue;
        }

        // Update board
        tabla[linie][coloana] = (turn == 0) ? 'X' : 'O';

        // Check for winner
        if (verificare_castig()) {
            send(client1, "Joc terminat! Ai castigat!\n", 128, 0);
            send(client2, "Joc terminat! Ai pierdut!\n", 128, 0);
            break;
        }

        // Send updated board to both clients
        make_game_state();
        send(client1, game_state, sizeof(game_state), 0);
        send(client2, game_state, sizeof(game_state), 0);

        // Switch turns
        turn = 1 - turn;
    }

    close(client1);
    close(client2);
    return NULL;
}


int main() {

    pthread_t conexiuni_thread, logica_thread;
    int ret;

    if ((ret = pthread_create(&conexiuni_thread, NULL, &asteptare_conexiuni, NULL)) != 0) {
        fprintf(stderr, "%s", strerror(ret));
        exit(EXIT_FAILURE);
    }

    pthread_join(conexiuni_thread, NULL);

    if ((ret = pthread_create(&logica_thread, NULL, &logica_programului, NULL)) != 0) {
        fprintf(stderr, "%s", strerror(ret));
        exit(EXIT_FAILURE);
    }

    pthread_join(logica_thread, NULL);

    close(server_fd);

    return 0;
}
