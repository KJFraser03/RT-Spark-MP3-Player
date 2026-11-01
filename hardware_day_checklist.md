3a. Silence Diagnostic Checklist
If the LED and ADC work but the 3.5mm jack outputs silence, verify these constraints before we resort to the PWM fallback:

I2C ACK: Did the scanner print Found I2C device at 0x40 (or 0x44)? If not, the codec isn't receiving commands, meaning the PF0/PF1 pins are wrong or unpowered.

MCLK Present: The ES8388 internal DAC strictly requires the Master Clock (PC7). If PC7 is not connected to the codec's MCLK pin, it will silently ignore I2S data.

I2S Pinout: Are you absolutely certain the F407's PA4, PC10, and PC12 route to the codec's WS, SCLK, and SDIN pins respectively? A mismatch here sends the 440Hz data into the void.


Running Hardware Day ChecklistFollow this checklist in exact order when you obtain physical access to the RT-Spark board:StepAction / TestExpected ResultCause & Fix if Test Fails1Check Crystal MarkingInspect silver crystal near MCU. Identify if 8.000 or 25.000 MHz.If 25 MHz, update HSE_VALUE in platformio.ini and PLLM = 25 in SystemClock_Config().2Console UART OutputOpen Serial Monitor at 115200 baud. Must see --- Stage 3b Initialized... --- banner cleanly.Garbled text: Wrong crystal HSE_VALUE or wrong baud rate.No text: PA9/PA10 wrong pins or TX/RX swapped on USB-TTL adapter.3Identify RGB PolarityDuring boot, observe RGB sequence.Always ON / White: Common Cathode board. Change #define LED_ACTIVE_LEVEL 1.4USER_BUTTON TestPress USER_BUTTON (PC0).Serial prints USER_BTN: 1 when pressed, 0 when released. If reversed, pull-up logic or pin mapping is wrong.5External Buttons 1–4 TestPress Buttons 1 to 4 individually.BTN1 lights GREEN LED.BTN2-4 change Song Bits and Index ($0$ to $7$). If no response, check header wiring on PE2–PE5.6Potentiometer ADC TestRotate 10k Potentiometer from end to end.POT ADC smoothly sweeps from 0 to 4095. If static 0 or 4095, verify PA3 connection and 3.3V/GND rails.



Updated Hardware Day ChecklistStepAction / TestExpected ResultCause & Fix if Test Fails1Check Crystal MarkingInspect silver crystal package near MCU (8.000 or 25.000 MHz).If 25 MHz, update HSE_VALUE = 25000000U and PLLM = 25.2Console UART OutputOpen Serial Monitor at 115200 baud.Must print --- Stage 3c Initialized... --- banner cleanly.3Identify RGB PolarityBoot board and observe RGB status.If Red LED stays lit constantly, set #define LED_ACTIVE_LEVEL 1.4Potentiometer CalibrationRotate pot to minimum position.Serial monitor must display Volume: 0% (compensating for the 100 $\Omega$ resistor offset). Max position reads 100%.5I2C Bus ScanObserve automatic boot scan or press USER_BUTTON.Must list ES8388 at 0x20 (or 0x22) representing 7-bit address 0x10 (or 0x11). Other addresses belong to on-board sensors. If 0 devices found, check PF0/PF1 connections and power.



Hardware Day Checklist (Stage 3c)
Verify Crystal / HSE: Confirm the onboard crystal frequency (8MHz vs 25MHz) and update HSE_VALUE / PLLM if it is not 8MHz.

UART Boot Output: Open terminal at 115200 baud. You should see === Stage 3c: I2C Scanner & Potentiometer === cleanly. If garbled, the clock setup/HSE is wrong.

Safe-Fail Check: If the Red LED blinks rapidly on boot and the terminal says [FATAL ERROR], an initialization routine failed.

Identify I2S Pins: Cross-reference the net labels on the schematic for I2S3_MCK, I2S3_CK, I2S3_WS, and I2S3_SD to their MCU pins. Note these down for Stage 3e.

