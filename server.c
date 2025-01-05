#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BOARD_SIZE 3

typedef struct {
    int player1;
    int player2;
    char board[BOARD_SIZE][BOARD_SIZE];
    int current_turn;
    int active;
} Game;

Game games[MAX_CLIENTS / 2];
int client_queue[MAX_CLIENTS];
int queue_start = 0, queue_end = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            board[i][j] = ' ';
}

void reset_game(Game *game) {
    game->player1 = 0;
    game->player2 = 0;
    init_board(game->board);
    game->current_turn = 0;
    game->active = 0;
}

void print_board(char board[BOARD_SIZE][BOARD_SIZE], char *buffer) {
    int index = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == ' ') buffer[index] = '0';
            else if (board[i][j] == 'X') buffer[index] = '1';
            else if (board[i][j] == 'O') buffer[index] = '2';
            index++;
        }
    }
    buffer[index] = 0;
}

int check_winner(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return 1;
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return 1;
    }
    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return 1;
    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return 1;
    return 0;
}

int is_draw(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] == ' ') return 0;
    return 1;
}

int valid_move(Game *game ,char *coordinates, int *row, int *col){
    if (strlen(coordinates) != 2)
        return 0;

    char letter = toupper(coordinates[0]);
    char digit = coordinates[1];

    if (letter < 'A' || letter > 'C' || digit < '1' || digit > '3')
    {
        return 0;
    }

    *row = digit - '1';
    *col = letter - 'A';

    if (game->board[*row][*col] != ' ')
        return 0;

    return 1;
}



void *game_thread(void *arg) {
    int game_index = *(int *)arg;
    free(arg);

    Game *game = &games[game_index];
    char buffer[1024];
    char game_str[10];
    char move[3];
    int row, col;

    int bad_move = 0;

    init_board(game->board);

    while (1) {
        int player_fd = (game->current_turn == 1) ? game->player1 : game->player2;
        int opponent_fd = (game->current_turn == 1) ? game->player2 : game->player1;
        
        
        if (bad_move){
            memset(buffer, 0, sizeof(buffer));

            sprintf(buffer, "\n Illegal move. Try Again\n");
            strcat(buffer, "\nYour move (A1, B2, etc.): ");

            send(player_fd, buffer, strlen(buffer), 0);

        }
        else{
            memset(buffer, 0, sizeof(buffer));

            // Tabla player 1
            memset(game_str, 0, sizeof(game_str));
            print_board(game->board, game_str);
            send(player_fd, game_str, strlen(game_str), 0);

            // Cerere miscare player 1
            sprintf(buffer, "\nYour move (A1, B2, etc.): ");
            send(player_fd, buffer, strlen(buffer), 0);


            // Tabla player 2
            send(opponent_fd, game_str, strlen(game_str), 0);

            // Cerere miscare player 2
            sprintf(buffer, "\nOpponent's move, wait...\n");
            send(opponent_fd, buffer, strlen(buffer), 0);
        }

        memset(move, 0, sizeof(move));
        if (recv(player_fd, move, sizeof(move), 0) <= 0) {
            strcat(buffer, "\n Your opponent disconnected. You win.\n");
            send(opponent_fd, buffer, strlen(buffer), 0);
            break;
        }
        move[2] = 0;

        // sscanf(move, "%d %d", &row, &col);
        if (!valid_move(game, move, &row, &col)) {
            bad_move = 1;
            continue;
        }

        bad_move = 0;

        pthread_mutex_lock(&game_mutex);
        if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && game->board[row][col] == ' ') {
            game->board[row][col] = (game->current_turn == 1) ? 'X' : 'O';

            if (check_winner(game->board)) {
                print_board(game->board, game_str);
                send(player_fd, game_str, strlen(game_str), 0);
                sprintf(buffer, "You win!\n");
                send(player_fd, buffer, strlen(buffer), 0);

                print_board(game->board, game_str);
                send(opponent_fd, game_str, strlen(game_str), 0);
                sprintf(buffer, "You lose!\n");
                send(opponent_fd, buffer, strlen(buffer), 0);
                pthread_mutex_unlock(&game_mutex);
                break;
            } else if (is_draw(game->board)) {
                print_board(game->board, game_str);
                send(player_fd, game_str, strlen(game_str), 0);
                send(opponent_fd, game_str, strlen(game_str), 0);

                sprintf(buffer, "It's a draw!\n");
                send(player_fd, buffer, strlen(buffer), 0);
                send(opponent_fd, buffer, strlen(buffer), 0);
                pthread_mutex_unlock(&game_mutex);
                break;
            }

            game->current_turn = (game->current_turn == 1) ? 2 : 1;
        }
        pthread_mutex_unlock(&game_mutex);
    }

   

    close(game->player1);
    close(game->player2);
    reset_game(game);
    return NULL;
}

void *matchmaking_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        if (queue_end - queue_start >= 2) {
            int player1 = client_queue[queue_start++];
            int player2 = client_queue[queue_start++];

            int game_index = -1;
            for (int i = 0; i < MAX_CLIENTS / 2; i++) {
                if (!games[i].active) {
                    game_index = i;
                    break;
                }
            }

            if (game_index != -1) {
                games[game_index].player1 = player1;
                games[game_index].player2 = player2;
                games[game_index].current_turn = 1;
                games[game_index].active = 1;

                int *arg = malloc(sizeof(int));
                *arg = game_index;
                pthread_t game_tid;
                pthread_create(&game_tid, NULL, game_thread, arg);
            }
        }
        pthread_mutex_unlock(&queue_mutex);
        usleep(100000);
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1; // SO_REUSEADDR

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    pthread_t matchmaking_tid;
    pthread_create(&matchmaking_tid, NULL, matchmaking_thread, NULL);

    printf("Server is running on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&queue_mutex);
        client_queue[queue_end++] = new_socket;
        pthread_mutex_unlock(&queue_mutex);
    }

    return 0;
}
