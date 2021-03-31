/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "waveforms.h"
#include "arm_math.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

arm_fir_instance_f32 FIR_F32_Struct;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ARM_MATH_CM4

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac1;
DAC_HandleTypeDef hdac3;
DAC_HandleTypeDef hdac4;
DMA_HandleTypeDef hdma_dac3_ch1;

HRTIM_HandleTypeDef hhrtim1;

UART_HandleTypeDef hlpuart1;

OPAMP_HandleTypeDef hopamp4;
OPAMP_HandleTypeDef hopamp6;

TIM_HandleTypeDef htim6;

/* USER CODE BEGIN PV */

uint32_t CPU_Freq, HR_Master_mul, HRTIM_Master_per, HRTIM_Master_Freq;
uint32_t TIM6_PSC, TIM6_ARR, TIM6_Freq;

uint32_t sampling_rate, sampling_pt_cntr;
//int16_t sampling_pt;
q15_t sampling_pt;

arm_fir_instance_q15 FIR_Q15_Struct;
q15_t Q15_FIR_Output, firStateQ15[NUM_FIR_TAPS_Q15 + Sample_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM6_Init(void);
static void MX_DAC3_Init(void);
static void MX_OPAMP6_Init(void);
static void MX_DAC4_Init(void);
static void MX_OPAMP4_Init(void);
static void MX_HRTIM1_Init(void);
static void MX_DAC1_Init(void);
/* USER CODE BEGIN PFP */

void VT_AppendData_CMSIS(void);
void VT_HRTIM_Master_FreqCalc(void);

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
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_TIM6_Init();
  MX_DAC3_Init();
  MX_OPAMP6_Init();
  MX_DAC4_Init();
  MX_OPAMP4_Init();
  MX_HRTIM1_Init();
  MX_DAC1_Init();
  /* USER CODE BEGIN 2 */

  CPU_Freq = HAL_RCC_GetHCLKFreq();

  VT_HRTIM_Master_FreqCalc();

  TIM6_Freq = CPU_Freq / (TIM6_PSC * TIM6_ARR);

  if ((TIM6_Freq % HRTIM_Master_Freq) != 0)
  {
	  Error_Handler();
  }

  sampling_rate = TIM6_Freq / HRTIM_Master_Freq;

  for (uint16_t cntr = 0; cntr < MySine2000_SIZE; cntr++)
  {
  	MySine2000[cntr] += 682;
  	MySine2000[cntr] += MySine200[cntr % MySine200_SIZE];
  }

  /**
   * @brief  Initialization function for the Q15 FIR filter.
   * @param[in,out] S          points to an instance of the Q15 FIR filter structure.
   * @param[in]     numTaps    Number of filter coefficients in the filter. Must be even and greater than or equal to 4.
   * @param[in]     pCoeffs    points to the filter coefficients.
   * @param[in]     pState     points to the state buffer.
   * @param[in]     blockSize  number of samples that are processed at a time.
   * @return     The function returns either
   * <code>ARM_MATH_SUCCESS</code> if initialization was successful or
   * <code>ARM_MATH_ARGUMENT_ERROR</code> if <code>numTaps</code> is not a supported value.
   */
//  arm_status arm_fir_init_q15(
//        arm_fir_instance_q15 * S,
//        uint16_t numTaps,
//  const q15_t * pCoeffs,
//        q15_t * pState,
//        uint32_t blockSize);

  arm_fir_init_q15(&FIR_Q15_Struct, 6, aFilterCoeffB, firStateQ15, Sample_SIZE); // https://arm-software.github.io/CMSIS_5/DSP/html/group__FIR.html#gaf4bfa7e20520f3b6e9f03a52ad6d0e41

	/*##- Enable DAC Channel and associated DMA ##############################*/
	if(HAL_OK != HAL_DAC_Start_DMA(&hdac3, DAC_CHANNEL_1,
				   (uint32_t*)MySine2000, MySine2000_SIZE, DAC_ALIGN_12B_R))
	{
		/* Start DMA Error */
		Error_Handler();
	}

	/*##- Start OPAMP    #####################################################*/
	/* Enable OPAMP */
	if(HAL_OK != HAL_OPAMP_Start(&hopamp6))
	{
		Error_Handler();
	}

	/*##- Enable DAC Channel ##############################*/
	if(HAL_OK != HAL_DAC_Start(&hdac4, DAC_CHANNEL_1))
	{
		/* Start Error */
		Error_Handler();
	}

	/*##- Start OPAMP    #####################################################*/
	/* Enable OPAMP */
	if(HAL_OK != HAL_OPAMP_Start(&hopamp4))
	{
		Error_Handler();
	}

	/*##- Enable DAC Channel ##############################*/
	if(HAL_OK != HAL_DAC_Start(&hdac1, DAC_CHANNEL_1))
	{
		/* Start Error */
		Error_Handler();
	}

  /*##- Enable TIM peripheral counter ######################################*/
  if(HAL_OK != HAL_TIM_Base_Start(&htim6))
  {
  	Error_Handler();
  }

  if(HAL_OK != HAL_HRTIM_WaveformCounterStart_IT(&hhrtim1, HRTIM_TIMERID_MASTER))
  {
  	Error_Handler();
  }

//  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, SET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */
  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_ABOVE_160MHZ;
  sConfig.DAC_DMADoubleDataMode = DISABLE;
  sConfig.DAC_SignedFormat = DISABLE;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief DAC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC3_Init(void)
{

  /* USER CODE BEGIN DAC3_Init 0 */

  /* USER CODE END DAC3_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC3_Init 1 */

  /* USER CODE END DAC3_Init 1 */
  /** DAC Initialization
  */
  hdac3.Instance = DAC3;
  if (HAL_DAC_Init(&hdac3) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_ABOVE_160MHZ;
  sConfig.DAC_DMADoubleDataMode = DISABLE;
  sConfig.DAC_SignedFormat = DISABLE;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac3, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC3_Init 2 */

  /* USER CODE END DAC3_Init 2 */

}

/**
  * @brief DAC4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC4_Init(void)
{

  /* USER CODE BEGIN DAC4_Init 0 */

  /* USER CODE END DAC4_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC4_Init 1 */

  /* USER CODE END DAC4_Init 1 */
  /** DAC Initialization
  */
  hdac4.Instance = DAC4;
  if (HAL_DAC_Init(&hdac4) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_ABOVE_160MHZ;
  sConfig.DAC_DMADoubleDataMode = DISABLE;
  sConfig.DAC_SignedFormat = DISABLE;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac4, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC4_Init 2 */

  /* USER CODE END DAC4_Init 2 */

}

/**
  * @brief HRTIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_HRTIM1_Init(void)
{

  /* USER CODE BEGIN HRTIM1_Init 0 */

  /* USER CODE END HRTIM1_Init 0 */

  HRTIM_TimeBaseCfgTypeDef pTimeBaseCfg = {0};
  HRTIM_TimerCfgTypeDef pTimerCfg = {0};

  /* USER CODE BEGIN HRTIM1_Init 1 */

  /* USER CODE END HRTIM1_Init 1 */
  hhrtim1.Instance = HRTIM1;
  hhrtim1.Init.HRTIMInterruptResquests = HRTIM_IT_NONE;
  hhrtim1.Init.SyncOptions = HRTIM_SYNCOPTION_NONE;
  if (HAL_HRTIM_Init(&hhrtim1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_DLLCalibrationStart(&hhrtim1, HRTIM_CALIBRATIONRATE_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_HRTIM_PollForDLLCalibration(&hhrtim1, 10) != HAL_OK)
  {
    Error_Handler();
  }
  pTimeBaseCfg.Period = 54400;
  pTimeBaseCfg.RepetitionCounter = 0x00;
  pTimeBaseCfg.PrescalerRatio = HRTIM_PRESCALERRATIO_MUL16;
  pTimeBaseCfg.Mode = HRTIM_MODE_CONTINUOUS;
  if (HAL_HRTIM_TimeBaseConfig(&hhrtim1, HRTIM_TIMERINDEX_MASTER, &pTimeBaseCfg) != HAL_OK)
  {
    Error_Handler();
  }
  pTimerCfg.InterruptRequests = HRTIM_MASTER_IT_MREP;
  pTimerCfg.DMARequests = HRTIM_MASTER_DMA_NONE;
  pTimerCfg.DMASrcAddress = 0x0000;
  pTimerCfg.DMADstAddress = 0x0000;
  pTimerCfg.DMASize = 0x1;
  pTimerCfg.HalfModeEnable = HRTIM_HALFMODE_DISABLED;
  pTimerCfg.InterleavedMode = HRTIM_INTERLEAVED_MODE_DISABLED;
  pTimerCfg.StartOnSync = HRTIM_SYNCSTART_DISABLED;
  pTimerCfg.ResetOnSync = HRTIM_SYNCRESET_DISABLED;
  pTimerCfg.DACSynchro = HRTIM_DACSYNC_NONE;
  pTimerCfg.PreloadEnable = HRTIM_PRELOAD_DISABLED;
  pTimerCfg.UpdateGating = HRTIM_UPDATEGATING_INDEPENDENT;
  pTimerCfg.BurstMode = HRTIM_TIMERBURSTMODE_MAINTAINCLOCK;
  pTimerCfg.RepetitionUpdate = HRTIM_UPDATEONREPETITION_DISABLED;
  pTimerCfg.ReSyncUpdate = HRTIM_TIMERESYNC_UPDATE_UNCONDITIONAL;

  if (HAL_HRTIM_WaveformTimerConfig(&hhrtim1, HRTIM_TIMERINDEX_MASTER, &pTimerCfg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN HRTIM1_Init 2 */

  /* USER CODE END HRTIM1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief OPAMP4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OPAMP4_Init(void)
{

  /* USER CODE BEGIN OPAMP4_Init 0 */

  /* USER CODE END OPAMP4_Init 0 */

  /* USER CODE BEGIN OPAMP4_Init 1 */

  /* USER CODE END OPAMP4_Init 1 */
  hopamp4.Instance = OPAMP4;
  hopamp4.Init.PowerMode = OPAMP_POWERMODE_HIGHSPEED;
  hopamp4.Init.Mode = OPAMP_FOLLOWER_MODE;
  hopamp4.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
  hopamp4.Init.InternalOutput = DISABLE;
  hopamp4.Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp4.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
  if (HAL_OPAMP_Init(&hopamp4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OPAMP4_Init 2 */

  /* USER CODE END OPAMP4_Init 2 */

}

/**
  * @brief OPAMP6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OPAMP6_Init(void)
{

  /* USER CODE BEGIN OPAMP6_Init 0 */

  /* USER CODE END OPAMP6_Init 0 */

  /* USER CODE BEGIN OPAMP6_Init 1 */

  /* USER CODE END OPAMP6_Init 1 */
  hopamp6.Instance = OPAMP6;
  hopamp6.Init.PowerMode = OPAMP_POWERMODE_HIGHSPEED;
  hopamp6.Init.Mode = OPAMP_FOLLOWER_MODE;
  hopamp6.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_DAC;
  hopamp6.Init.InternalOutput = DISABLE;
  hopamp6.Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp6.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
  if (HAL_OPAMP_Init(&hopamp6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OPAMP6_Init 2 */

  /* USER CODE END OPAMP6_Init 2 */

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 17-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 4;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  TIM6_PSC = htim6.Init.Prescaler + 1;

  TIM6_ARR = htim6.Init.Period + 1;

  /* USER CODE END TIM6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

void VT_AppendData_CMSIS(void)
{
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_11);

//	sampling_pt = MySine2000[(sampling_pt_cntr += sampling_rate) % MySine2000_SIZE]; // sampling_pt_cntr will over flow

	sampling_pt = MySine2000[sampling_pt_cntr = (sampling_pt_cntr + sampling_rate) % MySine2000_SIZE];

	HAL_DAC_SetValue(&hdac4, DAC_CHANNEL_1, DAC_ALIGN_12B_R, sampling_pt);

	/**
	* @brief Processing function for the Q15 FIR filter.
	* @param[in]  S          points to an instance of the Q15 FIR structure.
	* @param[in]  pSrc       points to the block of input data.
	* @param[out] pDst       points to the block of output data.
	* @param[in]  blockSize  number of samples to process.
	*/
//	void arm_fir_q15(
//	const arm_fir_instance_q15 * S,
//	const q15_t * pSrc,
//		q15_t * pDst,
//		uint32_t blockSize);

	arm_fir_q15(&FIR_Q15_Struct, &sampling_pt, &Q15_FIR_Output, 1);

	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, Q15_FIR_Output);

//	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, SET);
}

void VT_HRTIM_Master_FreqCalc(void)
{
	HR_Master_mul = pow(2,(0x00000007 & HRTIM1->sMasterRegs.MCR)); // Master Tim multiplier, it is reversed HR_Master_mul = 2^PSC
	HRTIM_Master_per = HRTIM1->sMasterRegs.MPER;
	HRTIM_Master_Freq = (((uint64_t) CPU_Freq * 32 ) / HR_Master_mul) / HRTIM_Master_per;
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
//  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, SET);
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
