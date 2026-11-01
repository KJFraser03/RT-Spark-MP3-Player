#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <math.h>
#include "NHD_0216HZ.h"

// =========================================================================
// CONFIGURABLE I2S AND I2C PINS (HARDWARE-VERIFY)
// =========================================================================
// I2S3 Peripheral Mapping
#define I2S3_MCK_PORT       GPIOC
#define I2S3_MCK_PIN        GPIO_PIN_7      // HARDWARE-VERIFY: I2S3_MCK Pin
#define I2S3_CK_PORT        GPIOB
#define I2S3_CK_PIN         GPIO_PIN_3      // HARDWARE-VERIFY: I2S3_CK Pin
#define I2S3_WS_PORT        GPIOA
#define I2S3_WS_PIN         GPIO_PIN_4      // HARDWARE-VERIFY: I2S3_WS Pin (Alt: PA15)
#define I2S3_SD_PORT        GPIOB
#define I2S3_SD_PIN         GPIO_PIN_5      // HARDWARE-VERIFY: I2S3_SD Pin

// ES8388 Control Interface (Software I2C)
#define ES8388_I2C_SCL_PORT GPIOB
#define ES8388_I2C_SCL_PIN  GPIO_PIN_6      // HARDWARE-VERIFY: ES8388 SCL Pin
#define ES8388_I2C_SDA_PORT GPIOB
#define ES8388_I2C_SDA_PIN  GPIO_PIN_7      // HARDWARE-VERIFY: ES8388 SDA Pin
#define ES8388_I2C_ADDR     0x20            // HARDWARE-VERIFY: 8-bit write address (0x10 7-bit << 1)

// =========================================================================
// ES8388 DATASHEET REGISTER ADDRESSES
// =========================================================================
#define ES8388_CONTROL1        0x00
#define ES8388_CONTROL2        0x01
#define ES8388_CHIPPOWER       0x02
#define ES8388_ADCPOWER        0x03
#define ES8388_DACPOWER        0x04
#define ES8388_MASTERMODE      0x08
#define ES8388_DACCONTROL1     0x17
#define ES8388_DACCONTROL2     0x18
#define ES8388_DACCONTROL23    0x26
#define ES8388_DACCONTROL24    0x27 // DAC Mixer routing
#define ES8388_DACCONTROL27    0x2A // Output Mux setting
#define ES8388_DACCONTROL28    0x2B // LOUT1/ROUT1 Volume

#define LED_ACTIVE_LEVEL 0 
#define LED_ON(port, pin)  HAL_GPIO_WritePin(port, pin, (GPIO_PinState)LED_ACTIVE_LEVEL)
#define LED_OFF(port, pin) HAL_GPIO_WritePin(port, pin, (GPIO_PinState)!LED_ACTIVE_LEVEL)

#define TONE_FREQ           440
#define SAMPLE_RATE         48000
#define SINE_SAMPLES        (SAMPLE_RATE / TONE_FREQ) // 109 samples
#define AUDIO_BUF_SIZE      (SINE_SAMPLES * 2)       // 218 int16 samples (Stereo)

UART_HandleTypeDef huart1;
ADC_HandleTypeDef hadc1;
SRAM_HandleTypeDef hsram1; 
I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_i2s3_tx;
NHD_0216HZ my_lcd;

int16_t audio_buffer[AUDIO_BUF_SIZE];

extern "C" void SysTick_Handler(void) { HAL_IncTick(); }

extern "C" void DMA1_Stream5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_i2s3_tx);
}

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

// Software I2C Driver for ES8388 Control
static void I2C_Delay(void) {
    for (volatile int i = 0; i < 50; i++);
}

static void I2C_Start(void) {
    HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_RESET);
    I2C_Delay();
    HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_RESET);
    I2C_Delay();
}

static void I2C_Stop(void) {
    HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_SET);
    I2C_Delay();
}

static uint8_t I2C_WriteByte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80)
            HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_RESET);
        byte <<= 1;
        I2C_Delay();
        HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_SET);
        I2C_Delay();
        HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_RESET);
        I2C_Delay();
    }
    HAL_GPIO_WritePin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    uint8_t ack = (HAL_GPIO_ReadPin(ES8388_I2C_SDA_PORT, ES8388_I2C_SDA_PIN) == GPIO_PIN_RESET) ? 1 : 0;
    HAL_GPIO_WritePin(ES8388_I2C_SCL_PORT, ES8388_I2C_SCL_PIN, GPIO_PIN_RESET);
    I2C_Delay();
    return ack;
}

void ES8388_WriteReg(uint8_t reg, uint8_t val) {
    I2C_Start();
    I2C_WriteByte(ES8388_I2C_ADDR);
    I2C_WriteByte(reg);
    I2C_WriteByte(val);
    I2C_Stop();
}

