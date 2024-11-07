#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>

//definim global caracterele din tabela extinsa ASCII de care vom avea nevoie pentru a desena patratul
char colt_stanga_sus = 218;    // ┌
char colt_dreapta_sus = 191;   // ┐
char colt_stanga_jos = 192;    // └
char colt_dreapta_jos = 217;   // ┘
char orizontala = 196;         // ─
char verticala = 179;          // │
char separator_sus = 194;      // ┬
char separator_jos = 193;      // ┴
char separator_stanga = 195;  // ├
char separator_central = 197;  // ┼
char separator_dreapta = 180;  // ┤

void print_orizontala() // functie care va printa o linie orizontala corespunzatoare unei singure casute din patrat
{
    printf("%c%c%c%c%c", orizontala, orizontala, orizontala, orizontala, orizontala);
}

void print_linie_sus() // functie care printeaza latura de sus a patratului cu colturi si delimitari intre primele 3 casute 
{
    printf("%c", colt_stanga_sus);
    print_orizontala();
    printf("%c", separator_sus);
    print_orizontala();
    printf("%c", separator_sus);
    print_orizontala();
    printf("%c\n", colt_dreapta_sus);
}

void print_linie_mijloc() // functie care printeaza latura de mijloc a patratului cu colturi si delimitari intre cele 3 casute 
{
    printf("%c", separator_stanga);
    print_orizontala();
    printf("%c", separator_central);
    print_orizontala();
    printf("%c", separator_central);
    print_orizontala();
    printf("%c\n", separator_dreapta);
}

void print_linie_jos() // functie care printeaza latura de jos a patratului cu colturi si delimitari intre ultimele 3 casute 
{
    printf("%c", colt_stanga_jos);
    print_orizontala();
    printf("%c", separator_jos);
    print_orizontala();
    printf("%c", separator_jos);
    print_orizontala();
    printf("%c\n", colt_dreapta_jos);
}

void print_celule() // functie care printeaza laturile verticale ale patratului la distanta de o "casuta"
{
    printf("%c     %c     %c     %c\n", verticala, verticala, verticala, verticala);
}

void print_tabla() 
{
    print_linie_sus();   
    print_celule();      
    print_linie_mijloc(); 
    print_celule();      
    print_linie_mijloc(); 
    print_celule();      
    print_linie_jos(); 
}

int main(int argc, char* argv[]) 
{
    print_tabla();
    return 0;
}
