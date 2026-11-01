#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_PORT GPIOF
#define LED1_PIN GPIO_PIN_11   // HARDWARE-VERIFY: on-board LED
#define LED2_PIN GPIO_PIN_12   // HARDWARE-VERIFY: on-board LED

extern "C" void xPortSysTickHandler(void);

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

extern "C" void vApplicationIdleHook(void) {}
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) { while (1) {} }
extern "C" void vApplicationMallocFailedHook(void) { while (1) {} }

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED1_PIN | LED2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

void Task1_Blink(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void Task2_Blink(void *pvParameters) {
    while (1) {
        HAL_GPIO_TogglePin(LED_PORT, LED2_PIN);
        vTaskDelay(pdMS_TO_TICKS(333));
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    SystemCoreClockUpdate();

    xTaskCreate(Task1_Blink, "LED1_Task", 128, NULL, 1, NULL);
    xTaskCreate(Task2_Blink, "LED2_Task", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1) {}
}