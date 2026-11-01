    #include "stm32f4xx_hal.h"

/* Needed so HAL_Delay() works */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

int main(void)
{
  HAL_Init();

  __HAL_RCC_GPIOF_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin   = GPIO_PIN_11;          /* on-board LED (unverified) */
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &gpio);

  while (1)
  {
    HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_11);
    HAL_Delay(500);
  }
}