#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#define MAX 100000

// gcc -o dyspozytornia Dyspozytornia.c
// ./dyspozytornia "kolejka" 1000 5 3 4



int kurierzy[9] = {1,1,1,1,1,1,1,1,1};
int magazyny[3] = {1,1,1};
// Gdy konczy sie czas kuriera, wysyla on signala ze sie skonczyl z powodu czasu, w moim przypadku kurierzy sa rozroznialni wiec potrzeba 9 signalow
void handler14(){
    printf("Kurier 1 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[0] = 0;
}
void handler1(){
    printf("Kurier 2 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[1] = 0;
}
void handler13(){
    printf("Kurier 3 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[2] = 0;
}
void handler15(){
    printf("Kurier 4 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[3] = 0;
}
void handler16(){
    printf("Kurier 5 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[4] = 0;
}
void handler17(){
    printf("Kurier 6 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[5] = 0;
}
void handler19(){
    printf("Kurier 7 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[6] = 0;
}
void handler4(){
    printf("Kurier 8 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[7] = 0;
}
void handler5(){
    printf("Kurier 9 sie wylaczyl z powodu kryterium czasowego\n");
    kurierzy[8] = 0;
}

int main(int argc, char* argv[]){

    if (argc-1 != 5){
        printf("Podano zla liczbe argumetow: %d! Spodziewana liczba to 5!\n", argc-1);
        exit(1);
    }
    // pobranie danych z parametrow oraz konwersja ze string do int (long)
    char *klucz = argv[1];

    int liczba_zamowien = strtol(argv[2], NULL, 10);
    int max_A = strtol(argv[3], NULL, 10);
    int max_B = strtol(argv[4], NULL, 10);
    int max_C = strtol(argv[5], NULL, 10);

    if(liczba_zamowien <= 0 || max_A < 0 || max_B < 0 || max_C < 0){
        printf("Liczba zamowien nie moze byc niedodatnia, a maksymalne liczby surowcow nie moga byc ujemne!\n");
        exit(0);
    }
    if(max_A == 0 && max_B == 0 && max_C == 0){
        printf("Wszystkie maksymalne liczby surowcow sa rowne 0! Zamowienie niemozliwe do skonstruowania!!\n");
        exit(0);
    }

    int GLD = 0;

    srand(time(NULL));


    printf("Dyspozytornia zaczela prace, klucz to: %s\nliczba zamowien wynosi: %d\nmaksymalna liczba parametru A wynosi: %d\nmaksymalna liczba parametru b wynosi: %d\nmaksymalna liczba parametru C wynosi: %d\n", klucz, liczba_zamowien, max_A, max_B, max_C);

    mkfifo(klucz, 0640);
    mkfifo("kpowrot", 0640);

    sleep(4);
    int pid = getpid();
    for(int i = 0; i<9 ;i++){
        int desk = open(klucz, O_WRONLY);
        printf("Trwa nawiazywanie lacznosci z kurierami pid: %d\n", pid);
        write(desk, &pid, 4);
        close(desk);
    }

    signal(14, handler14);
    signal(1, handler1);
    signal(13, handler13);
    signal(15, handler15);
    signal(16, handler16);
    signal(17, handler17);
    signal(6, handler19);
    signal(4, handler4);
    signal(5, handler5);

    for(int zamowienie = 1; zamowienie <= liczba_zamowien; zamowienie++){
        // w mikrosekundach (10^-6 sekundy)
        usleep(500000); //0.5s


        int parametr_A, parametr_B, parametr_C;
        do{
            parametr_A = rand()%(max_A+1);
            parametr_B = rand()%(max_B+1);
            parametr_C = rand()%(max_C+1);
        }
        while(parametr_A == 0 && parametr_B == 0 && parametr_C == 0);
        printf("Zamowienie nr %d: A - %d, B - %d, C - %d\n", zamowienie, parametr_A, parametr_B, parametr_C);

        //wysylamy zamowienie: nr_zamowienia, skladniki
        int desk = open(klucz, O_WRONLY);
        int message[4] = {zamowienie, parametr_A, parametr_B, parametr_C};
        write(desk, &message, 16);
        close(desk);

        desk = open("kpowrot", O_RDONLY);
        int temp_GLD = 0;
        int kurier;
        int pom1 = read(desk, &kurier, 4);
        int pom2 = read(desk, &temp_GLD, 4);

        if(temp_GLD != 0){
            GLD += temp_GLD;
            printf("Kurier %d odebral zamowienie nr %d, do zaplaty %d GLD\n", kurier, zamowienie, temp_GLD);
        }
        else{
            kurierzy[kurier-1] = 0;
            printf("Kurier %d nie odebral zamowienia z powodu braku towaru w magazynie, zamowienie nr %d przepadlo!\n", kurier, zamowienie);
            
            if(kurierzy[0] == 0 && kurierzy[1] == 0 && kurierzy[2] == 0 && magazyny[0] != 0){
                magazyny[0] = 0;
                printf("Magazyn 1 sie zamknal z powodu braku kurierow!\n");
            }
            if(kurierzy[3] == 0 && kurierzy[4] == 0 && kurierzy[5] == 0 && magazyny[1] != 0){
                magazyny[1] = 0;
                printf("Magazyn 2 sie zamknal z powodu braku kurierow!\n");
            }
            if(kurierzy[6] == 0 && kurierzy[7] == 0 && kurierzy[8] == 0 && magazyny[2] != 0){
                magazyny[2] = 0;
                printf("Magazyn 3 sie zamknal z powodu braku kurierow!\n");
            }
        }
        if(magazyny[0] == 0 && magazyny[1] == 0 && magazyny[2] == 0){
                printf("Wszystkie magazyny zostaly zamkniete, dyspozytornia sie zamyka!!! Laczne wydatki wyniosly %d GLD\n", GLD);
                unlink(klucz);
                unlink("kpowrot");
                exit(0);
        }
    }

    for(int i =0;i<9;i++){
        int desk = open(klucz, O_WRONLY);
        int message[4] = {0, 0, 0, 0};
        write(desk, &message, 16);
        close(desk);
    }
    printf("WSZYSTKIE ZAMOWIENIA ZOSTALY ZREALIZOWANE, wydatki wyniosly %d GLD\n", GLD);
    unlink(klucz);
    unlink("kpowrot");
    

}
