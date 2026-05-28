#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_CLASSES 3
#define NUM_FEATURES 7
#define NUM_SAMPLES 211 // Ajustat a la mida real del teu fitxer

// Matrius per dades i resultats
float Feature_Extractions[NUM_SAMPLES][NUM_FEATURES];
int Prediction_SVM[NUM_SAMPLES];

// Pesos (W) - En SVC OvO:
// fila 0: Classe 0 vs 1
// fila 1: Classe 0 vs 2
// fila 2: Classe 1 vs 2
/*
float weights[NUM_CLASSES][NUM_FEATURES] = {
    {-3.09207295, 0.341360259, 1.39203519, -3.39725737, -0.240353615, -0.761439992, 0.420467186},
    {-0.172082961, -0.00417619369, 2.29275246, -6.48684752, 0.227702716, 0.4591748, 0.0172306253},
    {1.50410258, -0.107608064, 5.5158498, -6.75496519, -0.363511064, 1.97576721, 0.458480969}
};

// Bias (b) per a cada combinació OvO
float bias[NUM_CLASSES] = {-1.78089729, -2.30574566, -0.04319417};
*/
// Nous Pesos (W) extrets de la imatge
/*
float weights[NUM_CLASSES][NUM_FEATURES] = {
    {-3.58036581, -0.07709288, 0.90082239, -3.07732054, 0.29829732, -1.19810158, 0.62071196},
    {0.18590442, -0.16688174, 2.22184659, -7.05278577, 0.06850051, 0.61774409, -0.09237074},
    {1.35176664, -0.19274083, 5.4610753, -6.52574261, -0.28969333, 2.01853293, 0.35252828}
};

// Nous Bias (b) extrets de la imatge
float bias[NUM_CLASSES] = {-1.59345921, -2.38064735, -0.1810491};
*/

// Nous Pesos (W) extrets de la segona imatge
/*
float weights[NUM_CLASSES][NUM_FEATURES] = {
    {-3.10912952, 0.30154562, 1.40123519, -3.34565737, -0.220153615, -0.76143999, 0.45046718},
    {-0.15208296, -0.01417619, 2.24275246, -6.42684752, 0.23770271, 0.45917480, 0.01723062},
    {1.51410258, -0.11760806, 5.51584980, -6.78496519, -0.34351106, 1.97576721, 0.48848096}
};

// Nous Bias (b) extrets de la segona imatge
float bias[NUM_CLASSES] = {-1.76089729, -2.31574566, -0.05319417};
*/

/*
float weights[NUM_CLASSES][NUM_FEATURES] = {
    {-3.72694343, 0.18357369, 1.198512, -3.13132731, 0.07301549, -1.0829009, 0.69490841},
    {0.36137317, -0.09327572, 2.05889867, -7.31576893, 0.197072, 0.64217983, 0.01629949},
    {1.40828765, -0.27001168, 5.50434016, -6.60097747, -0.27815645, 2.1636527, 0.3481127}};

// Nous Bias (b) extrets de la segona imatge
float bias[NUM_CLASSES] = {-1.42308575, -2.45034164, -0.19318406};
*/
float weights[NUM_CLASSES][NUM_FEATURES] = {
    {-2.48034033, 0.11212455, -1.10545507, -1.04048779, -0.19900071, -1.36819265, 0.82353578},
    {-1.77942692, 0.06294023, -0.58432685, -1.27357369, -0.53066175, 0.31222742, -0.22674487},
    {2.03278852, 0.40560687, 2.22696351, -4.8411432, -0.71682096, 1.54048824, -0.3479578}
};

float bias[NUM_CLASSES] = {
    -2.80727151,
    -1.39952904,
    0.06097139
};

// FUNCIÓ CORREGIDA: Lògica One-vs-One amb votació
int Prediccio_SVM_linear(float *input)
{
    int vots[3] = {0, 0, 0};
    float score;

    // 1. Comparació Classe 0 vs Classe 1
    score = bias[0];
    for (int j = 0; j < NUM_FEATURES; j++)
        score += weights[0][j] * input[j];
    if (score > 0)
        vots[0]++;
    else
        vots[1]++;

    // 2. Comparació Classe 0 vs Classe 2
    score = bias[1];
    for (int j = 0; j < NUM_FEATURES; j++)
        score += weights[1][j] * input[j];
    if (score > 0)
        vots[0]++;
    else
        vots[2]++;

    // 3. Comparació Classe 1 vs Classe 2
    score = bias[2];
    for (int j = 0; j < NUM_FEATURES; j++)
        score += weights[2][j] * input[j];
    if (score > 0)
        vots[1]++;
    else
        vots[2]++;

    // Guanya la classe amb més vots
    int max_vots = -1;
    int best_class = 0;
    for (int i = 0; i < 3; i++)
    {
        if (vots[i] > max_vots)
        {
            max_vots = vots[i];
            best_class = i;
        }
    }
    return best_class;
}

int main(void)
{
    FILE *file;
    FILE *file_2;
    float dummy_etiqueta; // Per llegir la 8a columna (l'etiqueta real)
    int ticks = 0;

    file = fopen("FE_Programa_C_exterior.txt", "r"); // Amb V6 un 80%
    file_2 = fopen("Prediccions_Model_c_v2.txt", "w");

    if (file == NULL || file_2 == NULL)
    {
        printf("ERROR OBERTURA FITXERS\n");
        return 1;
    }

    printf("Iniciant processament...\n");

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        // Llegim 7 valors double i 1 double final (l'etiqueta)
        // HEM TREIET el ";" que tenies al final de l'if, que trencava el codi

        if (fscanf(file, "%f,%f,%f,%f,%f,%f,%f",
                   &Feature_Extractions[i][0], &Feature_Extractions[i][1],
                   &Feature_Extractions[i][2], &Feature_Extractions[i][3],
                   &Feature_Extractions[i][4], &Feature_Extractions[i][5],
                   &Feature_Extractions[i][6]) == 7)
        {

            // Fem la predicció OvO
            int resultat = Prediccio_SVM_linear(Feature_Extractions[i]);
            Prediction_SVM[i] = resultat;
            if (resultat == dummy_etiqueta)
            {
                ticks++;
            }

            // Guardem: Index, Predicció, Etiqueta Real (per comparar)
            fprintf(file_2, "%d,%d,%.0f\n", i, resultat, dummy_etiqueta);
        }
        else
        {
            printf("Fi de fitxer o error de format a la línia %d\n", i);
            break;
        }
    }
    float resultat2 = ((float)ticks / (float)NUM_SAMPLES) * 100;
    printf("El nombre d'acerts és: %f\n", resultat2);

    fclose(file);
    fflush(file_2);
    fclose(file_2);

    printf("OK. Processament finalitzat.\n");
    return 0;
}
/*
        if (fscanf(file, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                   &Feature_Extractions[i][0], &Feature_Extractions[i][1],
                   &Feature_Extractions[i][2], &Feature_Extractions[i][3],
                   &Feature_Extractions[i][4], &Feature_Extractions[i][5],
                   &Feature_Extractions[i][6], &dummy_etiqueta) == 8)
        {*/