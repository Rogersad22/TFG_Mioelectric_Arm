#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "circular_buffer.h"
#include <unistd.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float Coeficients_FIR[251] = {-3.819836317e-19, 4.155086572e-06, 0.0001888578554, 0.0003796207893, 0.0003908814397,
                               0.0002111615177, 2.216271423e-05, 2.529325138e-05, 0.0002387723071, 0.0004673587973,
                               0.0004868496035, 0.0002718554751, 3.799903425e-05, 3.969757381e-05, 0.0003098973539,
                               0.0006051745149, 0.0006328126765, 0.0003516421712, 4.043851732e-05, 3.83800907e-05,
                               0.0003950563259, 0.0007877408643, 0.000821478141, 0.0004373224801, 1.025888469e-05,
                               6.120238628e-18, 0.0004748658394, 0.0009977601003, 0.001033577137, 0.0005034123897,
                               -8.434613119e-05, -0.0001092527164, 0.0005180794396, 0.00120679161, 0.001239039935,
                               0.0005134957028, -0.0002862610854, -0.000333828124, 0.0004839491739, 0.001378170913,
                               0.001400256064, 0.0004235577362, -0.0006460853037, -0.0007251726347, 0.0003264586267,
                               0.001471864642, 0.001477217884, 0.0001870101551, -0.001217298675, -0.001336708083,
                               -1.036956201e-18, 0.001450855169, 0.001434106613, -0.0002391470189, -0.002050632378,
                               -0.002218153793, -0.0005341687356, 0.001288466388, 0.001246712171, -0.0008877796354,
                               -0.003188567236, -0.00341015053, -0.001301506301, 0.0009759900859, 0.0009100523312,
                               -0.001775549608, -0.004661090206, -0.004940351006, -0.002308680676, 0.000530103629,
                               0.0004457305185, -0.002898173174, -0.006484045181, -0.006822351366, -0.003539776895,
                               2.605693045e-17, -9.093305562e-05, -0.004227735102, -0.0086617833, -0.009059280157,
                               -0.004954844713, -0.0005248832167, -0.0006028657663, -0.005712561775, -0.01119676791,
                               -0.01165495347, -0.00649114605, -0.0009035115945, -0.0009387027822, -0.00727984868,
                               -0.01411145274, -0.01463874802, -0.00806712918, -0.0009199206834, -0.000865841168,
                               -0.008840902708, -0.01749604009, -0.01812038012, -0.009588814341, -0.0002238268062,
                               -3.325880161e-17, -0.0102985166, -0.02162416279, -0.02242633887, -0.01095796563,
                               0.001846092404, 0.002410510322, -0.01155570894, -0.02729843929, -0.02852647193,
                               -0.0120811658, 0.006914084312, 0.008321021684, -0.01252488792, -0.03729808331,
                               -0.03996382654, -0.01287879609, 0.02119392529, 0.02607475594, -0.01313638221,
                               -0.06818245351, -0.08207027614, -0.013292877, 0.1245544627, 0.2625336647,
                               0.320288837, 0.2625336647, 0.1245544627, -0.013292877, -0.08207027614,
                               -0.06818245351, -0.01313638221, 0.02607475594, 0.02119392529, -0.01287879609,
                               -0.03996382654, -0.03729808331, -0.01252488792, 0.008321021684, 0.006914084312,
                               -0.0120811658, -0.02852647193, -0.02729843929, -0.01155570894, 0.002410510322,
                               0.001846092404, -0.01095796563, -0.02242633887, -0.02162416279, -0.0102985166,
                               -3.325880161e-17, -0.0002238268062, -0.009588814341, -0.01812038012, -0.01749604009,
                               -0.008840902708, -0.000865841168, -0.0009199206834, -0.00806712918, -0.01463874802,
                               -0.01411145274, -0.00727984868, -0.0009387027822, -0.0009035115945, -0.00649114605,
                               -0.01165495347, -0.01119676791, -0.005712561775, -0.0006028657663, -0.0005248832167,
                               -0.004954844713, -0.009059280157, -0.0086617833, -0.004227735102, -9.093305562e-05,
                               2.605693045e-17, -0.003539776895, -0.006822351366, -0.006484045181, -0.002898173174,
                               0.0004457305185, 0.000530103629, -0.002308680676, -0.004940351006, -0.004661090206,
                               -0.001775549608, 0.0009100523312, 0.0009759900859, -0.001301506301, -0.00341015053,
                               -0.003188567236, -0.0008877796354, 0.001246712171, 0.001288466388, -0.0005341687356,
                               -0.002218153793, -0.002050632378, -0.0002391470189, 0.001434106613, 0.001450855169,
                               -1.036956201e-18, -0.001336708083, -0.001217298675, 0.0001870101551, 0.001477217884,
                               0.001471864642, 0.0003264586267, -0.0007251726347, -0.0006460853037, 0.0004235577362,
                               0.001400256064, 0.001378170913, 0.0004839491739, -0.000333828124, -0.0002862610854,
                               0.0005134957028, 0.001239039935, 0.00120679161, 0.0005180794396, -0.0001092527164,
                               -8.434613119e-05, 0.0005034123897, 0.001033577137, 0.0009977601003, 0.0004748658394,
                               6.120238628e-18, 1.025888469e-05, 0.0004373224801, 0.000821478141, 0.0007877408643,
                               0.0003950563259, 3.83800907e-05, 4.043851732e-05, 0.0003516421712, 0.0006328126765,
                               0.0006051745149, 0.0003098973539, 3.969757381e-05, 3.799903425e-05, 0.0002718554751,
                               0.0004868496035, 0.0004673587973, 0.0002387723071, 2.529325138e-05, 2.216271423e-05,
                               0.0002111615177, 0.0003908814397, 0.0003796207893, 0.0001888578554, 4.155086572e-06,
                               -3.819836317e-19};