Potentiometer Dead Zone: Turn the knob fully counter-clockwise. The console should reliably read Vol:   0%. Turn it fully clockwise, it should read Vol: 100%.

I2C Bus Scan: On boot (or when pressing the USER_BUTTON), the terminal should list detected I2C devices. You expect to see the ES8388 (typically at 7-bit 0x10 or 0x11) and any other onboard sensors. If it says "No devices found", verify the PF0/PF1 assignment and check for missing pull-up resistors or an unpowered codec.


Hardware Day Checklist (Stage 3d)StepAction / TestExpected ResultCause & Fix if Test Fails1Check I2S NetsOpen schematic and trace Codec pins 1,2,3,4.Provide MCU pin names to me for Stage 3e.2ADC Average TestRotate Potentiometer.Smoothly hits 0% at bottom (deadzone $\le 60$) and 100% at top ($\ge 4040$). Much less flicker due to 8x averaging and 84-cycle sample time.3Debounce TestRapidly press Buttons 2-4.The UART index and LCD index should update solidly without bouncing rapidly between numbers.4LCD Init & RenderBoot the board.Screen turns on, backlight activates (via PF9), displays "MP3 Player Boot", then updates with Song Index and Volume.5LCD Blank/White?Check LCD failure modes.If white: Backlight PF9 works but ST7789 init failed (wrong FSMC bank NE3, or A18 mismatch). If black: Backlight pin PF9 is wrong.


Hardware Day Checklist (Stage 3d)
Step	Action / Test	Expected Result	Cause & Fix if Test Fails
5	LCD Blank/White?	Boot the board.	If white: ST7789 failed to initialize. Check if RS is on a different pin than A18 (PD13). Verify LCD_DATA pointer address. If black: PF9 backlight control is failing.
6	Text Overwrite Test	Turn potentiometer.	The text should update cleanly over the old text without flashing black. If it flashes or leaves garbage, the draw_char background loop or fixed-width padding spaces (" ") are failing.
7	Button Debounce Test	Rapidly and lightly tap Buttons 2-4.	The UART and LCD readouts should step cleanly to the new index without rapidly skipping back and forth. If it skips, increase the 20ms debounce threshold.
8	ADC Averaging Test	Slowly turn potentiometer.	Volume should rest stably on a single percentage point. If it jitters rapidly between two values while stationary, ADC_SAMPLETIME_84CYCLES and the 8-sample average are not enough to filter electrical noise.



Stage 3d Checklist
LCD Init Test: Boot the board. The screen should immediately clear to black and display text. If the text is mirrored or inverted, the MADCTL orientation needs adjustment.

Window Draw Test: Turn the potentiometer. The digits should update instantly without the whole screen flashing. Ensure padded trailing digits (e.g., when dropping from 100% to 99%) overwrite cleanly due to the "   " spaces.

A18 Verification: If the screen remains pure white, verify on the RT-Spark schematic if the LCD RS/DC pin is wired to PD13 (A18). If it is on a different pin, the bit shift in LCD_DATA needs changing.


## Open questions (verify before or on hardware day)

### Clock and console
- [ ] Crystal frequency: read the marking on the crystal near the MCU (8.000 = 8 MHz). If it is not 8 MHz, change PLLM in SystemClock_Config.
- [ ] HSE_VALUE is 8000000 (search the project for HSE_VALUE; set it in stm32f4xx_hal_conf.h or build_flags).
- [ ] Console UART is USART1 on PA9 (TX) / PA10 (RX), and it comes out through the ST-Link virtual COM port. A COM port appears in Device Manager.

### Pins (check each against the RT-Spark schematic or the RT-Thread BSP README)
- [ ] USER_BUTTON = PC0 (KEY_LEFT), active low
- [ ] External Buttons 1-4 = PE2, PE3, PE4, PE5: broken out on the 40-pin header and not used by anything else
- [ ] Potentiometer wiper = PA3 (ADC1_IN3): on the header and free
- [ ] RGB LED = PF13 (R), PF14 (G), PF15 (B): on the header and free (PF11/PF12 are the board LEDs)
- [ ] Avoid PG2/PG4/PG6 (another RT-Spark project uses them for RGB)

