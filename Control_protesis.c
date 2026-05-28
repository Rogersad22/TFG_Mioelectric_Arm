#include "sdkconfig.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "rom/lldesc.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "driver/mcpwm_prelude.h"
#include "Circular_Buffer_FIR.h"

#define NUM_CLASSES 3
#define NUM_FEATURES 7
#define NUM_SAMPLES 211

float Feature_Extractions[NUM_SAMPLES][NUM_FEATURES];
int Prediction_SVM[NUM_SAMPLES];
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
    {2.03278852, 0.40560687, 2.22696351, -4.8411432, -0.71682096, 1.54048824, -0.3479578}};

float bias[NUM_CLASSES] = {
    -2.80727151,
    -1.39952904,
    0.06097139};

adc_channel_t channels[2] = {ADC_CHANNEL_2, ADC_CHANNEL_5};
adc_unit_t units[2] = {ADC_UNIT_1, ADC_UNIT_1};

adc_continuous_handle_t adc_handle; // controlador per gestionar el mostreig analogic d'alta veolcitat amb DMA

TaskHandle_t cb_task;
TaskHandle_t SPI_Task;
TaskHandle_t FE_Task;
TaskHandle_t SVM_Task;
TaskHandle_t Motors_Task;

CircularBuffer1 cb;
CircularBuffer2 cb2;

adc_cali_handle_t cali_handle_ch2 = NULL;
adc_cali_handle_t cali_handle_ch5 = NULL;

bool estat_buff1_free = true;
bool estat_buff2_free = true;

typedef struct
{
        uint8_t *buffer;
        uint32_t len;
} adc_frame_t;

typedef struct
{
        uint8_t canal_id;
        float *punter_dades;
} FE_Zscore_t;

typedef struct
{
        uint8_t id_canal;
        float vector_FE[7];
} SVM_Linear_t;

typedef struct
{
        uint8_t resposta;
} Control_Motors_t;

QueueHandle_t GlobalQueue;
QueueHandle_t GlobalQueue2;
QueueHandle_t GlobalQueue3;
QueueHandle_t GlobalQueue4;

uint8_t *dma_buffer1;
uint8_t *dma_buffer2;

// uint8_t *dma_buffer3;
// uint8_t *dma_buff;

int count_buff1 = 0, count_buff2 = 0;

float derivadaRMS_prev_canal2 = 0.0;
float derivadaWL_prev_canal2 = 0.0;
float derivadaRMS_prev_canal5 = 0.0;
float derivadaWL_prev_canal5 = 0.0;

struct Mu_Sigma
{
        float Mu;
        float Sigma;
};

// Inicialització de l'estructura
/*
struct Mu_Sigma MS_RMS = {89.5396, 56.609};
struct Mu_Sigma MS_RMSD = {20.5886, 28.1323};
struct Mu_Sigma MS_MAV = {58.5703, 36.9264};
struct Mu_Sigma MS_WL = {7519.8, 5782.2};
struct Mu_Sigma MS_WLD = {1709, 2316};
struct Mu_Sigma MS_SSC = {127.5304, 24.7379};
struct Mu_Sigma MS_MCR = {48.5719, 14.8822};
*/
struct Mu_Sigma MS_RMS = {123.8388, 86.3108};
struct Mu_Sigma MS_RMSD = {35.0827, 39.0164};
struct Mu_Sigma MS_MAV = {75.244, 52.6242};
struct Mu_Sigma MS_WL = {10811, 8388.5};
struct Mu_Sigma MS_WLD = {2717.4, 3207};
struct Mu_Sigma MS_SSC = {140.6682, 25.2682};
struct Mu_Sigma MS_MCR = {51.097, 18.7757};
static const char *TAG = "SERVO_CONTROL";

// Configuració del hardware pel servo

#define SERVO_GPIO_PIN_OBRIR 17    // Servo 1: Obrir/Tancar
#define SERVO_GPIO_PIN_ROTACIO 4   // Servo 2: Rotació
#define SERVO_TIMEBASE_RES 1000000 // 1MHz
#define SERVO_PERIOD_TICKS 20000   // 20ms (50Hz)

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500

