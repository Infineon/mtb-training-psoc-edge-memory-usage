/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application in the CM33 CPU
*
* Related Document : See README.md
*
********************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/

#include "cybsp.h"
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define BLINKY_LED_DELAY_MSEC       (1000U)

/* The timeout value in microseconds used to wait for CM55 core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC    (10U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)
                                        
/*******************************************************************************
* Variables
*******************************************************************************/
CY_SECTION_SHAREDMEM __attribute__((used))  volatile uint32_t shared_sram_variable;
CY_SECTION_SOCMEMSRAMSHARED_BEGIN __attribute__((used))  volatile uint32_t socmem_variable;

/*******************************************************************************
* Function Name: gpio_isr_handler
********************************************************************************
* Summary:
*  Interrupt serive routine for CYBSP_USER_BTN_PORT and CYBSP_USER_BTN2_PORT.
*  This function checks which user button was presssed and clears the
*  interrupt.
*
* Parameter:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void gpio_isr_handler(void)
{
    /* Check if USER_BTN1 was pressed */
    if(1UL == Cy_GPIO_GetInterruptStatus(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN))
    {
        /* Clear the USER_BTN1 interrupt */
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN);
    }

    /* Check if USER_BTN2 was pressed */
    if(1UL == Cy_GPIO_GetInterruptStatus(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN))
    {
        /* Clear the USER_BTN2 interrupt */
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
    }
}
/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function of the CM33 non-secure application. 
* 
* It initializes the device and board peripherals. It also initializes the 
* retarget-io middleware to be used with the debug UART port using which 
* "Hello World!" is printed on the debug UART. The LED pin is initialized with
* default configurations. The CM55 core is enabled and then the programs enters
* an infinite while loop which toggles the LED1 with a frequency of 1 Hz.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_en_sysint_status_t int_status;

    cy_stc_sysint_t gpio_int_config =
    {
        .intrSrc = CYBSP_USER_BTN1_IRQ,
        .intrPriority = 7U,
    };

    /* Initialize the device and board peripherals. */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution. */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }
    
    int_status = Cy_SysInt_Init(&gpio_int_config, gpio_isr_handler);
    if(CY_SYSINT_SUCCESS != int_status)
    {
        handle_app_error();
    }

    NVIC_EnableIRQ(gpio_int_config.intrSrc);

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen. */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "PSOC Edge MCU: Hello world "
           "****************** \r\n\n");

    printf("Hello World!\r\n\n");
    printf("For more projects, "
           "visit our code examples repositories:\r\n\n");

    printf("https://github.com/Infineon/"
           "Code-Examples-for-ModusToolbox-Software\r\n\n");

    /* Enable CM55. */
    /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);
    
    /* Disable System SRAM (SoCMEM) */
	Cy_SysEnableSOCMEM(false);
    
    shared_sram_variable = 0xDEADBEEF;
    socmem_variable = 0xBEADCAFE;

    
    for(;;)
    {
		
        printf("Shared SRAM Variable: 0x%08X; Location: 0x%08X \n", 
               shared_sram_variable,
               (unsigned int)&shared_sram_variable);
        printf("SoCMEM Variable: 0x%08X; Location: 0x%08X \n\n", 
               socmem_variable,
               (unsigned int)&socmem_variable);
			
	    while(!(Cy_SCB_UART_IsTxComplete(CYBSP_DEBUG_UART_HW))) {};
			
	    Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
	
	    Cy_SysLib_Delay(500);
	        
	    Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
	}

}

/* [] END OF FILE */