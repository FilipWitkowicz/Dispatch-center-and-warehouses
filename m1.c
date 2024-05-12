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

// gcc -o m1 m1.c
// ./m1 ./m1_conf.txt "kolejka"



int main(int argc, char* argv[]){

    // otworzylem plik w takiej formie, aby dzialal fscanf
    FILE * odczyt;
    odczyt = fopen(argv[1], "r");
    if (odczyt < 0){
        printf("Nie udalo sie odczytac pliku konfiguracyjnego!");
        exit(0);
    }


    char *klucz = argv[2];

    int parametr_A,  parametr_B, parametr_C, cena_A, cena_B, cena_C;
    int x = fscanf(odczyt, "%d %d %d %d %d %d", &parametr_A,  &parametr_B, &parametr_C, &cena_A, &cena_B, &cena_C);
    // cos nie dziala jak jst wiecej niz 6 w pliku fscanf chce go odczytac???
    if(x != 6){
        printf("Nie udalo sie odczytac wystarczajacej liczby danych z pliku konfiguracyjnego!\n");
        exit(0);
    }
    if(cena_A == 0 || cena_B == 0 || cena_C == 0){
        printf("Niepoprawny odczyt parametrow! Cena nie powinna sie rownac 0!\n");
        exit(0);
    }
    if(parametr_A < 0 || parametr_B < 0 || parametr_C < 0){
        printf("Niepoprawny odczyt parametrow! Ujemna liczba produktow w magazynie!\n");
        exit(0);
    }
    if(parametr_A == 0 && parametr_B == 0 && parametr_C == 0){
        printf("Magazyn jest pusty!\n");
        exit(0);
    }
    printf("%d %d %d %d %d %d\n", parametr_A,  parametr_B, parametr_C, cena_A, cena_B, cena_C);
    mkfifo("kolejkam1", 0640);

    int czas = 150;

    int zarobione_GLD = 0;
    int magazyn = 1;
    int pid_rodz = getpid();
    int pd1 = fork();
    int pd2 = fork();

    int martwi_kurierzy = 0;



    //rodzic
    if(pd1 != 0 && pd2 != 0){
        while(1){

            if (martwi_kurierzy >= 3){
                printf("Wszyscy kurierzy sie zabili, magazyn %d zarobil %d GLD i sie zamyka\n", magazyn, zarobione_GLD);
                printf("W magazynie pozostalo %d surowca A, %d surowca B, %d surowca C\n", parametr_A, parametr_B, parametr_C);
                unlink("kolejkam1");
                exit(0);
            }

            int desk2 = open("kolejkam1", O_RDONLY);
            int message[4];
            read(desk2, &message, 16);
            close(desk2);

            int nr_zamowienia = message[0];
            int zamowienie_A = message[1];
            int zamowienie_B = message[2];
            int zamowienie_C = message[3];
            int GLD = 0;

            if(nr_zamowienia == 0){
                martwi_kurierzy += 1;
                int message2[4] = {GLD, parametr_A, parametr_B, parametr_C};
                desk2 = open("kolejkam1", O_WRONLY);
                write(desk2, &message2, 16);
                close(desk2);
            }

            else if(parametr_A - zamowienie_A < 0 || parametr_B - zamowienie_B < 0 || parametr_C - zamowienie_C < 0){
                martwi_kurierzy += 1;
                int message2[4] = {GLD, parametr_A, parametr_B, parametr_C};
                desk2 = open("kolejkam1", O_WRONLY);
                write(desk2, &message2, 16);
                close(desk2);
            }
            else{
                parametr_A -= zamowienie_A;
                parametr_B -= zamowienie_B;
                parametr_C -= zamowienie_C; 
                GLD = cena_A*zamowienie_A + cena_B*zamowienie_B + cena_C*zamowienie_C;
                zarobione_GLD += GLD;
                int message2[4] = {GLD, parametr_A, parametr_B, parametr_C};
                desk2 = open("kolejkam1", O_WRONLY);
                write(desk2, &message2, 16);
                close(desk2);
            }
        }
    }
    //kurier1
    if(pd1 == 0 && pd2 != 0){
        int nr_zamowienia, zamowienie_A, zamowienie_B, zamowienie_C;
        int kurier = 1;

        //pobieranie informacji o pidzie dyspozytorni 

        int rozmiar1;
        int pid_dysp;
        while(1){
            int desk = open(klucz, O_RDONLY);
            rozmiar1 = read(desk, &pid_dysp, 4);
            close(desk);
            if(rozmiar1 == 4){
                break;
            }
        }

        if(rozmiar1 == 4){
            printf("Kurier %d pobral informacje o pidzie dyspozytorni! pid: %d\n", kurier, pid_dysp);
            time_t start, stop;
            time(&start);
            time(&stop);
            while(start - stop < czas){
                time(&start);
                //odbieramy zamowienie, nr zamowienia, skladniki
                int desk = open(klucz, O_RDONLY);
                if(desk == -1){
                    printf("FIFO NIE ISTNIEJE");
                    exit(0);
                }
                int message[4];
                int rozmiar = read(desk, &message, 16);
                nr_zamowienia = message[0];
                zamowienie_A = message[1];
                zamowienie_B = message[2];
                zamowienie_C = message[3];
                close(desk);

                if(rozmiar == 16){
                    if(nr_zamowienia == 0){
                        printf("Kurier %d sie zabija. Dyspozytor poinformowal ze skonczyly sie zamowienia!\n", kurier);
                        // kill(pid_rodz, 14);
                        int desk2 = open("kolejkam1", O_WRONLY);
                        write(desk2, &message, 16);
                        close(desk2);

                        int message2[4];
                        desk2 = open("kolejkam1", O_RDONLY);
                        read(desk2, &message2, 16);
                        close(desk2);

                        // jesli kurier sie ma wylaczac nie od razu jak zamknie sie dyspozytornia tylko po czasie to wystarczy zrobic petle while z aktualizacja czasu start
                        // po petli robimy break i kurier sie zabija z powodu kryterium czasu

                        exit(0);
                    }
                    time(&stop);
                    time(&start);
                    printf("Kurier %d odebral zamowienie nr: %d, liczba skladnikow: %d %d %d\n", kurier, nr_zamowienia, zamowienie_A, zamowienie_B, zamowienie_C);
                    

                    int desk2 = open("kolejkam1", O_WRONLY);
                    write(desk2, &message, 16);
                    close(desk2);

                    int message2[4];
                    desk2 = open("kolejkam1", O_RDONLY);
                    read(desk2, &message2, 16);
                    close(desk2);

                    int GLD = message2[0];
                    int zostalo_A = message2[1];
                    int zostalo_B = message2[2];
                    int zostalo_C = message2[3];

                    if(GLD == 0){
                        printf("BRAK PRODUKTU, KURIER %d SIE ZABIL\n", kurier);
                        int desk = open("kpowrot", O_WRONLY);
                        write(desk, &kurier, 4);
                        write(desk, &GLD, 4);
                        close(desk);
                        exit(0);
                    }

                    else{
                        printf("W magazynie %d zostalo parametrow: A-%d B-%d C-%d\n", magazyn, zostalo_A, zostalo_B, zostalo_C);
                        int desk = open("kpowrot", O_WRONLY);
                        write(desk, &kurier, 4);
                        write(desk, &GLD, 4);
                        close(desk);
                    }
                }
            }
            printf("MINAL CZAS, KURIER %d SIE ZABIL   \n", kurier);
            int desk2 = open("kolejkam1", O_WRONLY);
            int message[4] = {0,0,0,0};
            write(desk2, &message, 16);
            close(desk2);

            int message2[4];
            desk2 = open("kolejkam1", O_RDONLY);
            read(desk2, &message2, 16);
            close(desk2);


            // kill(pid_rodz, 14);
            kill(pid_dysp, 14);
            exit(0);
        }
    }
    
    //kurier2
    if(pd1 != 0 && pd2 == 0){
        int nr_zamowienia, zamowienie_A, zamowienie_B, zamowienie_C;
        int kurier = 2;

        //pobieranie informacji o pidzie dyspozytorni
        int rozmiar1;
        int pid_dysp;
        while(1){
            int desk = open(klucz, O_RDONLY);
            rozmiar1 = read(desk, &pid_dysp, 4);
            close(desk);
            if(rozmiar1 == 4){
                break;
            }
        }
        if(rozmiar1 == 4){
            printf("Kurier %d pobral informacje o pidzie dyspozytorni! pid: %d\n", kurier, pid_dysp);
            time_t start, stop;
            time(&start);
            time(&stop);
            while(start - stop < czas){
                time(&start);
                //odbieramy zamowienie, nr zamowienia, skladniki
                int desk = open(klucz, O_RDONLY);
                if(desk == -1){
                    printf("FIFO NIE ISTNIEJE");
                    exit(0);
                }
                int message[4];
                int rozmiar = read(desk, &message, 16);
                nr_zamowienia = message[0];
                zamowienie_A = message[1];
                zamowienie_B = message[2];
                zamowienie_C = message[3];
                close(desk);

                if(rozmiar == 16){
                    if(nr_zamowienia == 0){
                        sleep(1);
                        printf("Kurier %d sie zabija. Dyspozytor poinformowal ze skonczyly sie zamowienia!\n", kurier);
                        // kill(pid_rodz, 14);
                        int desk2 = open("kolejkam1", O_WRONLY);
                        write(desk2, &message, 16);
                        close(desk2);

                        int message2[4];
                        desk2 = open("kolejkam1", O_RDONLY);
                        read(desk2, &message2, 16);
                        close(desk2);


                        exit(0);
                    }
                    time(&stop);
                    time(&start);
                    printf("Kurier %d odebral zamowienie nr: %d, liczba skladnikow: %d %d %d\n", kurier, nr_zamowienia, zamowienie_A, zamowienie_B, zamowienie_C);
                    

                    int desk2 = open("kolejkam1", O_WRONLY);
                    write(desk2, &message, 16);
                    close(desk2);

                    int message2[4];
                    desk2 = open("kolejkam1", O_RDONLY);
                    read(desk2, &message2, 16);
                    close(desk2);

                    int GLD = message2[0];
                    int zostalo_A = message2[1];
                    int zostalo_B = message2[2];
                    int zostalo_C = message2[3];

                    if(GLD == 0){
                        printf("BRAK PRODUKTU, KURIER %d SIE ZABIL\n", kurier);
                        int desk = open("kpowrot", O_WRONLY);
                        write(desk, &kurier, 4);
                        write(desk, &GLD, 4);
                        close(desk);
                        exit(0);
                    }

                    else{
                        printf("W magazynie %d zostalo parametrow: A-%d B-%d C-%d\n", magazyn, zostalo_A, zostalo_B, zostalo_C);
                        int desk = open("kpowrot", O_WRONLY);
                        write(desk, &kurier, 4);
                        write(desk, &GLD, 4);
                        close(desk);
                    }
                }
            }
            sleep(1);
            printf("MINAL CZAS, KURIER %d SIE ZABIL   \n", kurier);
            int desk2 = open("kolejkam1", O_WRONLY);
            int message[4] = {0,0,0,0};
            write(desk2, &message, 16);
            close(desk2);

            int message2[4];
            desk2 = open("kolejkam1", O_RDONLY);
            read(desk2, &message2, 16);
            close(desk2);
            // kill(pid_rodz, 1);
            kill(pid_dysp, 1);
            exit(0);
        }
    }
    //kurier3
    if(pd1 == 0 && pd2 == 0){
        int nr_zamowienia, zamowienie_A, zamowienie_B, zamowienie_C;
        int kurier = 3;

        //pobieranie informacji o pidzie dyspozytorni
        int rozmiar1;
        int pid_dysp;
        while(1){
            int desk = open(klucz, O_RDONLY);
            rozmiar1 = read(desk, &pid_dysp, 4);
            close(desk);
            if(rozmiar1 == 4){
                break;
            }
        }
        if(rozmiar1 == 4){
            printf("Kurier %d pobral informacje o pidzie dyspozytorni! pid: %d\n", kurier, pid_dysp);
            time_t start, stop;
            time(&start);
            time(&stop);
            while(start - stop < czas){
                time(&start);
                //odbieramy zamowienie, nr zamowienia, skladniki
                int desk = open(klucz, O_RDONLY);
                if(desk == -1){
                    printf("FIFO NIE ISTNIEJE");
                    exit(0);
                }
                int message[4];
                int rozmiar = read(desk, &message, 16);
                nr_zamowienia = message[0];
                zamowienie_A = message[1];
                zamowienie_B = message[2];
                zamowienie_C = message[3];
                close(desk);

                if(rozmiar == 16){
                    if(nr_zamowienia == 0){
                        sleep(2);
                        printf("Kurier %d sie zabija. Dyspozytor poinformowal ze skonczyly sie zamowienia!\n", kurier);
                        // kill(pid_rodz, 14);
                        int desk2 = open("kolejkam1", O_WRONLY);
                        write(desk2, &message, 16);
                        close(desk2);

                        int message2[4];
                        desk2 = open("kolejkam1", O_RDONLY);
                        read(desk2, &message2, 16);
                        close(desk2);


                        exit(0);
                    }
                    time(&stop);
                    time(&start);
                    printf("Kurier %d odebral zamowienie nr: %d, liczba skladnikow: %d %d %d\n", kurier, nr_zamowienia, zamowienie_A, zamowienie_B, zamowienie_C);
                    

                    int desk2 = open("kolejkam1", O_WRONLY);
                    write(desk2, &message, 16);
                    close(desk2);

                    int message2[4];
                    desk2 = open("kolejkam1", O_RDONLY);
                    read(desk2, &message2, 16);
                    close(desk2);

                    int GLD = message2[0];
                    int zostalo_A = message2[1];
                    int zostalo_B = message2[2];
                    int zostalo_C = message2[3];

                    if(GLD == 0){
                        printf("BRAK PRODUKTU, KURIER %d SIE ZABIL\n", kurier);
                        int desk = open("kpowrot", O_WRONLY);
                        write(desk, &kurier, 4);
                        write(desk, &GLD, 4);
                        close(desk);
                        exit(0);
                    }

                    else{
                        printf("W magazynie %d zostalo parametrow: A-%d B-%d C-%d\n", magazyn, zostalo_A, zostalo_B, zostalo_C);
                        int desk = open("kpowrot", O_WRONLY);
                        write(desk, &kurier, 4);
                        write(desk, &GLD, 4);
                        close(desk);
                    }
                }
            }
            sleep(2);
            printf("MINAL CZAS, KURIER %d SIE ZABIL   \n", kurier);
            int desk2 = open("kolejkam1", O_WRONLY);
            int message[4] = {0,0,0,0};
            write(desk2, &message, 16);
            close(desk2);

            int message2[4];
            desk2 = open("kolejkam1", O_RDONLY);
            read(desk2, &message2, 16);
            close(desk2);
            // kill(pid_rodz, 13);
            kill(pid_dysp, 13);
            exit(0);
        }
    }


}