### RGB LED
- [ ] Polarity: common anode or common cathode? (multimeter diode test, or run the LED test and flip LED_ACTIVE_LEVEL)
- [ ] 330 ohm resistor on each color leg; common pin to 3.3 V (anode) or GND (cathode), NOT 5 V

### I2C / codec
- [ ] I2C2 on PF0 (SDA) / PF1 (SCL), shared with the on-board sensors
- [ ] ES8388 address: expect 7-bit 0x10 or 0x11. Run the I2C scan and write down every address found.
- [ ] Does the headphone jack need a separate amp or enable GPIO? (not found; check the schematic)

### I2S audio pins (still unconfirmed)
- [ ] I2S3_MCK = ? (default guess PC7)
- [ ] I2S3_CK = ? (guess PB3)
- [ ] I2S3_WS = ? (guess PA4 or PA15)
- [ ] I2S3_SD = ? (guess PB5)
- [ ] Do NOT use PC10/PC12 (SDIO pins, the SD slot)
- [ ] Audio decision gate: if there is no sound after the troubleshooting steps, use the fallback (buzzer or GPIO PWM tone)

### LCD (ST7789 over FSMC)
- [ ] Data register address: Gemini used 0x68000000 | (1 << 18) = 0x68040000. Another RT-Spark project uses 0x68000000 for commands and 0x68000000 + 0x3FFFE for data. Check the RT-Thread RT-Spark BSP LCD driver (drv_lcd.c) to see which is right.
- [ ] RS (data/command) is on A18 = PD13
- [ ] LCD reset = PD3, backlight = PF9
- [ ] FSMC chip select = NE3 = PG10
- [ ] ST7789 init sequence matches the BSP's full sequence
- [ ] Screen orientation (MADCTL) and colors look right
- [ ] Font text is not mirrored. In draw_char the pixel test must be (1 << j) for font8x8_basic (LSB-first), not 0x80 >> j.

### Analog / input behavior
- [ ] ADC minimum is about 40 (the 100 ohm resistor), so volume 0% is only reached through the dead zone (about 60)
- [ ] Button debounce feels right at 20 ms

### Lab document
- [ ] The doc says 50 points (the title page); I originally assumed 100. Confirm with the instructor.
- [ ] "5 external push buttons" in the text vs 4 buttons plus USER_BUTTON in the hardware list. Confirm which physical buttons are Button 1-4.
- [ ] Is ES8388 (3.5 mm jack) required for audio, or is the 100 ohm / buzzer fallback acceptable?

Confirm a clean tone through the headphone jack; if silent, check the I2S pins, then codec I2C ACK, then the register values (DAC power, mute, mixer route)."

Stage 3e Hardware-Day ChecklistI2C Verification: Probe PF0/PF1 (or check UART error logs). Hardware I2C2 relies on physical pull-up resistors on the board; if they are missing or if the address (0x20) is incorrect, initialization will fail.Audio Trace Pinout: Validate the schematic against the HARDWARE-VERIFY pin declarations at the top of main.cpp before running the DMA stream.Sine Output Check: Connect headphones. The tone should be a clear, continuous $\sim440\text{ Hz}$ sine wave without popping. 


Stage 6a Pre-Flight Checklist
Directory Structure: Double-check that heap_4.c and port.c are physically present in the paths PlatformIO is checking. Missing these will cause linker errors for pvPortMalloc or vPortSVCHandler.

Compile Step: Build the environment. Because there's no lib_deps auto-magic here, any failure is likely a missing include path in platformio.ini or a duplicated SysTick_Handler if stm32f4xx_it.c is left in the tree from CubeMX.

Awaiting Confirmation: Let me know if this builds cleanly, and I will generate the complete Phase 6 MP3 logic implementing song_def.h, the UI threads, mutexes, and timers exactly as specified.

        