#include "stm32f4xx_hal.h"
#include <stdio.h>

// --- CONFIGURATION & HARDWARE ASSUMPTIONS ---
// HARDWARE-VERIFY: Set to 0 if Common Anode (active LOW), or 1 if Common Cathode (active HIGH)
#define LED_ACTIVE_LEVEL 0 

#define LED_ON(port, pin)  HAL_GPIO_WritePin(port, pin, (GPIO_PinState)LED_ACTIVE_LEVEL)
#define LED_OFF(port, pin) HAL_GPIO_WritePin(port, pin, (GPIO_PinState)!LED_ACTIVE_LEVEL)

// --- PERIPHERAL HANDLES ---
UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c2;

// --- INTERRUPT HANDLERS ---
extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
}

// --- PRINTF RETARGET ---
extern "C" int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

// --- SELF-CONTAINED SAFE-FAIL ERROR HANDLER ---
void Error_Handler_Visual(const char *error_msg) {
    printf("\r\n[CRITICAL ERROR]: %s\r\n", error_msg);
    
    // Self-contained GPIO setup for Red LED on PF13 (does not rely on MX_GPIO_Init)
    __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    while (1) {
        // Fast blink RED LED to signal hardware failure while respecting LED_ACTIVE_LEVEL
        LED_ON(GPIOF, GPIO_PIN_13);
        HAL_Delay(100);
        LED_OFF(GPIOF, GPIO_PIN_13);
        HAL_Delay(100);
    }
}

// --- SYSTEM CLOCK CONFIGURATION (168 MHz) ---
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // HARDWARE-VERIFY: Assumes 8 MHz external crystal (HSE).
    // If board uses 25 MHz, set PLLM = 25 and HSE_VALUE = 25000000U in platformio.ini.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler_Visual("HSE Oscillator Configuration Failed");
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler_Visual("System Clock Configuration Failed");
    }
}

// --- UART1 INITIALIZATION & MSP ---
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
        Error_Handler_Visual("USART1 Peripheral Init Failed");
    }
}

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (uartHandle->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        // HARDWARE-VERIFY: PA9 = USART1_TX, PA10 = USART1_RX (AF7)
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

// --- GPIO INITIALIZATION ---
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // HARDWARE-VERIFY: RGB LEDs on PF13 (Red), PF14 (Green), PF15 (Blue)
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    LED_OFF(GPIOF, GPIO_PIN_13);
    LED_OFF(GPIOF, GPIO_PIN_14);
    LED_OFF(GPIOF, GPIO_PIN_15);

    // HARDWARE-VERIFY: USER_BUTTON on PC0 (active-LOW with pull-up)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // HARDWARE-VERIFY: External Buttons 1 to 4 on PE2, PE3, PE4, PE5 (active-LOW with pull-up)
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

// --- ADC1 INITIALIZATION & MSP ---
void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

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
        Error_Handler_Visual("ADC1 Initialization Failed");
    }

    // HARDWARE-VERIFY: PA3 maps to ADC1_CHANNEL_3
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler_Visual("ADC1 Channel Configuration Failed");
    }
}

extern "C" void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (adcHandle->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // HARDWARE-VERIFY: PA3 set as Analog input
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

// --- I2C2 INITIALIZATION & MSP ---
void MX_I2C2_Init(void) {
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 100000; // 100 kHz Standard Mode
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
        Error_Handler_Visual("I2C2 Initialization Failed");
    }
}

extern "C" void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (i2cHandle->Instance == I2C2) {
        __HAL_RCC_I2C2_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();

        // HARDWARE-VERIFY: PF0 = I2C2_SDA, PF1 = I2C2_SCL (AF4)
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; // Open-drain for I2C
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    }
}

// --- READ POTENTIOMETER WITH DEAD-ZONE COMPENSATION ---
uint32_t Read_Volume_Percentage(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t raw_adc = HAL_ADC_GetValue(&hadc1);
        
        // 100 Ohm ground resistor creates a ~40 count offset at the bottom
        const uint32_t ADC_MIN_OFFSET = 40;
        const uint32_t ADC_MAX = 4095;

        if (raw_adc <= ADC_MIN_OFFSET) {
            return 0; // True 0% volume in dead zone
        }

        // Scale [40..4095] linearly to [0..100]%
        uint32_t volume_percent = ((raw_adc - ADC_MIN_OFFSET) * 100) / (ADC_MAX - ADC_MIN_OFFSET);
        return (volume_percent > 100) ? 100 : volume_percent;
    }
    return 0;
}

// --- I2C BUS SCANNER ---
void I2C2_Scan_Bus(void) {
    printf("\r\n--- Scanning I2C2 Bus (PF0/PF1) ---\r\n");
    uint8_t devices_found = 0;

    for (uint8_t addr = 1; addr < 128; addr++) {
        // Shift 7-bit address left by 1 for STM32 HAL write address format
        uint16_t hal_addr = (uint16_t)(addr << 1);
        
        if (HAL_I2C_IsDeviceReady(&hi2c2, hal_addr, 2, 100) == HAL_OK) {
            printf("  [FOUND] I2C Device at 8-bit Write Addr: 0x%02X (7-bit Addr: 0x%02X)\r\n", 
                   hal_addr, addr);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        printf("  [WARNING]: No I2C devices responded! Check PF0/PF1 pull-ups and power.\r\n");
    } else {
        printf("--- Scan Complete: %d device(s) found ---\r\n\r\n", devices_found);
    }
}

// --- MAIN APPLICATION ---
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_I2C2_Init();

    printf("\r\n==================================================\r\n");
    printf("--- Stage 3c Initialized: I2C2 Bus Scanner ---\r\n");
    printf("==================================================\r\n");

    // Perform an initial scan at boot
    I2C2_Scan_Bus();

    uint32_t last_print_time = 0;

    while (1) {
        if (HAL_GetTick() - last_print_time >= 200) {
            last_print_time = HAL_GetTick();

            uint8_t user_btn = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0) == GPIO_PIN_RESET) ? 1 : 0;
            uint8_t btn1     = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2) == GPIO_PIN_RESET) ? 1 : 0;
            uint8_t btn2     = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_3) == GPIO_PIN_RESET) ? 1 : 0;
            uint8_t btn3     = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_RESET) ? 1 : 0;
            uint8_t btn4     = (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_5) == GPIO_PIN_RESET) ? 1 : 0;

            uint8_t song_index = (btn4 << 2) | (btn3 << 1) | btn2;
            uint32_t volume = Read_Volume_Percentage();

            printf("USER: %d | BTN1: %d | Song Bits: [%d%d%d] -> Index: %d | Volume: %3lu%%\r\n",
                   user_btn, btn1, btn4, btn3, btn2, song_index, volume);

            // Re-run I2C scan if USER_BUTTON is pressed
            if (user_btn) {
                I2C2_Scan_Bus();
            }
        }
    }
}