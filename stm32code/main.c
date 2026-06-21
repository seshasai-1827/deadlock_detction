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
#include "cmsis_os.h"
#include "deadlock_rf.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define THINKING           0
#define GOT_LEFT           1
#define WAITING_RIGHT      2
#define EATING             3
#define RELEASED           4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

osThreadId defaultTaskHandle;
osMutexId fork_0Handle;
osMutexId fork_1Handle;
osMutexId fork_2Handle;
osMutexId fork_3Handle;
osMutexId fork_4Handle;
/* USER CODE BEGIN PV */
osThreadId philosopherHandle[5];
osThreadId monitorHandle;

__attribute__((section(".noinit")))
volatile uint32_t recovery_count;


uint32_t philosopherID[5] =
{
    0,1,2,3,4
};

volatile float ml_score = 0.0f;
volatile uint32_t ml_trigger_count = 0;
volatile uint32_t philosopher_state[5];
volatile uint32_t philosopher_left_fork[5];
volatile uint32_t philosopher_right_fork[5];

volatile uint32_t deadlock_flag = 0;
volatile int waiting_for[5];
volatile int owns_fork[5];
volatile int recovery_requested = 0;
int graph[5][5];

int visited[5];
int recStack[5];

volatile uint32_t blocked_tasks_live = 0;
volatile uint32_t edge_count_live = 0;

volatile float graph_density_live = 0.0f;
volatile float mutex_utilization_live = 0.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
void PhilosopherTask(void const *argument);
void DeadlockMonitor(void const *argument);
int DFS(int node);
int DetectDeadlock(void);
void BuildWFG(void);
void RecoverDeadlock(void);
float PredictDeadlockScore(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
osThreadDef(
    Philosopher,
    PhilosopherTask,
    osPriorityNormal,
    0,
    256
);

osThreadDef(
    Monitor,
    DeadlockMonitor,
    osPriorityAboveNormal,
    0,
    256
);
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  for(int i=0;i<5;i++)
  {
      waiting_for[i] = -1;
      owns_fork[i] = -1;
  }

  if(recovery_count > 1000)
  {
      recovery_count = 0;
  }

  /* USER CODE END 2 */

  /* Create the mutex(es) */
  /* definition and creation of fork_0 */
  osMutexDef(fork_0);
  fork_0Handle = osMutexCreate(osMutex(fork_0));

  /* definition and creation of fork_1 */
  osMutexDef(fork_1);
  fork_1Handle = osMutexCreate(osMutex(fork_1));

  /* definition and creation of fork_2 */
  osMutexDef(fork_2);
  fork_2Handle = osMutexCreate(osMutex(fork_2));

  /* definition and creation of fork_3 */
  osMutexDef(fork_3);
  fork_3Handle = osMutexCreate(osMutex(fork_3));

  /* definition and creation of fork_4 */
  osMutexDef(fork_4);
  fork_4Handle = osMutexCreate(osMutex(fork_4));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  for(int i=0;i<5;i++)
  {
      philosopherHandle[i] =
          osThreadCreate(
              osThread(Philosopher),
              &philosopherID[i]
          );
  }

  monitorHandle =
      osThreadCreate(
          osThread(Monitor),
          NULL
      );
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
    for(;;)
    {
        osDelay(1000);
    }
}