void ES8388_Init(void) {
    // HARDWARE-VERIFY: Register configuration sequence matching RT-Spark audio codec
    ES8388_WriteReg(ES8388_CONTROL1, 0x06); // VMID selection and MCLK divider
    ES8388_WriteReg(ES8388_CONTROL2, 0x50); // Power up analog and clocks
    ES8388_WriteReg(ES8388_CHIPPOWER, 0x00); // Power up chip
    ES8388_WriteReg(ES8388_ADCPOWER, 0xFF);  // Disable ADC (DAC output test only)
    ES8388_WriteReg(ES8388_DACPOWER, 0x3C);  // Power up LDAC, RDAC, LOUT1, ROUT1
    ES8388_WriteReg(ES8388_MASTERMODE, 0x00);// ES8388 in Slave Mode

    ES8388_WriteReg(ES8388_DACCONTROL1, 0x18); // 16-Bit I2S Format
    ES8388_WriteReg(ES8388_DACCONTROL2, 0x02); // SCLK/LRCK ratio
    ES8388_WriteReg(ES8388_DACCONTROL23, 0x00);

    // Route DAC straight to mixer (0x90 = LDAC to Left Mixer, RDAC to Right Mixer, Line-In Disabled)
    ES8388_WriteReg(ES8388_DACCONTROL24, 0x90); 

    // Route Left/Right Mixer outputs to LOUT1/ROUT1 (0x90 = LOUT1/ROUT1 active, Mixer source)
    ES8388_WriteReg(ES8388_DACCONTROL27, 0x90); 

    // Set LOUT1/ROUT1 Volume (0x00 = 0dB gain, unattenuated)
    ES8388_WriteReg(ES8388_DACCONTROL28, 0x00); 
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

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

    // Configure PLLI2S for 48 kHz sampling rate (Audio Clock = 96 MHz)
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
    PeriphClkInitStruct.PLLI2S.PLLI2SR = 2; // HARDWARE-VERIFY: PLLI2SR divisor
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler_Visual("PLLI2S Config Failed");
    }
}

void MX_DMA_Init(void) {
    __HAL_RCC_DMA1_CLK_ENABLE();

    // Configure DMA1 Stream5 Channel 0 for SPI3_TX (I2S3_TX)
    hdma_i2s3_tx.Instance = DMA1_Stream5;     // HARDWARE-VERIFY: Stream assignment
    hdma_i2s3_tx.Init.Channel = DMA_CHANNEL_0;// HARDWARE-VERIFY: Channel assignment
    hdma_i2s3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_i2s3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2s3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2s3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_i2s3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_i2s3_tx.Init.Mode = DMA_CIRCULAR;
    hdma_i2s3_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_i2s3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_i2s3_tx) != HAL_OK) {
        Error_Handler_Visual("I2S DMA Init Failed");
    }

    __HAL_LINKDMA(&hi2s3, hdmatx, hdma_i2s3_tx);

    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

void MX_I2S3_Init(void) {
    hi2s3.Instance = SPI3;
    hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_48K;
    hi2s3.Init.CPOL = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource = I2S_CLOCK_PLL;

    if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
        Error_Handler_Visual("I2S3 Init Failed");
    }
}

extern "C" void HAL_I2S_MspInit(I2S_HandleTypeDef* i2sHandle) {
    if (i2sHandle->Instance == SPI3) {
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStruct = {0};

        // Configure MCK (PC7)
        GPIO_InitStruct.Pin = I2S3_MCK_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
        HAL_GPIO_Init(I2S3_MCK_PORT, &GPIO_InitStruct);

        // Configure CK (PB3) and SD (PB5)
        GPIO_InitStruct.Pin = I2S3_CK_PIN | I2S3_SD_PIN;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        // Configure WS (PA4 or PA15)
        GPIO_InitStruct.Pin = I2S3_WS_PIN;
        HAL_GPIO_Init(I2S3_WS_PORT, &GPIO_InitStruct);
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
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Soft-I2C Pins for ES8388
    GPIO_InitStruct.Pin = ES8388_I2C_SCL_PIN | ES8388_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ES8388_I2C_SCL_PORT, &GPIO_InitStruct);

    // LEDs and Backlight
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    LED_OFF(GPIOF, GPIO_PIN_13); LED_OFF(GPIOF, GPIO_PIN_14); LED_OFF(GPIOF, GPIO_PIN_15);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    // LCD Reset
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Buttons
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void Generate_440Hz_SineWave(void) {
    for (int i = 0; i < SINE_SAMPLES; i++) {
        float angle = (2.0f * 3.14159265f * (float)i) / (float)SINE_SAMPLES;
        int16_t sample = (int16_t)(10000.0f * sinf(angle));
        audio_buffer[2 * i]     = sample; // Left Channel
        audio_buffer[2 * i + 1] = sample; // Right Channel
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_FSMC_Init();
    MX_DMA_Init();
    MX_I2S3_Init();

    printf("\r\n=== Stage 3e: Audio (ES8388 + I2S DMA Tone Test) ===\r\n");

    my_lcd.init_lcd();
    my_lcd.set_cursor(0, 0);
    my_lcd.printf("Audio Tone Test");
    my_lcd.set_cursor(0, 1);
    my_lcd.printf("Playing 440 Hz...");

    // Initialize ES8388 Codec via Software I2C
    ES8388_Init();

    // Populate lookup buffer with 440 Hz sine wave @ 48 kHz
    Generate_440Hz_SineWave();

    // Start Circular DMA Audio Transmission over I2S
    if (HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)audio_buffer, AUDIO_BUF_SIZE) != HAL_OK) {
        Error_Handler_Visual("I2S DMA Transmit Start Failed");
    }

    while (1) {
        HAL_Delay(500);
        HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_14); // Heartbeat LED
    }
}