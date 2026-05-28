#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

void main(void)
{
        int canal2_ordre1 = 0;
        int canal5_ordre1 = 0;
        int canal2_ordre2 = 0;
        int canal5_ordre2 = 0;

        int resultat_SVM[4] = {canal2_ordre1, canal5_ordre1, canal2_ordre2, canal5_ordre2};
        uint32_t estat_actual;

        static bool bloqueig_per_zero = true;
        static bool ma_oberta = false;
        static bool posicio_horitzontal = false;

        FILE *file;

        file = fopen("Pos_motors", "r");

        if (file == NULL)
        {
                printf("ERROR OBERTURA 1");
                return;
        }

        for (int i = 0; i < 34; i++)
        {

                if (fscanf(file, "%d,%d,%d,%d", &resultat_SVM[0], &resultat_SVM[1], &resultat_SVM[2], &resultat_SVM[3]) != 4) // Va guardant cada dada del file en la varaible senyal_EMG[i]
                {
                        printf("ERROR");
                        return;
                }
                else
                {
                        estat_actual = ((resultat_SVM[0] * 1000) + (resultat_SVM[1] * 100) + (resultat_SVM[2] * 10) + (resultat_SVM[3]));
                        switch (estat_actual)
                        {
                        case 0000:
                                if (ma_oberta == false && posicio_horitzontal == false)
                                {
                                        bloqueig_per_zero = false;
                                }
                                else if (ma_oberta == true || posicio_horitzontal == true)
                                {
                                        bloqueig_per_zero = false;
                                }

                                break;

                        case 1010:
                                if (bloqueig_per_zero == false && ma_oberta == false)
                                {
                                        ma_oberta = true;
                                        /*Obertura de mà*/
                                        printf("Obertura de mà\n");
                                        bloqueig_per_zero = true;
                                }
                                else if (bloqueig_per_zero == false && ma_oberta == true)
                                {
                                        ma_oberta = false;
                                        /*Tancament de mà*/
                                        printf("Tancament de mà\n");
                                        bloqueig_per_zero = true;
                                }

                                break;

                        case 202:
                                if (bloqueig_per_zero == false && posicio_horitzontal == false)
                                {
                                        posicio_horitzontal = true;
                                        printf("Posició horitzontal\n");
                                        bloqueig_per_zero = true;
                                }
                                else if (bloqueig_per_zero == false && posicio_horitzontal == true)
                                {
                                        posicio_horitzontal = false;
                                        printf("Posició vertical\n");
                                        bloqueig_per_zero = true;
                                }

                        default:
                                break;
                        }
                }
        }

        // static bool anterior_obertura = false; // True si esta obert
        // static bool anterior_rotacio = false;  // True si este en horitzontal
        /*
        Canal2: 1 o 0
        Canal5: 2 o 0 (en teoria)
        */
}