int contador1;
float senyal_EMG_filtrada_rectificada[48000];

float Feature_EXtractions[7];

int count = 0;
int count2 = 0;
int R = 450;

float derivadaRMS_prev = 0.0;
float derivadaWL_prev = 0.0;

struct Mu_Sigma
{
        float Mu;
        float Sigma;
};

// Inicialització de l'estructura

struct Mu_Sigma MS_RMS = {123.8388, 86.3108};
struct Mu_Sigma MS_RMSD = {35.0827, 39.0164};
struct Mu_Sigma MS_MAV = {75.244, 52.6242};
struct Mu_Sigma MS_WL = {10811, 8388.5};
struct Mu_Sigma MS_WLD = {2717.4, 3207};
struct Mu_Sigma MS_SSC = {140.6682, 25.2682};
struct Mu_Sigma MS_MCR = {51.097, 18.7757};

/*
void convolucio(double senyal[48000])
{

        double senyal_EMG_filtrada[48000];
        int i, j, k;
        double tmp;

        printf("OK4");

        for (k = 0; k < 48000; k++)
        {
                tmp = 0;
                for (i = 0; i < 251; i++) // for (i = 0; i <= k && i < 251; i++) COM A FUTURA MILLORA
                {
                        j = k - i;
                        if (j >= 0)
                        {
                                tmp += senyal[j] * Coeficients_FIR[i];
                        }
                }
                senyal_EMG_filtrada[k] = tmp;
        }

        printf("OK5\n");

        FILE *file_2;

        file_2 = fopen("Dades_Senyal_Captura14_Roger_Exterior_Filtrat.txt", "w");

        if (file_2 == NULL)
        {
                printf("ERROR OBERTURA");
                return;
        }
        printf("OK22\n");
        for (int r = 0; r < 48000; r++)
        {
                if (fprintf(file_2, "%d,%lf\n", r, senyal_EMG_filtrada[r]) < 0)
                {
                        printf("ERROR AQUI\n");
                        return;
                }
        }
        fclose(file_2);

        printf("OK6\n");

        return;
}*/

void initializeBuffer(CircularBuffer *cb)
{
        cb->write_index = 0;
        cb->read_index = 0;
        cb->contador = 0;
}

int isFULL(CircularBuffer *cb)
{
        return cb->contador == BUFFER_SIZE;
}

int isEmpty(CircularBuffer *cb)
{
        return cb->contador == 0;
}

