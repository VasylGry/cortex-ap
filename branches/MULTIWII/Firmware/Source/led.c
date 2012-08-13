//============================================================================+
//
// $HeadURL: $
// $Revision: $
// $Date:  $
// $Author: $
//
/// \brief LED driver
///
/// \file
///
// Change:
//
//============================================================================*/

#include "stm32f10x.h"
#include "led.h"

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

/// pins connected to LEDs
const uint16_t GPIO_PIN[LED_NUM] =
{GREEN_PIN, BLUE_PIN};

/*---------------------------------- Globals ---------------------------------*/

/*----------------------------------- Locals ---------------------------------*/

/*--------------------------------- Prototypes -------------------------------*/

///----------------------------------------------------------------------------
///
/// \brief   turn on specified led
/// \param   Led = which led
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void LEDOn(Led_TypeDef Led)
{
  GPIOC->BSRR = GPIO_PIN[Led];
}

///----------------------------------------------------------------------------
///
/// \brief   turn off specified led
/// \param   Led = which led
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void LEDOff(Led_TypeDef Led)
{
  GPIOC->BRR = GPIO_PIN[Led];
}

///----------------------------------------------------------------------------
///
/// \brief   toglle status of specified led
/// \param   Led = which led
/// \return  -
/// \remarks -
///
///----------------------------------------------------------------------------
void LEDToggle(Led_TypeDef Led)
{
  GPIOC->ODR ^= GPIO_PIN[Led];
}

/*****END OF FILE****/
