#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm_prelude.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

static const char *TAG = "SERVO_CONTROL";

#define SERVO_GPIO_PIN_OBRIR 17    // Servo 1: Obrir/Tancar
#define SERVO_GPIO_PIN_ROTACIO 6   // Servo 2: Rotació
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

void app_main(void)
{
        int resultat_SVM[4];
        uint32_t estat_actual;

        int dades_entrada[34][4] = {
            {0, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 2, 0, 2}, {1, 0, 1, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {0, 2, 0, 2}, {0, 1, 1, 0}, {0, 0, 0, 0}, {1, 0, 1, 0}, {0, 2, 0, 2}, {0, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {1, 1, 1, 1}, {1, 0, 1, 0}, {0, 0, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}, {0, 2, 0, 2}, {0, 0, 0, 0}, {0, 2, 0, 2}, {0, 0, 0, 0}, {1, 0, 1, 0}, {0, 0, 0, 0}};

        static bool bloqueig_per_zero = true;
        static bool ma_oberta = false;
        static bool posicio_horitzontal = false;

        ESP_LOGI(TAG, "Configurando MCPWM para dos servos...");

        // Configurar el Timer (compartit)
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

        // Configurar comparadors
        mcpwm_cmpr_handle_t comp_obrir = NULL;
        mcpwm_cmpr_handle_t comp_rotacio = NULL;
        mcpwm_comparator_config_t compare_config = {.flags.update_cmp_on_tez = true};

        ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &compare_config, &comp_obrir));
        ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &compare_config, &comp_rotacio));

        // Configurar generadors
        mcpwm_gen_handle_t gen_obrir = NULL;
        mcpwm_gen_handle_t gen_rotacio = NULL;

        mcpwm_generator_config_t gen_config_obrir = {.gen_gpio_num = SERVO_GPIO_PIN_OBRIR};
        ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config_obrir, &gen_obrir));

        mcpwm_generator_config_t gen_config_rotacio = {.gen_gpio_num = SERVO_GPIO_PIN_ROTACIO};
        ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gen_config_rotacio, &gen_rotacio));

        // obertura i tancament
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(gen_obrir,
                                                                  MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(gen_obrir,
                                                                    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comp_obrir, MCPWM_GEN_ACTION_LOW)));

        // rotació
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(gen_rotacio,
                                                                  MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(gen_rotacio,
                                                                    MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comp_rotacio, MCPWM_GEN_ACTION_LOW)));

        
        ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
        ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

        ESP_LOGI(TAG, "Servos a punt.");

        for (int i = 0; i < 34; i++)
        {
                resultat_SVM[0] = dades_entrada[i][0];
                resultat_SVM[1] = dades_entrada[i][1];
                resultat_SVM[2] = dades_entrada[i][2];
                resultat_SVM[3] = dades_entrada[i][3];

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
                                mcpwm_comparator_set_compare_value(comp_obrir, angle_to_compare(130));
                                vTaskDelay(pdMS_TO_TICKS(3000));
                                bloqueig_per_zero = true;
                        }
                        else if (bloqueig_per_zero == false && ma_oberta == true)
                        {
                                ma_oberta = false;
                                /*Tancament de mà*/
                                printf("Tancament de mà\n");
                                mcpwm_comparator_set_compare_value(comp_obrir, angle_to_compare(80));
                                vTaskDelay(pdMS_TO_TICKS(3000));
                                bloqueig_per_zero = true;
                        }

                        break;

                case 202:
                        if (bloqueig_per_zero == false && posicio_horitzontal == false)
                        {
                                posicio_horitzontal = true;
                                printf("Posició horitzontal\n");
                                mcpwm_comparator_set_compare_value(comp_rotacio, angle_to_compare(97));
                                vTaskDelay(pdMS_TO_TICKS(3000));
                                bloqueig_per_zero = true;
                        }
                        else if (bloqueig_per_zero == false && posicio_horitzontal == true)
                        {
                                posicio_horitzontal = false;
                                printf("Posició vertical\n");
                                mcpwm_comparator_set_compare_value(comp_rotacio, angle_to_compare(7));
                                vTaskDelay(pdMS_TO_TICKS(3000));
                                bloqueig_per_zero = true;
                        }

                default:
                        break;
                }
        }
}