float ProcessatFIR(CircularBuffer *cb, float senyal_EMG)
{
        float FIR = 0.0;
        float FIR_rec = 0.0;

        cb->buffer[cb->write_index] = senyal_EMG;
        cb->write_index = (cb->write_index + 1) % BUFFER_SIZE;

        cb->read_index = (cb->write_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;

        // Per llegir de la mostra més recent i tirem enrere.

        // printf("Senyal EMG %lf\n", senyal_EMG);
        // printf("Read index %d\n", cb->read_index);

        for (int i = 0; i < 251; i++) // 251 coefiecients del filtre
        {
                FIR += cb->buffer[cb->read_index] * Coeficients_FIR[i];

                cb->read_index = (cb->read_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;
        }

        // cb->write_index = (cb->write_index + 1) % BUFFER_SIZE;

        // printf("write index %d\n", cb->write_index);

        FIR_rec = fabs(FIR);

        // printf("Senyal EMG filtrat i rec: %lf\n", FIR_rec);

        return FIR_rec;
}

/*        if (!isFULL(cb))
        {
                // EXPLICACIÓ PER EL PRIMER CAS

                // Guardem la primera mostra al buffer circular quan el write_index = 0.

                // Fem la convolució, multiplicant la mostra per els 251 coeficients
        }
        else
        {
                printf("Buffer PLE");
        }
        // FIR_rec = fabs(FIR); // Rectifiquem el senyal.
        // return FIR_rec;
}*/

float calculateRMS(float dades_RMS[])
{
        float sum_squares = 0.0;
        for (int i = 0; i < 450; i++)
        {
                sum_squares += dades_RMS[i] * dades_RMS[i]; // Square
        }
        float mean_squares = sum_squares / 450; // Mean
        return sqrt(mean_squares);               // Root
}

float calculateMEAN(float dades_MEAN[])
{
        float sum_squares = 0.0;
        for (int i = 0; i < 450; i++)
        {
                sum_squares += dades_MEAN[i]; // Square
        }
        float mean_squares = sum_squares / 450; // Mean
        return (mean_squares);               // Root
}

float calculateMAV(float dades_MAV[])
{
        float sum = 0.0;

        for (int i = 0; i < 450; i++)
        {
                // Use fabs() for absolute value of doubles, abs() for integers
                sum += fabs(dades_MAV[i]);
        }

        return sum / 450;
}

float calculateWL(float dades_WL[])
{
        float sum = 0.0;

        for (int i = 1; i < 450; i++)
        {
                // Use fabs() for absolute value of doubles, abs() for integers
                sum += fabs((dades_WL[i]) - dades_WL[i - 1]);
        }

        return sum;
}

float calculSSC(float dades_SSC[], float threshold)
{
        float slope[450];
        float segmentSSC = 0.0;

        for (int i = 1; i < 450; i++)
        {
                // Use fabs() for absolute value of doubles, abs() for integers
                slope[i] = ((dades_SSC[i]) - dades_SSC[i - 1]);
        }
        for (int r = 1; r < 449; r++)
        {
                if ((slope[r] > threshold && slope[r + 1] < -threshold) || (slope[r] < -threshold && slope[r + 1] > threshold))
                {
                        segmentSSC++;
                }
        }
        return segmentSSC;
}

float calculMCR(float dades_MCR[], float mean)
{

        float segmentMCR = 0.0;

        for (int r = 0; r < 449; r++)
        {
                if ((dades_MCR[r] > mean && dades_MCR[r + 1] < mean) || (dades_MCR[r] < mean && dades_MCR[r + 1] > mean))
                {
                        segmentMCR++;
                }
        }
        return segmentMCR;
}

float *Hamming_Feature_Extraction(float Senyal_tractada)
{
        float threshold = 0.01;
        static float buf1[450];

        float buf_ham[450];
        float Ham_window[450];
        float derivadaRMS = 0.0;
        float derivadaWL = 0.0;

        if (count < R)
        {
                buf1[count] = Senyal_tractada;
                count++;
        }
        if (count == R)
        {
                for (int n = 0; n < 450; n++)
                {
                        Ham_window[n] = 0.54 - 0.46 * cos((2.0 * M_PI * n) / (450 - 1));
                        buf_ham[n] = Ham_window[n] * buf1[n];
                }
                float RMS = calculateRMS(buf_ham);
                float mean = calculateMEAN(buf_ham); // perquè RMS es sqrt de la mitjana, i volem la mitjana
                float MAV = calculateMAV(buf_ham);
                float WL = calculateWL(buf_ham);
                float SSC = calculSSC(buf_ham, threshold);
                float MCR = calculMCR(buf_ham, mean);

                derivadaRMS = fabs(RMS - derivadaRMS_prev);
                derivadaWL = fabs(WL - derivadaWL_prev);

                // Normalització de les dades

                Feature_EXtractions[0] = (RMS - MS_RMS.Mu) / MS_RMS.Sigma;
                if (Feature_EXtractions[0] > 3.0)
                        Feature_EXtractions[0] = 3.0;
                if (Feature_EXtractions[0] < -3.0)
                        Feature_EXtractions[0] = -3.0;

                if (count2 == 0)
                {
                        Feature_EXtractions[1] = 0.0;
                        Feature_EXtractions[4] = 0.0;
                }
                else
                {
                        Feature_EXtractions[1] = (derivadaRMS - MS_RMSD.Mu) / MS_RMSD.Sigma;
                        Feature_EXtractions[4] = (derivadaWL - MS_WLD.Mu) / MS_WLD.Sigma;
                }
                Feature_EXtractions[2] = (MAV - MS_MAV.Mu) / MS_MAV.Sigma;
                Feature_EXtractions[3] = (WL - MS_WL.Mu) / MS_WL.Sigma;

                Feature_EXtractions[5] = (SSC - MS_SSC.Mu) / MS_SSC.Sigma;
                Feature_EXtractions[6] = (MCR - MS_MCR.Mu) / MS_MCR.Sigma;

                printf("Els valors no normalitzats son %f,%f,%f,%f,%f,%f,%f,%f: \n", RMS, derivadaRMS, MAV, WL, derivadaWL, SSC, MCR, mean);

                                printf("Els valors del vector són %f,%f,%f,%f,%f,%f,%f: \n", Feature_EXtractions[0], Feature_EXtractions[1], Feature_EXtractions[2],
                                       Feature_EXtractions[3], Feature_EXtractions[4], Feature_EXtractions[5], Feature_EXtractions[6]);

                for (int n = 0; n < 225; n++)
                {
                        // Passem el que hi ha a la segona meitat a la primera meitat
                        buf1[n] = buf1[n + 225];
                }
                derivadaRMS_prev = RMS;
                derivadaWL_prev = WL;
                count = 225;

                printf("Fins aqui OK. %d\n", count2);
                count2++;

                return Feature_EXtractions;
        }
        return NULL;
}

int main(void)
{

        float senyal_EMG[48000];
        CircularBuffer cb;
        initializeBuffer(&cb);
        float *senyal_FE_Normalitzada; // Es un punter per indicar nomes la direcció de memoria i no haver de pasar tota la array
        float dummy;

        printf("OK1");

        FILE *file;
        FILE *file_2;

        file = fopen("Dades_Senyal_Captura14_Roger_test1.txt", "r");
        file_2 = fopen("FE_Programa_C_exterior.txt", "w");

        if (file == NULL)
        {
                printf("ERROR OBERTURA 1");
                return 1;
        }

        printf("OK2");

        if (file_2 == NULL)
        {
                printf("ERROR OBERTURA 2");
                return 1;
        }
        printf("OK22\n");

        for (int i = 0; i < 48000; i++)
        {
                if (fscanf(file, "%d,%f,%f", &contador1, &dummy, &senyal_EMG[i]) != 3) // Va guardant cada dada del file en la varaible senyal_EMG[i]
                {
                        printf("ERROR");
                        return 1;
                }
                else
                {
                        senyal_EMG_filtrada_rectificada[i] = ProcessatFIR(&cb, senyal_EMG[i]);

                        // fprintf(file_2, "%d,%lf\n", i, senyal_EMG_filtrada_rectificada[i]);

                        senyal_FE_Normalitzada = Hamming_Feature_Extraction(senyal_EMG_filtrada_rectificada[i]);

                        // printf("Els valors del vector són %lf,%lf,%lf,%lf,%lf,%lf,%lf: \n", Feature_EXtractions[0], Feature_EXtractions[1], Feature_EXtractions[2],
                        //        Feature_EXtractions[3], Feature_EXtractions[4], Feature_EXtractions[5], Feature_EXtractions[6]);
                        if (senyal_FE_Normalitzada != NULL)
                        {
                                fprintf(file_2, "%f, %f, %f, %f, %f, %f, %f\n", senyal_FE_Normalitzada[0], senyal_FE_Normalitzada[1],
                                        senyal_FE_Normalitzada[2], senyal_FE_Normalitzada[3], senyal_FE_Normalitzada[4], senyal_FE_Normalitzada[5], senyal_FE_Normalitzada[6]);
                        }
                }
                // usleep(333);
        }
        fclose(file);
        fclose(file_2);

        printf("OK3");

        // convolucio(senyal_EMG);
}
