#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

// definim caracterele din tabela extinsa ASCII de care vom avea nevoie pentru a desena patratul
char colt_stanga_sus = 218;    // ┐
char colt_dreapta_sus = 191;   // ┒
char colt_stanga_jos = 192;    // └
char colt_dreapta_jos = 217;   // ┘
char orizontala = 196;         // ─
char verticala = 179;          // │
char separator_sus = 194;      // ┬
char separator_jos = 193;      // ┴
char separator_stanga = 195;   // ├
char separator_central = 197;  // ┼
char separator_dreapta = 180;  // ┤

// tabla initial goala de 3x3 sub forma de matrice pentru a putea plasa X sau 0 
char tabla[3][3] = { {' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '} };

void print_orizontala() // functie care va printa o linie orizontala corespunzatoare unei singure casute din patrat
{
    printf("%c%c%c%c%c", orizontala, orizontala, orizontala, orizontala, orizontala);
}

void print_linie_sus() // functie care printeaza latura de sus a patratului cu colturi si delimitari intre primele 3 casute 
{
    printf("    A     B     C\n"); // Adaugam literele pentru coloane
    printf("  %c", colt_stanga_sus);
    print_orizontala();
    printf("%c", separator_sus);
    print_orizontala();
    printf("%c", separator_sus);
    print_orizontala();
    printf("%c\n", colt_dreapta_sus);
}

void print_linie_mijloc() // functie care printeaza latura de mijloc a patratului cu colturi si delimitari intre cele 3 casute 
{
    printf("  %c", separator_stanga);
    print_orizontala();
    printf("%c", separator_central);
    print_orizontala();
    printf("%c", separator_central);
    print_orizontala();
    printf("%c\n", separator_dreapta);
}

void print_linie_jos() // functie care printeaza latura de jos a patratului cu colturi si delimitari intre ultimele 3 casute 
{
    printf("  %c", colt_stanga_jos);
    print_orizontala();
    printf("%c", separator_jos);
    print_orizontala();
    printf("%c", separator_jos);
    print_orizontala();
    printf("%c\n", colt_dreapta_jos);
}

void print_celule(int rand) // functie care printeaza laturile verticale ale patratului la distanta de o "casuta" si primeste ca parametru un intreg reprezentand numarul randului din tabla
{
    printf("%d %c  %c  %c  %c  %c  %c  %c\n", rand + 1, verticala, tabla[rand][0], verticala, tabla[rand][1], verticala, tabla[rand][2], verticala);
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

int verificare_castig() 
{
    int i;
    int j;

    // verificare linii
    for(i = 0; i < 3; i++) 
    {
        if((tabla[i][0] != ' ') && (tabla[i][0] == tabla[i][1]) && (tabla[i][1] == tabla[i][2]))
        {
            return 1;
        }
    }

    // verificare coloane
    for(j = 0; j < 3; j++) 
    {
        if((tabla[0][j] != ' ') && (tabla[0][j] == tabla[1][j]) && (tabla[1][j] == tabla[2][j]))
        {
            return 1;
        }
    }

    // verificare diagonale
    if((tabla[0][0] != ' ') && (tabla[0][0] == tabla[1][1]) && (tabla[1][1] == tabla[2][2]))
    {
        return 1;
    }
    if((tabla[0][2] != ' ') && (tabla[0][2] == tabla[1][1]) && (tabla[1][1] == tabla[2][0]))
    {
        return 1;
    }

    return 0;
}

void reseteaza_tabla()
{
    int i;
    int j;

    for(i = 0; i < 3; i++)
    {
        for(j = 0; j < 3; j++)
        {
            tabla[i][j] = ' ';
        }
    }
}

// Conversia coordonatelor (ex. "A1") in indicii matricei
int coordonate_la_indici(char* coordonate, int* rand, int* coloana) 
{
    if(strlen(coordonate) != 2) 
    {
        return 0; // coordonate invalide
    }
    
    char litera = toupper(coordonate[0]); // transformam litera in majuscula
    char cifra = coordonate[1];

    if(litera < 'A' || litera > 'C' || cifra < '1' || cifra > '3') 
    {
        return 0; // coordonate invalide
    }

    *rand = cifra - '1'; // convertim '1', '2', '3' in 0, 1, 2
    *coloana = litera - 'A'; // convertim 'A', 'B', 'C' in 0, 1, 2

    return 1; // coordonate invalide
}

void scrie_X_sau_0() 
{
    int flag = 0; // 0 pentru X, 1 pentru 0
    char caracter;
    int rand, coloana;
    int mutari; 
    char coordonate[3];
    char optiune[3];
    char jucator1[50], jucator2[50];

    printf("Introduceti numele jucatorului pentru X: ");
    scanf("%s", jucator1);
    printf("Introduceti numele jucatorului pentru 0: ");
    scanf("%s", jucator2);

    while(1)
    {
        caracter = 'X';
        flag = 0; // Resetam la jucatorul X la inceputul fiecarui joc

        for(mutari = 0; mutari < 9; mutari++) 
        { 
            printf("%s, introdu pozitia pentru %c (ex: A1): ", flag == 0 ? jucator1 : jucator2, caracter);
            scanf("%s", coordonate);

            if(!coordonate_la_indici(coordonate, &rand, &coloana)) 
            {
                printf("S-a introdus o pozitie invalida. Incearca din nou.\n");
                mutari--; 
                continue;
            }

            if(tabla[rand][coloana] != ' ') 
            {
                printf("Pozitia este deja ocupata. Incearca din nou.\n");
                mutari--; 
                continue;
            }

            tabla[rand][coloana] = caracter;

            print_tabla();

            if(verificare_castig()) 
            {
                printf("Felicitari, %s! Ai castigat!\n", flag == 0 ? jucator1 : jucator2);
                break;
            }

            caracter = (caracter == 'X') ? '0' : 'X';
            flag = 1 - flag; 
        }

        if(verificare_castig() == 0 && mutari == 9)
        {
            printf("Remiza.\n");
        }

        reseteaza_tabla();
        printf("Jucati din nou? Introduceti Da sau Nu: ");
        scanf("%s", optiune);

        if(strcmp(optiune, "Da") != 0)
        {
            printf("Bye.\n");
            break;
        }

        print_tabla();
    }
}

int main() 
{
    print_tabla();
    scrie_X_sau_0();
    return 0;
}