/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "MPU6050.h"
#include "state_control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 29 //timespand: 4, 2 MPU: 2*6*2, posture code: 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim22;

/* USER CODE BEGIN PV */
//TODO: định nghĩa thêm các file cho các state (nên ghi ra state làm cái gì rồi code service)
//state INIT: init SD, init MPU, init tim22, init_state, memset buffer, LED_start OFF
//state START: Bật LED start
//state IDLE: đọc SW3 (đầu while rồi), chờ dataflag (Interrupt callback), đọc SW[2..0]
//state DATA: clear data_flag, đọc 2 MPU, packetnization
//state SD_write: ghi dữ liệu xuống file POSTURE.bin, packet count ++, nếu packet count >= 50 thì f_sync và packet_count = 0;
//state STOP: tắt LED_start, thực hiện close_file
system_state_t system_state;
system_input_t system_input;
MPU6050_Data_t mpu_low_data;
MPU6050_Data_t mpu_high_data;
uint8_t stop_done = 0;
uint8_t posture_code;
uint8_t sd_buffer[BUFFER_SIZE];
FATFS FatFs;
FIL fil;
UINT bytes_written = 0;
uint8_t packet_count = 0;
//STATE defined function
void system_init();
void start();
void idle();
void data();
void write_sd();
void stop();
//Service function
void reset_buffer(
    uint8_t *buffer,
    uint16_t buffer_size
);
void TIM22_Init_IT(void);
uint8_t init_sd(void);
uint8_t MPU_init(
    I2C_HandleTypeDef *hi2c
);
void read_2_MPU(
    I2C_HandleTypeDef *hi2c,
    MPU6050_Data_t *mpu_low,
    MPU6050_Data_t *mpu_high
);
void packetnization(
    uint8_t *buffer,
    MPU6050_Data_t *mpu_low,
    MPU6050_Data_t *mpu_high,
    uint8_t posture
);
uint8_t read_SW3();
uint8_t read_posture_sw(void);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM22_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM22_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  system_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	 system_input.SW3 = read_SW3();
	 change_state(&system_state, &system_input);
	 switch(system_state)
	 	 {
	 	 	 case STATE_INIT:
	 	 		 break;
	 	 	 case STATE_START:
	 	 		 start();
	 	 		 break;
	 	 	 case STATE_IDLE:
	 	 		 idle();
	 	 		 break;
	 	 	 case STATE_DATA:
	 	 		 data();
	 	 		 break;
	 	 	 case STATE_WRITE_SD:
	 	 		 write_sd();
	 	 		 break;
	 	 	 case STATE_STOP:
	 	 		 stop();
	 	 		 break;
	 	 	 default: break;
	 	 }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM22 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM22_Init(void)
{

  /* USER CODE BEGIN TIM22_Init 0 */

  /* USER CODE END TIM22_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM22_Init 1 */

  /* USER CODE END TIM22_Init 1 */
  htim22.Instance = TIM22;
  htim22.Init.Prescaler = 3199;
  htim22.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim22.Init.Period = 999;
  htim22.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim22.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim22) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim22, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim22, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM22_Init 2 */

  /* USER CODE END TIM22_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_start_GPIO_Port, LED_start_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SW3_Pin SW2_Pin */
  GPIO_InitStruct.Pin = SW3_Pin|SW2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_start_Pin */
  GPIO_InitStruct.Pin = LED_start_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LED_start_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SW1_Pin SW0_Pin */
  GPIO_InitStruct.Pin = SW1_Pin|SW0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void system_init()
{
	init_sd();
	MPU_init(&hi2c1);
	TIM22_Init_IT();
	init_state(&system_state, &system_input);
	reset_buffer(sd_buffer, BUFFER_SIZE);
//	HAL_GPIO_WritePin(
//	    LED_start_GPIO_Port,
//	    LED_start_Pin,
//	    GPIO_PIN_RESET
//	);
	stop_done = 0;
}
void start()
{
	HAL_GPIO_WritePin(
		    LED_start_GPIO_Port,
		    LED_start_Pin,
		    GPIO_PIN_SET
		);
}
void idle()
{
	posture_code = read_posture_sw();
}
void data()
{
	if(system_input.data_flag)
		system_input.data_flag = 0;
	read_2_MPU(
	    &hi2c1,
	    &mpu_low_data,
	    &mpu_high_data
	);
	packetnization(
	    sd_buffer,
	    &mpu_low_data,
	    &mpu_high_data,
	    posture_code
	);
}
void write_sd()
{
    f_write(
        &fil,
        sd_buffer,
        BUFFER_SIZE,
        &bytes_written
    );
    packet_count++;
    if (packet_count >= 50)
    {
        f_sync(&fil);
        packet_count = 0;
    }
}
void stop()
{

    if (stop_done)
    {
        return;
    }

    stop_done = 1;

    HAL_GPIO_WritePin(
        LED_start_GPIO_Port,
        LED_start_Pin,
        GPIO_PIN_RESET
    );

    f_sync(&fil);

    f_close(&fil);

    f_mount(
        NULL,
        "",
        1
    );

    HAL_TIM_Base_Stop_IT(
        &htim22
    );
}
void reset_buffer(
    uint8_t *buffer,
    uint16_t buffer_size
)
{
    uint16_t i;

    for (
        i = 0;
        i < buffer_size;
        i++
    )
    {
        buffer[i] = 0;
    }
}
void TIM22_Init_IT(void)
{
    HAL_TIM_Base_Start_IT(
        &htim22
    );

    HAL_NVIC_SetPriority(
        TIM22_IRQn,
        0,
        0
    );

    HAL_NVIC_EnableIRQ(
        TIM22_IRQn
    );
}
uint8_t init_sd(void)
{
    FRESULT fres;

    HAL_Delay(1000);

    fres = f_mount(
        &FatFs,
        "/",
        1
    );

    if (fres != FR_OK)
    {
    	HAL_GPIO_WritePin(
    	        LED_start_GPIO_Port,
    	        LED_start_Pin,
    	        GPIO_PIN_SET
    	    );

    	return 0;
    }

    fres = f_open(
        &fil,
        "POSTURE.BIN",
        FA_WRITE |
        FA_OPEN_ALWAYS
    );

    if (fres != FR_OK)
    {
    	HAL_GPIO_WritePin(
    	    	        LED_start_GPIO_Port,
    	    	        LED_start_Pin,
    	    	        GPIO_PIN_SET
    	    	    );

    	return 0;
    }

    f_lseek(
        &fil,
        f_size(&fil)
    );

    return 1;
}
uint8_t MPU_init(
    I2C_HandleTypeDef *hi2c
)
{
    uint8_t low_status;
    uint8_t high_status;

    low_status = MPU6050_Init(
        hi2c
    );

    high_status = MPU6050_Init_High(
        hi2c
    );

    if (
        (low_status == 0) &&
        (high_status == 0)
    )
    {
        return 0;
    }

    return 1;
}
void read_2_MPU(
    I2C_HandleTypeDef *hi2c,
    MPU6050_Data_t *mpu_low,
    MPU6050_Data_t *mpu_high
)
{
    MPU6050_Read_Accel(
        hi2c,
        mpu_low
    );

    MPU6050_Read_Accel_High(
        hi2c,
        mpu_high
    );
}