static uint32_t angle_to_compare(int angle)
{
        if (angle < 0)
                angle = 0;
        if (angle > 180)
                angle = 180;
        return (uint32_t)(SERVO_MIN_PULSEWIDTH_US + (((SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) * angle) / 180));
}

void initializeBuffer(CircularBuffer1 *cb, CircularBuffer2 *cb2)
{
        cb->write_index = 0;
        cb->read_index = 0;
        cb->contador = 0;

        cb2->write_index2 = 0;
        cb2->read_index2 = 0;
        cb2->contador2 = 0;
}

float calculateRMS(float dades_RMS[])
{
        float sum_squares = 0.0;
        for (int i = 0; i < 450; i++)
        {
                sum_squares += dades_RMS[i] * dades_RMS[i]; // Square
        }
        float mean_squares = sum_squares / 450; // Mean
        return sqrt(mean_squares);              // Root
}

float calculateMEAN(float dades_MEAN[])
{
        float sum_squares = 0.0;
        for (int i = 0; i < 450; i++)
        {
                sum_squares += dades_MEAN[i]; // Square
        }
        float mean_squares = sum_squares / 450; // Mean
        return (mean_squares);                  // Root
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
        static float slope[450];
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

float ProcessatFIR_CH2(CircularBuffer1 *cb, float senyal_EMG)

{
        float FIR = 0.0;

        // Escriure sempre (buffer circular real)
        cb->buffer[cb->write_index] = senyal_EMG;
        cb->write_index = (cb->write_index + 1) % BUFFER_SIZE;

        // Actualitzar contador fins a omplir
        if (cb->contador < BUFFER_SIZE)
                cb->contador++;

       
        if (cb->contador < BUFFER_SIZE)
                return 0.0;

       
        cb->read_index = (cb->write_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;

        // Convolució FIR
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
                FIR += cb->buffer[cb->read_index] * Coeficients_FIR[i];
                cb->read_index = (cb->read_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;
        }

        // Rectificació final
        FIR = fabs(FIR);

        return FIR;
}

float ProcessatFIR_CH5(CircularBuffer2 *cb2, float senyal_EMG)
{
        float FIR = 0.0;

        // Escriure sempre (buffer circular real)
        cb2->buffer2[cb2->write_index2] = senyal_EMG;
        cb2->write_index2 = (cb2->write_index2 + 1) % BUFFER_SIZE;

        // Actualitzar contador fins a omplir
        if (cb2->contador2 < BUFFER_SIZE)
                cb2->contador2++;

  
        if (cb2->contador2 < BUFFER_SIZE)
                return 0.0;

        cb2->read_index2 = (cb2->write_index2 - 1 + BUFFER_SIZE) % BUFFER_SIZE;

        // Convolució FIR
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
                FIR += cb2->buffer2[cb2->read_index2] * Coeficients_FIR[i];
                cb2->read_index2 = (cb2->read_index2 - 1 + BUFFER_SIZE) % BUFFER_SIZE;
        }

        // Rectificació final
        FIR = fabs(FIR);

        return FIR;
}

bool IRAM_ATTR callback(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) // IRAM_ATTR = Internal RAM attribute
{
        BaseType_t mustYield = pdFALSE;
        vTaskNotifyGiveFromISR(cb_task, &mustYield); // s'envia una notificació que el frame s'ha omplert a la cbTask.
        return (mustYield == pdTRUE);
}

void cbTask(void *parameters)
{

        uint32_t rxLen = 0;
        uint8_t *p_buffer_active = dma_buffer1; // buffer actiu es el punter a buffer1

        for (;;) // bucle infinit
        {
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                esp_err_t ret = adc_continuous_read(adc_handle, p_buffer_active, 80, &rxLen, 0);

                if (ret == ESP_OK)
                {

                        adc_frame_t frame = {
                            .buffer = p_buffer_active,
                            .len = rxLen,
                        };
                        // printf("FINS AQUI OK \n");
                        xQueueSend(GlobalQueue, &frame, portMAX_DELAY); // enviem la direcció de memòria del buffer actiu. S'envia l'array de 20 mostres

                        // printf("Valor de rxLEN: %ld\n", rxLen);
                        if (p_buffer_active == dma_buffer1)
                        {
                                p_buffer_active = dma_buffer2; // buffer actiu es el punter a buffer2
                        }
                        else
                        {
                                p_buffer_active = dma_buffer1;
                        }
                }
        }
}

void SPI(void *pvParameters)
{
        // uint8_t *p_buffer_rebut;
        uint32_t suma1 = 0, suma2 = 0;
        int mitjana1, mitjana2;
        int count = 0;
        int voltatge_mv1 = 0, voltatge_mv2 = 0;
        adc_frame_t frame;
        FE_Zscore_t FE_Zs;
        static float Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal2[450];
        static float Buffer2_Senyal_EMG_Filtrada_Rectificada_Canal2[450];
        float *punter_memoria_1 = Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal2; // inicialment per el primer buffer
        static float Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal5[450];
        static float Buffer2_Senyal_EMG_Filtrada_Rectificada_Canal5[450];
        float *punter_memoria_2 = Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal5;

        while (1)
        {
                // printf("FINS AQUI OK 2 \n");
                if (xQueueReceive(GlobalQueue, &frame, pdMS_TO_TICKS(100)) != pdTRUE)
                        continue;
                /*static int64_t start_time = 0;
                if (mostres_enviades == 0 && start_time == 0)
                {
                    start_time = esp_timer_get_time();
                }*/

                for (int i = 0; i < frame.len; i += SOC_ADC_DIGI_RESULT_BYTES)
                {
                        adc_digi_output_data_t *p = (adc_digi_output_data_t *)&frame.buffer[i]; // per obtenir tots esl bits de la conversio, ja que es un array de 20 mostres

                        // printf("BUFFER REBUT: %d\n", *p_buffer_rebut[i]);
                        uint16_t canal = p->type2.channel;
                        uint16_t raw_data = p->type2.data;

                        if (canal == ADC_CHANNEL_2)
                        {
                                suma1 += raw_data;
                        }
                        else if (canal == ADC_CHANNEL_5)
                        {
                                suma2 += raw_data;
                        }

                        if (count == 20)
                        {
                                mitjana1 = suma1 / 10.0;
                                // printf("Valor de la mitjana %d", mitjana1);
                                mitjana2 = suma2 / 10.0;
                                // printf("Valor de la mitjana %d", mitjana2);

                                if (cali_handle_ch2 != NULL)
                                {
                                        adc_cali_raw_to_voltage(cali_handle_ch2, mitjana1, &voltatge_mv1);
                                }
                                else
                                {
                                        // printf("ERROR\n");
                                }

                                if (cali_handle_ch5 != NULL)
                                {
                                        adc_cali_raw_to_voltage(cali_handle_ch5, mitjana2, &voltatge_mv2);
                                }
                                else
                                {
                                        // printf("ERROR\n");
                                }
                                punter_memoria_1[count_buff1] = ProcessatFIR_CH2(&cb, voltatge_mv1);
                                punter_memoria_2[count_buff2] = ProcessatFIR_CH5(&cb2, voltatge_mv2);


                                if (count_buff1 >= 450)
                                {
                                        FE_Zs = (FE_Zscore_t){
                                            .punter_dades = punter_memoria_1,
                                            .canal_id = 2,
                                        };
                                        xQueueSend(GlobalQueue2, &FE_Zs, portMAX_DELAY);

                                        if (punter_memoria_1 == Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal2)
                                        {
                                                for (int n = 0; n < 225; n++)
                                                        Buffer2_Senyal_EMG_Filtrada_Rectificada_Canal2[n] = punter_memoria_1[n + 225];

                                                punter_memoria_1 = Buffer2_Senyal_EMG_Filtrada_Rectificada_Canal2;
                                        }
                                        else
                                        {
                                                for (int n = 0; n < 225; n++)
                                                        Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal2[n] = punter_memoria_1[n + 225];

                                                punter_memoria_1 = Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal2;
                                        }

                                        count_buff1 = 225;
                                }

                                if (count_buff2 >= 450)
                                {
                                        FE_Zs = (FE_Zscore_t){
                                            .punter_dades = punter_memoria_2,
                                            .canal_id = 5,
                                        };
                                        xQueueSend(GlobalQueue2, &FE_Zs, portMAX_DELAY);

                                        if (punter_memoria_2 == Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal5)
                                        {
                                                for (int n = 0; n < 225; n++)
                                                        Buffer2_Senyal_EMG_Filtrada_Rectificada_Canal5[n] = punter_memoria_2[n + 225];

                                                punter_memoria_2 = Buffer2_Senyal_EMG_Filtrada_Rectificada_Canal5;
                                        }
                                        else
                                        {
                                                for (int n = 0; n < 225; n++)
                                                        Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal5[n] = punter_memoria_2[n + 225];

                                                punter_memoria_2 = Buffer1_Senyal_EMG_Filtrada_Rectificada_Canal5;
                                        }

                                        count_buff2 = 225;
                                }
                                count_buff1++;
                                count_buff2++;
                                count = 0;
                                suma1 = suma2 = 0;
                        }
                        count++;
                }
        }
}

void FE_Normalitzacio(void *pvParameters)
{

        static float buf_ham[450];
        static float Ham_window[450];
        float threshold = 0.01;
        float derivadaRMS = 0.0;
        float derivadaWL = 0.0;
        float Feature_EXtractions[7];
        // double *punter_FE = Feature_EXtractions;
        FE_Zscore_t FE_Zs;
        SVM_Linear_t SVM_Lin;
        int count_canal5 = 0;
        int count_canal2 = 0;

        while (1)
        {
                if (xQueueReceive(GlobalQueue2, &FE_Zs, pdMS_TO_TICKS(100)) == pdTRUE) // Executa el paqut i l'elimina, si no hi ha cap paquet a la cua no executa res.
                {
                        if (FE_Zs.canal_id == 2)
                        {

                                //printf("Rebut bloc del Canal 2\n");
                        }
                        else if (FE_Zs.canal_id == 5)
                        {

                                //printf("Rebut bloc del Canal 5\n");
                        }
                        for (int n = 0; n < 450; n++)
                        {
                                Ham_window[n] = 0.54 - 0.46 * cos((2.0 * M_PI * n) / (450 - 1));
                                buf_ham[n] = Ham_window[n] * FE_Zs.punter_dades[n];
                        }
                        float RMS = calculateRMS(buf_ham);
                        float mean = calculateMEAN(buf_ham); // perquè RMS es sqrt de la mitjana, i volem la mitjana
                        float MAV = calculateMAV(buf_ham);
                        float WL = calculateWL(buf_ham);
                        float SSC = calculSSC(buf_ham, threshold);
                        float MCR = calculMCR(buf_ham, mean);

                        if (FE_Zs.canal_id == 2)
                        {
                                derivadaRMS = fabs(RMS - derivadaRMS_prev_canal2);
                                derivadaWL = fabs(WL - derivadaWL_prev_canal2);

                                if (count_canal2 == 0)
                                {
                                        Feature_EXtractions[1] = 0.0;
                                        Feature_EXtractions[4] = 0.0;
                                        count_canal2++;
                                }
                                else
                                {
                                        Feature_EXtractions[1] = (derivadaRMS - MS_RMSD.Mu) / MS_RMSD.Sigma;
                                        Feature_EXtractions[4] = (derivadaWL - MS_WLD.Mu) / MS_WLD.Sigma;
                                }

                                derivadaRMS_prev_canal2 = RMS;
                                derivadaWL_prev_canal2 = WL;
                        }
                        else if (FE_Zs.canal_id == 5)
                        {
                                derivadaRMS = fabs(RMS - derivadaRMS_prev_canal5);
                                derivadaWL = fabs(WL - derivadaWL_prev_canal5);

                                if (count_canal5 == 0)
                                {
                                        Feature_EXtractions[1] = 0.0;
                                        Feature_EXtractions[4] = 0.0;
                                        count_canal5++;
                                }
                                else
                                {
                                        Feature_EXtractions[1] = (derivadaRMS - MS_RMSD.Mu) / MS_RMSD.Sigma;
                                        Feature_EXtractions[4] = (derivadaWL - MS_WLD.Mu) / MS_WLD.Sigma;
                                }

                                derivadaRMS_prev_canal5 = RMS;
                                derivadaWL_prev_canal5 = WL;
                        }

                        Feature_EXtractions[0] = (RMS - MS_RMS.Mu) / MS_RMS.Sigma;
                        if (Feature_EXtractions[0] > 3.0)
                                Feature_EXtractions[0] = 3.0;
                        if (Feature_EXtractions[0] < -3.0)
                                Feature_EXtractions[0] = -3.0;
                        Feature_EXtractions[2] = (MAV - MS_MAV.Mu) / MS_MAV.Sigma;
                        Feature_EXtractions[3] = (WL - MS_WL.Mu) / MS_WL.Sigma;

                        Feature_EXtractions[5] = (SSC - MS_SSC.Mu) / MS_SSC.Sigma;
                        Feature_EXtractions[6] = (MCR - MS_MCR.Mu) / MS_MCR.Sigma;

                        /*printf("FE: %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
                               Feature_EXtractions[0],
                               Feature_EXtractions[1],
                               Feature_EXtractions[2],
                               Feature_EXtractions[3],
                               Feature_EXtractions[4],
                               Feature_EXtractions[5],
                               Feature_EXtractions[6]);*/

                        SVM_Lin = (SVM_Linear_t){

                            .vector_FE[0] = Feature_EXtractions[0],
                            .vector_FE[1] = Feature_EXtractions[1],
                            .vector_FE[2] = Feature_EXtractions[2],
                            .vector_FE[3] = Feature_EXtractions[3],
                            .vector_FE[4] = Feature_EXtractions[4],
                            .vector_FE[5] = Feature_EXtractions[5],
                            .vector_FE[6] = Feature_EXtractions[6],
                            .id_canal = FE_Zs.canal_id};
                        xQueueSend(GlobalQueue3, &SVM_Lin, portMAX_DELAY);
                }
        }
}

void SVM_Linear_ML(void *pvParameters)
{
        SVM_Linear_t SVM_Lin;
        Control_Motors_t Con_Mot;

        float score;
        while (1)
        {
                if (xQueueReceive(GlobalQueue3, &SVM_Lin, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                        int vots[3] = {0, 0, 0};
                        // Classe 0 vs Classe 1
                        score = bias[0];
                        for (int j = 0; j < NUM_FEATURES; j++)
                                score += weights[0][j] * SVM_Lin.vector_FE[j];
                        if (score > 0)
                                vots[0]++;
                        else
                                vots[1]++;

                        // Classe 0 vs Classe 2
                        score = bias[1];
                        for (int j = 0; j < NUM_FEATURES; j++)
                                score += weights[1][j] * SVM_Lin.vector_FE[j];
                        if (score > 0)
                                vots[0]++;
                        else
                                vots[2]++;

                        // Classe 1 vs Classe 2
                        score = bias[2];
                        for (int j = 0; j < NUM_FEATURES; j++)
                                score += weights[2][j] * SVM_Lin.vector_FE[j];
                        if (score > 0)
                                vots[1]++;
                        else
                                vots[2]++;

                        // Guanya la classe amb més vots
                        int mes_vots = -1;
                        int millor_clase = 0;
                        for (int i = 0; i < 3; i++)
                        {
                                if (vots[i] > mes_vots)
                                {
                                        mes_vots = vots[i];
                                        millor_clase = i;
                                }
                        }

                        //printf("Scores: %d %d %d\n", vots[0], vots[1], vots[2]);

                        Con_Mot = (Control_Motors_t){
                            .resposta = millor_clase};
                        xQueueSend(GlobalQueue4, &Con_Mot, portMAX_DELAY);
                }
        }
}

void Control_Motors(void *pvParameters)
{
        static bool bloqueig_per_zero = true;
        static bool ma_oberta = false;
        static bool posicio_horitzontal = false;

        uint8_t resultat_SVM[4] = {0};
        uint32_t estat_actual = 0;
        int idx = 0;

        Control_Motors_t Con_Mot;

        // --- CONFIG MCPWM (igual que tú) ---
        mcpwm_timer_handle_t timer = NULL;
        mcpwm_timer_config_t timer_config = {
            .group_id = 0,
            .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
            .resolution_hz = SERVO_TIMEBASE_RES,
            .period_ticks = SERVO_PERIOD_TICKS,
            .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
        };
        ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

        mcpwm_oper_handle_t oper = NULL;
        mcpwm_operator_config_t operator_config = {.group_id = 0};
        ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

        mcpwm_cmpr_handle_t comp_obrir = NULL;
        mcpwm_cmpr_handle_t comp_rotacio = NULL;
        mcpwm_comparator_config_t compare_config = {.flags.update_cmp_on_tez = true};

        ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &compare_config, &comp_obrir));
        ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &compare_config, &comp_rotacio));

        mcpwm_gen_handle_t gen_obrir = NULL;
        mcpwm_gen_handle_t gen_rotacio = NULL;

        mcpwm_generator_config_t gen_config_obrir = {.gen_gpio_num = SERVO_GPIO_PIN_OBRIR};
        ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config_obrir, &gen_obrir));

        mcpwm_generator_config_t gen_config_rotacio = {.gen_gpio_num = SERVO_GPIO_PIN_ROTACIO};
        ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config_rotacio, &gen_rotacio));

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
            gen_obrir,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
            gen_obrir,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comp_obrir, MCPWM_GEN_ACTION_LOW)));

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
            gen_rotacio,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
            gen_rotacio,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comp_rotacio, MCPWM_GEN_ACTION_LOW)));

        ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

        // --- LOOP PRINCIPAL ---
        while (1)
        {
                // Bloqueo eficiente (no consume CPU)
                if (xQueueReceive(GlobalQueue4, &Con_Mot, portMAX_DELAY) == pdTRUE)
                {
                        // Guardar muestra
                        resultat_SVM[idx++] = Con_Mot.resposta;

                        // Cuando tenemos 4 → procesamos
                        if (idx == 4)
                        {
                                idx = 0;

                                estat_actual =
                                    (resultat_SVM[0] * 1000) +
                                    (resultat_SVM[1] * 100) +
                                    (resultat_SVM[2] * 10) +
                                    (resultat_SVM[3]);

                                //printf("estat_actual = %lu\n", estat_actual);

                                switch (estat_actual)
                                {
                                case 0000:
                                        bloqueig_per_zero = false;
                                        break;

                                case 2020: // 2222
                                        if (!bloqueig_per_zero)
                                        {
                                                if (!ma_oberta)
                                                {
                                                        ma_oberta = true;
                                                        printf("Obertura de mà\n");
                                                        mcpwm_comparator_set_compare_value(comp_obrir, angle_to_compare(130));
                                                }
                                                else
                                                {
                                                        ma_oberta = false;
                                                        printf("Tancament de mà\n");
                                                        mcpwm_comparator_set_compare_value(comp_obrir, angle_to_compare(80));
                                                }
                                                bloqueig_per_zero = true;
                                        }
                                        break;

                                case 101:
                                        if (!bloqueig_per_zero)
                                        {
                                                if (!posicio_horitzontal)
                                                {
                                                        posicio_horitzontal = true;
                                                        printf("Posició horitzontal\n");
                                                        mcpwm_comparator_set_compare_value(comp_rotacio, angle_to_compare(97));
                                                }
                                                else
                                                {
                                                        posicio_horitzontal = false;
                                                        printf("Posició vertical\n");
                                                        mcpwm_comparator_set_compare_value(comp_rotacio, angle_to_compare(7));
                                                }

                                                bloqueig_per_zero = true;

                                                // Delay controlado (no bloquea sistema crítico)
                                                vTaskDelay(pdMS_TO_TICKS(300));
                                        }
                                        break;

                                default:
                                        printf("No detecta cap moviment\n");
                                        break;
                                }
                        }
                }
        }
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
        printf("STACK OVERFLOW a la task: %s\n", pcTaskName);
        abort();
}