void PhilosopherTask(void const *argument)
{
    uint32_t id =
        *(uint32_t *)argument;

    osMutexId forks[5] =
    {
        fork_0Handle,
        fork_1Handle,
        fork_2Handle,
        fork_3Handle,
        fork_4Handle
    };

    while(1)
    {
        philosopher_state[id] =
            THINKING;

        osDelay(500);
        if(recovery_count > 0)
        {
            osDelay(
                200
                + ((HAL_GetTick() ^ (id*7919)) % 1000)
            );
        }
        osMutexWait(
            forks[id],
            osWaitForever
        );
        owns_fork[id] = id;

        philosopher_left_fork[id] =
            id;

        philosopher_state[id] =
            GOT_LEFT;

        osDelay(100);

        philosopher_state[id] =
            WAITING_RIGHT;
        waiting_for[id] = (id+1)%5;
        osMutexWait(
            forks[(id+1)%5],
            osWaitForever
        );
        waiting_for[id] = -1;
        philosopher_right_fork[id] =
            (id+1)%5;

        philosopher_state[id] =
            EATING;

        osDelay(1000);

        osMutexRelease(
            forks[(id+1)%5]
        );

        osMutexRelease(
            forks[id]
        );
        owns_fork[id] = -1;

        philosopher_state[id] =
            RELEASED;

        osDelay(500);
    }
}

float PredictDeadlockScore(void)
{
    int blocked_tasks = 0;
    int used_mutexes = 0;
    int edge_count = 0;

    int16_t features[5];

    for(int i=0;i<5;i++)
    {
        if(waiting_for[i] != -1)
            blocked_tasks++;

        if(owns_fork[i] != -1)
            used_mutexes++;
    }

    BuildWFG();

    for(int i=0;i<5;i++)
    {
        for(int j=0;j<5;j++)
        {
            if(graph[i][j])
                edge_count++;
        }
    }

    float blocked_ratio =
        ((float)blocked_tasks) / 5.0f;

    float graph_density =
        ((float)edge_count) / 20.0f;

    float mutex_utilization =
        ((float)used_mutexes) / 5.0f;

    blocked_tasks_live =
        blocked_tasks;

    edge_count_live =
        edge_count;

    graph_density_live =
        graph_density;

    mutex_utilization_live =
        mutex_utilization;

    features[0] = blocked_tasks;

    features[1] =
        (int16_t)(blocked_ratio * 100);

    features[2] =
        edge_count;

    features[3] =
        (int16_t)(graph_density * 100);

    features[4] =
        (int16_t)(mutex_utilization * 100);

    return deadlock_rf_predict(
        features,
        5
    );
}

void DeadlockMonitor(void const *argument)
{
    static uint8_t triggered = 0;

    while(1)
    {
        ml_score =
            PredictDeadlockScore();

        if(ml_score > 0.50f)
        {
            if(!triggered)
            {
                ml_trigger_count++;
                triggered = 1;
            }
        }
        else
        {
            triggered = 0;
        }

        if(ml_score > 0.50f)
        {
            if(DetectDeadlock())
            {
                deadlock_flag = 1;
                recovery_requested = 1;

                RecoverDeadlock();
            }
        }
        else
        {
            deadlock_flag = 0;
        }

        osDelay(100);
    }
}


void BuildWFG(void)
{
    memset(graph,0,sizeof(graph));

    for(int i=0;i<5;i++)
    {
        if(waiting_for[i] != -1)
        {
            int fork =
                waiting_for[i];

            for(int j=0;j<5;j++)
            {
                if(owns_fork[j] == fork)
                {
                    graph[i][j] = 1;
                }
            }
        }
    }
}

int DFS(int node)
{
    visited[node] = 1;
    recStack[node] = 1;

    for(int v=0;v<5;v++)
    {
        if(graph[node][v])
        {
            if(!visited[v])
            {
                if(DFS(v))
                    return 1;
            }
            else if(recStack[v])
            {
                return 1;
            }
        }
    }

    recStack[node] = 0;

    return 0;
}

int DetectDeadlock(void)
{
    BuildWFG();

    memset(
        visited,
        0,
        sizeof(visited)
    );

    memset(
        recStack,
        0,
        sizeof(recStack)
    );

    for(int i=0;i<5;i++)
    {
        if(!visited[i])
        {
            if(DFS(i))
            {
                return 1;
            }
        }
    }

    return 0;
}

void RecoverDeadlock(void)
{
    recovery_count++;

    HAL_Delay(500);

    NVIC_SystemReset();
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
