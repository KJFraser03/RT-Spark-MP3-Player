#include "stm32f4xx_hal.h"
#include <stdio.h>
#include "NHD_0216HZ.h"

#define LED_ACTIVE_LEVEL 0 
#define LED_ON(port, pin)  HAL_GPIO_WritePin(port, pin, (GPIO_PinState)LED_ACTIVE_LEVEL)
#define LED_OFF(port, pin) HAL_GPIO_WritePin(port, pin, (GPIO_PinState)!LED_ACTIVE_LEVEL)

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
SRAM_HandleTypeDef hsram1; 
NHD_0216HZ my_lcd;

extern "C" void SysTick_Handler(void) { HAL_IncTick(); }

extern "C" int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void Error_Handler_Visual(const char *msg) {
    printf("\r\n[FATAL ERROR] %s\r\n", msg);
    __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    while (1) {
        LED_ON(GPIOF, GPIO_PIN_13); HAL_Delay(100);
        LED_OFF(GPIOF, GPIO_PIN_13); HAL_Delay(100);
    }
}

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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler_Visual("HSE Config Failed");
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler_Visual("SysClock Config Failed");
    }
}

void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler_Visual("UART1 Init Failed");
    }
}

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
    if (uartHandle->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void MX_ADC1_Init(void) {
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler_Visual("ADC Init Failed");
    }

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler_Visual("ADC Channel Failed");
    }
}

extern "C" void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle) {
    if (adcHandle->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void MX_FSMC_Init(void) {
    FSMC_NORSRAM_TimingTypeDef Timing = {0};
    hsram1.Instance = FSMC_NORSRAM_DEVICE;
    hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    
    hsram1.Init.NSBank = FSMC_NORSRAM_BANK3;
    hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_8;
    hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;

    Timing.AddressSetupTime = 5;
    Timing.AddressHoldTime = 1;
    Timing.DataSetupTime = 9;
    Timing.BusTurnAroundDuration = 1;
    Timing.CLKDivision = 2;
    Timing.DataLatency = 2;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;

    if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK) {
        Error_Handler_Visual("FSMC Init Failed");
    }
}

extern "C" void HAL_SRAM_MspInit(SRAM_HandleTypeDef* sramHandle) {
    if (sramHandle->Instance == FSMC_NORSRAM_DEVICE) {
        __HAL_RCC_FSMC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    }
}

void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    LED_OFF(GPIOF, GPIO_PIN_13); LED_OFF(GPIOF, GPIO_PIN_14); LED_OFF(GPIOF, GPIO_PIN_15);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

uint8_t Get_Volume_Percent(void) {
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            sum += HAL_ADC_GetValue(&hadc1);
        }
    }
    uint32_t val = sum / 8;
    if (val <= 60) return 0;
    if (val >= 4040) return 100;
    return ((val - 60) * 100) / (4040 - 60);
}

typedef struct {
    uint8_t state;
    uint32_t last_time;
} DebouncedBtn_t;

uint8_t Debounce_Button(GPIO_TypeDef* port, uint16_t pin, DebouncedBtn_t *btn) {
    uint8_t raw = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1 : 0;
    if (raw != btn->state) {
        if (HAL_GetTick() - btn->last_time > 20) {
            btn->state = raw;
            btn->last_time = HAL_GetTick();
        }
    } else {
        btn->last_time = HAL_GetTick();
    }
    return btn->state;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_FSMC_Init();

    printf("\r\n=== Stage 3d: ST7789 FSMC LCD & Debounced Inputs ===\r\n");

    my_lcd.init_lcd();
    my_lcd.set_cursor(0, 0);
    my_lcd.printf("MP3 Player Boot");
    my_lcd.set_cursor(0, 1);
    my_lcd.printf("Testing LCD...");

    DebouncedBtn_t btn2_db = {0, 0}, btn3_db = {0, 0}, btn4_db = {0, 0};
    uint32_t last_print = 0;
    
    // Tracking state prevents constant redrawing
    uint8_t last_idx = 255; 
    uint8_t last_vol = 255; 

    while (1) {
        uint8_t b2 = Debounce_Button(GPIOE, GPIO_PIN_3, &btn2_db);
        uint8_t b3 = Debounce_Button(GPIOE, GPIO_PIN_4, &btn3_db);
        uint8_t b4 = Debounce_Button(GPIOE, GPIO_PIN_5, &btn4_db);

        if (HAL_GetTick() - last_print >= 100) {
            last_print = HAL_GetTick();
            uint8_t song_idx = (b4 << 2) | (b3 << 1) | b2;
            uint8_t vol = Get_Volume_Percent();

            // Only redraw if a value changed
            if (song_idx != last_idx || vol != last_vol) {
                printf("Index: %d | Vol: %3d%%\r\n", song_idx, vol);
                
                my_lcd.set_cursor(0, 0);
                my_lcd.printf("Song Index: %d   ", song_idx);
                my_lcd.set_cursor(0, 1);
                my_lcd.printf("Volume: %3d%%   ", vol); // Padding spaces erase trailing artifacts
                
                last_idx = song_idx;
                last_vol = vol;
            }
        }
    }
}