void packetnization(
    uint8_t *buffer,
    MPU6050_Data_t *mpu_low,
    MPU6050_Data_t *mpu_high,
    uint8_t posture
)
{
    uint32_t timestamp;
    uint8_t index = 0;

    timestamp = HAL_GetTick();

    // Timestamp
    buffer[index++] = (timestamp >> 24) & 0xFF;
    buffer[index++] = (timestamp >> 16) & 0xFF;
    buffer[index++] = (timestamp >> 8) & 0xFF;
    buffer[index++] = timestamp & 0xFF;

    // MPU LOW
    buffer[index++] = (mpu_low->Accel_X >> 8) & 0xFF;
    buffer[index++] = mpu_low->Accel_X & 0xFF;

    buffer[index++] = (mpu_low->Accel_Y >> 8) & 0xFF;
    buffer[index++] = mpu_low->Accel_Y & 0xFF;

    buffer[index++] = (mpu_low->Accel_Z >> 8) & 0xFF;
    buffer[index++] = mpu_low->Accel_Z & 0xFF;

    buffer[index++] = (mpu_low->Gyro_X >> 8) & 0xFF;
    buffer[index++] = mpu_low->Gyro_X & 0xFF;

    buffer[index++] = (mpu_low->Gyro_Y >> 8) & 0xFF;
    buffer[index++] = mpu_low->Gyro_Y & 0xFF;

    buffer[index++] = (mpu_low->Gyro_Z >> 8) & 0xFF;
    buffer[index++] = mpu_low->Gyro_Z & 0xFF;

    // MPU HIGH
    buffer[index++] = (mpu_high->Accel_X >> 8) & 0xFF;
    buffer[index++] = mpu_high->Accel_X & 0xFF;

    buffer[index++] = (mpu_high->Accel_Y >> 8) & 0xFF;
    buffer[index++] = mpu_high->Accel_Y & 0xFF;

    buffer[index++] = (mpu_high->Accel_Z >> 8) & 0xFF;
    buffer[index++] = mpu_high->Accel_Z & 0xFF;

    buffer[index++] = (mpu_high->Gyro_X >> 8) & 0xFF;
    buffer[index++] = mpu_high->Gyro_X & 0xFF;

    buffer[index++] = (mpu_high->Gyro_Y >> 8) & 0xFF;
    buffer[index++] = mpu_high->Gyro_Y & 0xFF;

    buffer[index++] = (mpu_high->Gyro_Z >> 8) & 0xFF;
    buffer[index++] = mpu_high->Gyro_Z & 0xFF;

    // Posture
    buffer[index++] = posture;
}

uint8_t read_SW3(void)
{
    static uint8_t last_raw = 0;
    static uint8_t stable_state = 0;
    static uint8_t counter = 0;

    static uint32_t last_tick = 0;

    uint8_t raw;
    uint32_t now;

    now = HAL_GetTick();

    if ((now - last_tick) < 20)
    {
        return stable_state;
    }

    last_tick = now;

    raw = HAL_GPIO_ReadPin(
        SW3_GPIO_Port,
        SW3_Pin
    );

    if (raw == last_raw)
    {
        if (counter < 4)
        {
            counter++;
        }

        if (counter >= 4)
        {
            stable_state = raw;
        }
    }
    else
    {
        counter = 0;
    }

    last_raw = raw;

    return stable_state;
}
uint8_t read_posture_sw(void)
{
    uint8_t sw_value = 0;

    sw_value |= (
        HAL_GPIO_ReadPin(
            SW0_GPIO_Port,
            SW0_Pin
        ) << 0
    );

    sw_value |= (
        HAL_GPIO_ReadPin(
            SW1_GPIO_Port,
            SW1_Pin
        ) << 1
    );

    sw_value |= (
        HAL_GPIO_ReadPin(
            SW2_GPIO_Port,
            SW2_Pin
        ) << 2
    );

    return sw_value;
}
//Interrupt
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim
)
{
    if (htim->Instance == TIM22)
    {
        system_input.data_flag = 1;
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
