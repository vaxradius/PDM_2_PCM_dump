#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"


#define BUF_SIZE		256

//*****************************************************************************
//
// Global variables.
//
//*****************************************************************************
volatile bool g_bPDMDataReady = false;

int16_t i16PDMBuf[2][BUF_SIZE] = {{0},{0}};
uint32_t u32PDMPingpong = 0;


//*****************************************************************************
//
// PDM configuration information.
//
//*****************************************************************************
void *PDMHandle;

am_hal_pdm_config_t g_sPdmConfig =
{
	.eClkDivider = AM_HAL_PDM_MCLKDIV_1,
	//.eClkDivider = AM_HAL_PDM_MCLKDIV_2,
	.eLeftGain = AM_HAL_PDM_GAIN_P210DB,
	.eRightGain = AM_HAL_PDM_GAIN_P210DB,
	//.ui32DecimationRate = 0xC,
	.ui32DecimationRate = 0x18,
	//.ui32DecimationRate = 0x30,
	.bHighPassEnable = 0,
	.ui32HighPassCutoff = 0xB,
	.ePDMClkSpeed = AM_HAL_PDM_CLK_750KHZ,
	//.ePDMClkSpeed = AM_HAL_PDM_CLK_1_5MHZ,
	//.ePDMClkSpeed = AM_HAL_PDM_CLK_3MHZ,
	//.ePDMClkSpeed = AM_HAL_PDM_CLK_6MHZ,
	.bInvertI2SBCLK = 0,
	.ePDMClkSource = AM_HAL_PDM_INTERNAL_CLK,
	.bPDMSampleDelay = 0,
	.bDataPacking = 1,
	.ePCMChannels = AM_HAL_PDM_CHANNEL_RIGHT,
	.bLRSwap = 0,
};



//*****************************************************************************
//
// PDM initialization.
//
//*****************************************************************************
void
pdm_init(void)
{
	//
	// Initialize, power-up, and configure the PDM.
	//
	am_hal_pdm_initialize(0, &PDMHandle);
	am_hal_pdm_power_control(PDMHandle, AM_HAL_PDM_POWER_ON, false);
	am_hal_pdm_configure(PDMHandle, &g_sPdmConfig);
	am_hal_pdm_enable(PDMHandle);

	//
	// Configure the necessary pins.
	//
	am_hal_gpio_pincfg_t sPinCfg = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    sPinCfg.uFuncSel = AM_HAL_PIN_12_PDMCLK;
    am_hal_gpio_pinconfig(12, sPinCfg);

    sPinCfg.uFuncSel = AM_HAL_PIN_11_PDMDATA;
    am_hal_gpio_pinconfig(11, sPinCfg);

	//
	// Configure and enable PDM interrupts (set up to trigger on DMA
	// completion).
	//
	am_hal_pdm_interrupt_enable(PDMHandle, (AM_HAL_PDM_INT_DERR
	                                        | AM_HAL_PDM_INT_DCMP
	                                        | AM_HAL_PDM_INT_UNDFL
	                                        | AM_HAL_PDM_INT_OVF));

#if AM_CMSIS_REGS
	NVIC_EnableIRQ(PDM_IRQn);
#else
	am_hal_interrupt_enable(AM_HAL_INTERRUPT_PDM);
#endif
}


//*****************************************************************************
//
// Start a transaction to get some number of bytes from the PDM interface.
//
//*****************************************************************************
void
pdm_data_get(int16_t *dest)
{
	//
	// Configure DMA and target address.
	//
	//am_hal_pdm_disable(PDMHandle);
	am_hal_pdm_transfer_t sTransfer;
	sTransfer.ui32TargetAddr = (uint32_t ) dest;
	sTransfer.ui32TotalCount = BUF_SIZE*2;

	//am_hal_pdm_fifo_flush(PDMHandle);

	//
	// Start the data transfer.
	//
	am_hal_pdm_dma_start(PDMHandle, &sTransfer);
	//am_hal_pdm_enable(PDMHandle);
}

//*****************************************************************************
//
// PDM interrupt handler.
//
//*****************************************************************************
void
am_pdm0_isr(void)
{
	uint32_t ui32Status;
	//am_hal_gpio_state_write(8 , AM_HAL_GPIO_OUTPUT_CLEAR);

	//
	// Read the interrupt status.
	//
	am_hal_pdm_interrupt_status_get(PDMHandle, &ui32Status, false);
	am_hal_pdm_interrupt_clear(PDMHandle, ui32Status);

	if (ui32Status & AM_HAL_PDM_INT_DCMP)
	{
		g_bPDMDataReady = true;
		pdm_data_get(i16PDMBuf[(++u32PDMPingpong)%2]);
	}
 	//am_hal_gpio_output_toggle(8); 
}


const am_hal_gpio_pincfg_t g_deepsleep_button0 =
{
    .uFuncSel = 3,
    .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};

void init_button0_detection(void)
{
	am_hal_gpio_pinconfig(AM_BSP_GPIO_BUTTON1, g_deepsleep_button0);
}

