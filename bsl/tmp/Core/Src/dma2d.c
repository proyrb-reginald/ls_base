/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma2d.c
  * @brief   This file provides code for the configuration
  *          of the DMA2D instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "dma2d.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* DMA2D init function */
void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* Peripheral clock enable */
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_DMA2D);

  /* DMA2D interrupt Init */
  NVIC_SetPriority(DMA2D_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(DMA2D_IRQn);

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  LL_DMA2D_SetMode(DMA2D, LL_DMA2D_MODE_M2M);
  LL_DMA2D_SetOutputColorMode(DMA2D, LL_DMA2D_OUTPUT_MODE_RGB565);
  LL_DMA2D_SetLineOffset(DMA2D, 0);
  LL_DMA2D_FGND_SetColorMode(DMA2D, LL_DMA2D_INPUT_MODE_RGB565);
  LL_DMA2D_FGND_SetAlphaMode(DMA2D, LL_DMA2D_ALPHA_MODE_NO_MODIF);
  LL_DMA2D_FGND_SetAlpha(DMA2D, 0);
  LL_DMA2D_FGND_SetLineOffset(DMA2D, 0);
  LL_DMA2D_FGND_SetRBSwapMode(DMA2D, LL_DMA2D_RB_MODE_REGULAR);
  LL_DMA2D_FGND_SetAlphaInvMode(DMA2D, LL_DMA2D_ALPHA_REGULAR);
  LL_DMA2D_FGND_SetChrSubSampling(DMA2D, LL_DMA2D_CSS_444);
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