void ADC_Init(adc_channel_t *channels, uint8_t numChannels)
{
        adc_continuous_handle_cfg_t handle_config = {
            .max_store_buf_size = 160, // per obtenir 20mostres, 10 de cada cannal per exemple es deixa que sigui multiple de conv. Com més gran més latencia
            .conv_frame_size = 80,     // Quan el DMA ha escrit 32 bytes en RAM s'avisa el callback
        };

        ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, &adc_handle));

        adc_continuous_config_t adc_config = {
            .pattern_num = numChannels, // Nº od ADC Channels
            //.adc_pattern =,
            .sample_freq_hz = 60 * 1000,
            .conv_mode = ADC_CONV_SINGLE_UNIT_1, // Interleaves between ADC1 and ADC2 to maximize sampling speed.
            .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        };

        adc_digi_pattern_config_t adc_pattern_config[2];
        for (int i = 0; i < 2; i++)
        {
                adc_pattern_config[i].atten = ADC_ATTEN_DB_12;
                adc_pattern_config[i].channel = channels[i];
                adc_pattern_config[i].unit = units[i];
                adc_pattern_config[i].bit_width = ADC_BITWIDTH_12;
        }
        adc_config.adc_pattern = adc_pattern_config;
        ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &adc_config));

        adc_continuous_evt_cbs_t cb_config = {
            .on_conv_done = callback, // When the conversion frame is genereted we call the callback function
        };
        ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cb_config, NULL));
}