//*****************************************************************************
//
// Main
//
//*****************************************************************************
int
main(void)
{
	uint32_t index = (512*1024);
	uint32_t u32PDMpg;
	am_hal_burst_avail_e	eBurstModeAvailable;
	am_hal_burst_mode_e		eBurstMode;
	uint32_t ui32PinValue = 1;
	uint32_t ui32deBounce = 0;
	int8_t ret = 0;
	//
	// Perform the standard initialzation for clocks, cache settings, and
	// board-level low-power operation.
	//
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();

	am_hal_interrupt_master_disable();

	Uart_Init();

	am_hal_interrupt_master_enable();

	pdm_init();
	am_hal_pdm_fifo_flush(PDMHandle);
	am_hal_pdm_enable(PDMHandle);
	

	pdm_data_get(i16PDMBuf[0]);

	am_hal_gpio_pinconfig(6, g_AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_pinconfig(8, g_AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_state_write(6, AM_HAL_GPIO_OUTPUT_SET);
	am_hal_gpio_state_write(8, AM_HAL_GPIO_OUTPUT_SET);


	am_hal_gpio_state_write(AM_BSP_GPIO_LED0, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
	am_hal_gpio_state_write(AM_BSP_GPIO_LED1, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
	am_hal_gpio_state_write(AM_BSP_GPIO_LED2, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
	am_hal_gpio_state_write(AM_BSP_GPIO_LED3, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
	am_hal_gpio_state_write(AM_BSP_GPIO_LED4, AM_HAL_GPIO_OUTPUT_SET);

	//am_hal_gpio_pinconfig(AM_BSP_GPIO_LED0, g_AM_BSP_GPIO_LED0);
	//am_hal_gpio_pinconfig(AM_BSP_GPIO_LED1, g_AM_BSP_GPIO_LED1);
	//am_hal_gpio_pinconfig(AM_BSP_GPIO_LED2, g_AM_BSP_GPIO_LED2);
	//am_hal_gpio_pinconfig(AM_BSP_GPIO_LED3, g_AM_BSP_GPIO_LED3);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_LED4, g_AM_BSP_GPIO_LED4);


	am_hal_burst_mode_initialize(&eBurstModeAvailable);

	am_hal_burst_mode_enable(&eBurstMode);

	//
    // Enable floating point.
    //
    am_hal_sysctrl_fpu_enable();
    am_hal_sysctrl_fpu_stacking_enable(true);

    //
    // Turn ON Flash1
    //
    am_hal_pwrctrl_memory_enable(AM_HAL_PWRCTRL_MEM_FLASH_1M);


    //
    // Power on SRAM
    //
    PWRCTRL->MEMPWDINSLEEP_b.SRAMPWDSLP = PWRCTRL_MEMPWDINSLEEP_SRAMPWDSLP_NONE;


		
	init_button0_detection();

	ui32PinValue = 1;
	am_util_stdio_printf("Press & release BTN3 to erase Flash (start from 0x80000, size:512KB[0x80000])\r\n");

	while(ui32PinValue)
	{
		am_hal_gpio_state_read(AM_BSP_GPIO_BUTTON1, AM_HAL_GPIO_INPUT_READ, &ui32PinValue );
		
		if(ui32PinValue == 0)
		{
			am_hal_gpio_state_write(AM_BSP_GPIO_LED3, AM_HAL_GPIO_OUTPUT_SET);
			ret = storage_erase((512*1024), (512*1024));
			if(ret)
				am_util_stdio_printf("storage_erase failed \r\n");
			else
			{
				am_util_stdio_printf("storage_erase OK \r\n");
				am_util_stdio_printf("Press & HOLD BTN3 to record PCM into Flash\r\n");
				am_util_stdio_printf("HOLD BTN3 till message -Done- appeared \r\n");
			}
			break;
		}
	}

	am_hal_gpio_state_write(AM_BSP_GPIO_LED2, AM_HAL_GPIO_OUTPUT_SET);



	//
	// Loop forever while sleeping.
	//
	while (1)
	{

		if(g_bPDMDataReady == true)
		{
			g_bPDMDataReady = false;

			u32PDMpg = u32PDMPingpong;

			am_hal_gpio_state_read(AM_BSP_GPIO_BUTTON1, AM_HAL_GPIO_INPUT_READ, &ui32PinValue );
			if(ui32PinValue == 0)
			{
				if(ui32deBounce++ >= 3)
				{
					am_hal_gpio_state_write(AM_BSP_GPIO_LED1, AM_HAL_GPIO_OUTPUT_SET);
					ret = storage_write(index, i16PDMBuf[(u32PDMpg-1)%2], BUF_SIZE*2);
					if(ret)
						am_util_stdio_printf("storage_write failed \r\n");
					index += BUF_SIZE*2; 
					if(index > ((512+384)*1024))
					{
						am_util_stdio_printf("Done \r\n");
						
						am_hal_gpio_state_write(AM_BSP_GPIO_LED0, AM_HAL_GPIO_OUTPUT_SET);
						am_hal_interrupt_master_disable();
						am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
						while(1);
					}
				}
			}
			else
			{
				ui32deBounce = 0;
			}
		

			if(u32PDMpg != u32PDMPingpong)
			{
				am_util_stdio_printf("u32PDMpg != u32PDMPingpong \r\n");
				while(1);
			}
		}
		//
		// Go to Deep Sleep.
		//
		//am_hal_gpio_output_toggle(6);
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
		//am_hal_gpio_output_toggle(6);

	}
}

