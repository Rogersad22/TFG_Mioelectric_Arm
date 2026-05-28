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

#include "Circular_Buffer_FIR.h"

adc_channel_t channels[2] = {ADC_CHANNEL_2, ADC_CHANNEL_5};
adc_unit_t units[2] = {ADC_UNIT_1, ADC_UNIT_1};

adc_continuous_handle_t adc_handle; 
TaskHandle_t cb_task;
TaskHandle_t SPI_Task;

adc_cali_handle_t cali_handle_ch2 = NULL;
adc_cali_handle_t cali_handle_ch5 = NULL;

int i = 1;
bool estat_buff1_free = true;
bool estat_buff2_free = true;

#define MISO_GPIO 12
#define MOSI_GPIO 13
#define SPI_CLK 15
#define SPI_CSS 14
#define MODE 0

typedef struct
{
        uint8_t *buffer;
        uint32_t len;
} adc_frame_t;

QueueHandle_t GlobalQueue;

uint8_t *dma_buffer1;
uint8_t *dma_buffer2;

// uint8_t *dma_buffer3;
// uint8_t *dma_buff;

double Senyal_EMG_Filtrada_Rectificada_Canal2[4800];
double Senyal_EMG_Filtrada_Rectificada_Canal5[4800];

void initializeBuffer(CircularBuffer1 *cb, CircularBuffer2 *cb2)
{
        cb->write_index = 0;
        cb->read_index = 0;
        cb->contador = 0;

        cb2->write_index2 = 0;
        cb2->read_index2 = 0;
        cb2->contador2 = 0;
}

double ProcessatFIR_CH2(CircularBuffer *cb, double senyal_EMG)

{
        double FIR = 0.0;

        if (!isFULL(cb))
        {

                cb->buffer[cb->write_index] = senyal_EMG;
                cb->read_index = cb->write_index;

                //printf("Senyal EMG %lf\n", senyal_EMG);
                //printf("Read index %d\n", cb->read_index);

                // Fem la convolució, multiplicant la mostra per els 251 coeficients
                for (int i = 0; i < 251; i++)
                {
                        FIR += cb->buffer[cb->read_index] * Coeficients_FIR[i];
                        FIR = fabs(FIR); //Rectifiquem el senyal. 
                        cb->read_index--; // Endererim per multiplicar cada coef pel valor del senyal. Explicat a la llibreta.
                        if (cb->read_index < 0)
                        {
                                cb->read_index = 251 - 1;
                        }
                }

                cb->write_index = (cb->write_index + 1) % BUFFER_SIZE;
                //printf("write index %d\n", cb->write_index);
        }
        else
        {
                printf("Buffer PLE");
        }

        return FIR;
}

double ProcessatFIR_CH5(CircularBuffer2 *cb, double senyal_EMG)
{

        double FIR = 0.0;

        if (!isFULL(cb))
        {
                // EXPLICACIÓ PER EL PRIMER CAS

                // Guardem la primera mostra al buffer circular quan el write_index = 0.
                cb->buffer2[cb->write_index2] = senyal_EMG;
                cb->read_index2 = cb->write_index2;

                //printf("Senyal EMG %lf\n", senyal_EMG);
                //printf("Read index %d\n", cb->read_index);

                // Fem la convolució, multiplicant la mostra per els 251 coeficients
                for (int i = 0; i < 251; i++)
                {
                        FIR += cb->buffer2[cb->read_index2] * Coeficients_FIR[i];
                        FIR = fabs(FIR); //Rectifiquem el senyal. 
                        cb->read_index2--; // Endererim per multiplicar cada coef pel valor del senyal. Explicat a la llibreta.
                        if (cb->read_index2 < 0)
                        {
                                cb->read_index2 = 251 - 1;
                        }
                }

                cb->write_index2 = (cb->write_index2 + 1) % BUFFER_SIZE;
                printf("write index %d\n", cb->write_index2);
        }
        else
        {
                printf("Buffer PLE");
        }

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
        adc_frame_t frame;
        int mitjana1, mitjana2;
        int count = 0;
        int voltatge_mv1 = 0, voltatge_mv2 = 0;

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

          
                                if (canal == ADC_CHANNEL_2)
                                {
                                        Senyal_EMG_Filtrada_Rectificada_Canal2 = ProcessatFIR_CH2(&cb, voltatge_mv1);
                                }
                                else if (canal == ADC_CHANNEL_5)
                                {
                                        Senyal_EMG_Filtrada_Rectificada_Canal5 = ProcessatFIR_CH5(&cb2, voltatge_mv2);
                                }

                                count = 0;
                                suma1 = suma2 = 0;

                                // printf("raw val=0x%08lX | ch=%d | data=%d\n", p->val, p->type2.channel, p->type2.data);
                        }
                        count++;
                }
        }
}

void ADC_Init(adc_channel_t *channels, uint8_t numChannels)
{
        adc_continuous_handle_cfg_t handle_config = {
            .max_store_buf_size = 160, // per obtenir 20mostres, 10 de cada cannal per exemple es deixa que sigui multiple de conv. Com més gran més latencia
            .conv_frame_size = 80,     // Quan el DMA ha escrit 32 bytes en RAM s'avisa el callback
        };

        ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, &adc_handle));

        adc_continuous_config_t adc_config = {
            .pattern_num = numChannels,
            .sample_freq_hz = 60 * 1000,
            .conv_mode = ADC_CONV_SINGLE_UNIT_1, 
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
            .on_conv_done = callback, 
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

        CircularBuffer1 cb;
        CircularBuffer2 cb2;
        initializeBuffer(&cb, &cb2);

        GlobalQueue = xQueueCreate(2000, sizeof(adc_frame_t));         
        dma_buffer1 = (uint8_t *)heap_caps_malloc(80, MALLOC_CAP_DMA); // buffers definits en l'espai de memòrai correcte
        dma_buffer2 = (uint8_t *)heap_caps_malloc(80, MALLOC_CAP_DMA);
        // dma_buffer3 = heap_caps_malloc(80, MALLOC_CAP_DMA);
        // dma_buff = heap_caps_malloc(80, MALLOC_CAP_DMA);
        ADC_Init(channels, 2);
        ADC_Calibrate();

        // DMA_CONF(dma_buffer1, dma_buffer2);                // Dos canals + numero de canals No necessari
        ESP_ERROR_CHECK(adc_continuous_start(adc_handle)); // Inicia la conversió, si hi ha algun error ho detecta

        xTaskCreate(cbTask, "Callback Task", 4096, NULL, 5, &cb_task);
        xTaskCreate(SPI, "SPI Task", 4096, NULL, 4, &SPI_Task);
}