void ADC_Calibrate(void)
{
        // Calibració canal 2
        adc_cali_curve_fitting_config_t cali_config_ch2 = {
            .unit_id = ADC_UNIT_1,
            .chan = ADC_CHANNEL_2,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config_ch2, &cali_handle_ch2);
        if (ret == ESP_OK)
                printf("Calibració CH2 OK\n");
        else
        {
                // printf("Calibració CH2 NO disponible\n");
                cali_handle_ch2 = NULL;
        }

        // Calibració canal 5
        adc_cali_curve_fitting_config_t cali_config_ch5 = {
            .unit_id = ADC_UNIT_1,
            .chan = ADC_CHANNEL_5,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config_ch5, &cali_handle_ch5);
        if (ret == ESP_OK)
                printf("Calibració CH5 OK\n");
        else
        {
                // printf("Calibració CH5 NO disponible\n");
                cali_handle_ch5 = NULL;
        }
}

void app_main(void)
{

        initializeBuffer(&cb, &cb2);

        GlobalQueue = xQueueCreate(20, sizeof(adc_frame_t)); // podem posar com a maxim 10 buffers en cua, tamany es un punter.
        GlobalQueue2 = xQueueCreate(20, sizeof(FE_Zscore_t));
        GlobalQueue3 = xQueueCreate(20, sizeof(SVM_Linear_t));
        GlobalQueue4 = xQueueCreate(20, sizeof(Control_Motors_t));
        dma_buffer1 = (uint8_t *)heap_caps_malloc(80, MALLOC_CAP_DMA); // buffers definits en l'espai de memòrai correcte
        dma_buffer2 = (uint8_t *)heap_caps_malloc(80, MALLOC_CAP_DMA);
        // dma_buffer3 = heap_caps_malloc(80, MALLOC_CAP_DMA);
        // dma_buff = heap_caps_malloc(80, MALLOC_CAP_DMA);
        ADC_Init(channels, 2);
        ADC_Calibrate();

        // DMA_CONF(dma_buffer1, dma_buffer2);                // Dos canals + numero de canals No necessari
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle)); // Inicia la conversió, si hi ha algun error ho detecta

        xTaskCreatePinnedToCore(cbTask, "Callback Task", 4096, NULL, 5, &cb_task, 0);
        xTaskCreatePinnedToCore(SPI, "SPI Task", 4096, NULL, 4, &SPI_Task, 0);
        xTaskCreatePinnedToCore(FE_Normalitzacio, "FE_Normalitzacio", 12288, NULL, 4, &FE_Task, 0);
        xTaskCreatePinnedToCore(SVM_Linear_ML, "SVM_Linear", 4096, NULL, 4, &SVM_Task, 1);
        xTaskCreatePinnedToCore(Control_Motors, "Control_Motors", 4096, NULL, 4, &Motors_Task, 1);
}
