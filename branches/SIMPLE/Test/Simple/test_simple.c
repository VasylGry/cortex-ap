//============================================================================+
//
// $HeadURL: $
// $Revision: $
// $Date:  $
// $Author: $
//
/// \brief main program
///
/// \file
///
/// \todo
/// 1) Move task definitions in the main.c file, export relevant functions
/// from specific modules and call them from inside task, this should improve
/// testability.
///
/// \todo
/// 2) Use only one data structure for SD file read/write, add a semaphore
/// to manage multiple accesses, this will reduce RAM usage by 512 bytes.
///
// Change: added #definitions for GPIOA configuration
//
//============================================================================*/
/*
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
*/
#include "stm32f10x.h"
/*
#include "i2c_mems_driver.h"
#include "l3g4200d_driver.h"
#include "adxl345_driver.h"
#include "servodriver.h"
#include "ppmdriver.h"
#include "usart1driver.h"
#include "diskio.h"

#include "config.h"
// uncomment telemetry type that applies
//#include "telemetry.h"
#include "mav_telemetry.h"
//#include "multiwii.h"
#include "attitude.h"
#include "log.h"
#include "nav.h"
*/
#include "led.h"

/** @addtogroup cortex_ap
  * @{
  */

/** @addtogroup main
  * @{
  */

/*--------------------------------- Definitions ------------------------------*/

#ifdef VAR_STATIC
#undef VAR_STATIC
#endif
#define VAR_STATIC static
#ifdef VAR_GLOBAL
#undef VAR_GLOBAL
#endif
#define VAR_GLOBAL

/*----------------------------------- Macros ---------------------------------*/

/*-------------------------------- Enumerations ------------------------------*/

/*----------------------------------- Types ----------------------------------*/

/*---------------------------------- Constants -------------------------------*/

/*---------------------------------- Globals ---------------------------------*/

/*----------------------------------- Locals ---------------------------------*/

/*--------------------------------- Prototypes -------------------------------*/

void RCC_Configuration(void);
void GPIO_Configuration(void);

/*--------------------------------- Functions --------------------------------*/


///----------------------------------------------------------------------------
///
/// \brief   main
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
int32_t main(void) {

  /* At this stage the microcontroller clock setting is already configured,
     this is done through SystemInit() function which is called from startup
     file (startup_stm32f10x_xx.s) before to branch to application main.
     To reconfigure the default setting of SystemInit() function, refer to
     system_stm32f10x.c file */

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);   // Configure priority group
/* equivalente:
  SCB->AIRCR = AIRCR_VECTKEY_MASK | NVIC_PriorityGroup_4;
*/
  RCC_Configuration();                              // System Clocks Configuration
  GPIO_Configuration();                             // GPIO Configuration

  while (1) {
  	LEDOn(BLUE);
	LEDOff(BLUE);
	LEDToggle(BLUE);
  }
}

///----------------------------------------------------------------------------
///
/// \brief   Configure the different system clocks.
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void RCC_Configuration(void)
{
  uint32_t tmpreg;
/*
  RCC_HCLKConfig(RCC_SYSCLK_Div1);    // HCLK = SYSCLK
  RCC_PCLK1Config(RCC_HCLK_Div2);     // PCLK1 = HCLK/2
  RCC_PCLK2Config(RCC_HCLK_Div1);     // PCLK2 = HCLK
*/
/* equivalente: */
  tmpreg = RCC->CFGR;
  tmpreg &= 0xFFFFFF0F;         // Clear HPRE[3:0] bits
  tmpreg |= 0x00000000;         // Set HPRE[3:0] bits according to RCC_SYSCLK_Div1
  tmpreg &= 0xFFFFF8FF;         // Clear PPRE1[2:0] bits
  tmpreg |= 0x00000400;         // Set PPRE1[2:0] bits according to RCC_HCLK_Div2
  tmpreg &= 0xFFFFC7FF;         // Clear PPRE2[2:0] bits
  tmpreg |= 0x00000000 << 3;    // Set PPRE2[2:0] bits according to RCC_HCLK_Div1
  RCC->CFGR = tmpreg;           // Store the new value
/*
  // TIM2, TIM3 USART2 clock enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 |  // Used for PPM signal capture
                         RCC_APB1Periph_TIM3 |  // Used for servo signal PWM
                         RCC_APB1Periph_USART2, // Used for GPS communication
                         ENABLE);
*/
/* equivalente: */
  tmpreg = 0x00000001 |  // Timer 2 used for PPM signal capture
           0x00000002 |  // Timer 3 used for servo signal PWM
           0x00020000;   // Usart 2 used for GPS communication
  RCC->APB1ENR |= tmpreg;
/*
  // GPIOA, GPIOB, GPIOC, USART1 clock enable
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | // TIM2, TIM3, USART1, USART2
                         RCC_APB2Periph_GPIOB | // TIM3
                         RCC_APB2Periph_GPIOC | // LED
                         RCC_APB2Periph_AFIO  | //
                         RCC_APB2Periph_USART1, // Used for telemetry
                         ENABLE);
*/
/* equivalente */
  tmpreg = 0x00000004 | // GPIOA used by TIM2, TIM3, USART1, USART2
           0x00000008 | // GPIOB used for TIM3
           0x00000010 | // GPIOC used for LEDs
           0x00000001 | // AFIO used for ...
           0x00004000;  // USART 1 used for telemetry
  RCC->APB2ENR |= tmpreg;
}

///----------------------------------------------------------------------------
///
/// \brief   Configure pins.
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void GPIO_Configuration(void)
{

#define GPIOA_CRL_MASK  0x00FF000F
#define GPIOA_CRL_BITS  0xAA004A80
                     //   ||  |||
                     //   ||  ||+- Pin  1: TIM2 CH 2 input pull down
                     //   ||  |+-- Pin  2: USART 2 TX alternate push pull 2 MHz
                     //   ||  +--- Pin  3: USART 2 RX input floating
                     //   |+------ Pin  6: TIM3 CH 1 alternate push pull 2 MHz
                     //   +------- Pin  7: TIM3 CH 2 alternate push pull 2 MHz

#define GPIOA_CRH_MASK  0xFFFFF00F
#define GPIOA_CRH_BITS  0x000004A0
                    //         ||
                    //         |+- Pin  9: USART 1 TX alternate push pull 2 MHz
                    //         +-- Pin 10: USART 1 RX input floating

  tempreg = GPIOA->CRL;
  tempreg &= GPIOA_CRL_MASK;
  tempreg |= GPIOA_CRL_BITS;
  GPIOA->CRL = tempreg;

  tempreg = GPIOA->CRH;
  tempreg &= GPIOA_CRH_MASK;
  tempreg |= GPIOA_CRH_BITS;
  GPIOA->CRH = tempreg;

  GPIO_InitTypeDef GPIO_InitStructure;
  uint32_t tempreg;

  /* GPIOA Configuration */

  // TIM2 Channel 2 as input pull down (1)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // USART 2 TX pin as alternate function push pull (2)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // USART 2 RX pin as input floating (3)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // TIM3 Channel 1, 2 as alternate function push-pull (6, 7)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // USART 1 TX pin as alternate function push pull (9)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // USART 1 RX pin as input floating (10)
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* GPIOB Configuration */

  // TIM3 Channel 3, 4 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* GPIOC Configuration */

  // LED pins as push pull outputs
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}


#ifdef  USE_FULL_ASSERT
///----------------------------------------------------------------------------
///
/// \brief   Reports the name of the source file and the source line number
///          where the assert_param error has occurred.
/// \param   file: pointer to the source file name
/// \param   line: assert_param error line source number
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/*****END OF FILE****/
