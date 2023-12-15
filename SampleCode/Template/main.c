/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "misc_config.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_TIMER_PERIOD_1000MS                 	(flag_PROJ_CTL.bit0)
#define FLAG_PROJ_CAN_PERIOD                   			(flag_PROJ_CTL.bit1)
#define FLAG_PROJ_CAN_LOOPBACK            				(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_REVERSE3                              (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_REVERSE4                              (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)


/*_____ D E F I N I T I O N S ______________________________________________*/

volatile unsigned int counter_systick = 0;
volatile uint32_t counter_tick = 0;

CANFD_FD_MSG_T      g_sRxMsgFrame;
CANFD_FD_MSG_T      g_sTxMsgFrame;

uint8_t msg_rx_buffer_idx = 0;
uint8_t msg_tx_buffer_idx = 0;

uint8_t num_cnt = 0;

#define ENABLE_LOOP_BACK
// #define ENABLE_MONITOR_MODE

#define ENABLE_CAN_NORMAL
// #define ENABLE_CAN_FD

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

unsigned int get_systick(void)
{
	return (counter_systick);
}

void set_systick(unsigned int t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned int delay)
{  
    
    unsigned int tickstart = get_systick(); 
    unsigned int wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(unsigned int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

// void delay_ms(uint16_t ms)
// {
// 	TIMER_Delay(TIMER0, 1000*ms);
// }

void CAN_Rx_polling(void)
{
    uint8_t u8Cnt;

    if (CANFD_ReadRxFifoMsg(CANFD0, msg_rx_buffer_idx, &g_sRxMsgFrame) == eCANFD_RECEIVE_SUCCESS)    
    {
        if (g_sRxMsgFrame.eIdType == eCANFD_SID)
            printf("Rx FIFO1(Standard ID) ID = 0x%08X\r\n", g_sRxMsgFrame.u32Id);
        else
            printf("Rx FIFO1(Extended ID) ID = 0x%08X\r\n", g_sRxMsgFrame.u32Id);

        printf("Message Data(%02u bytes) : ", g_sRxMsgFrame.u32DLC);

        for (u8Cnt = 0; u8Cnt <  g_sRxMsgFrame.u32DLC; u8Cnt++)
        {
            // printf("%02u ,", g_sRxMsgFrame.au8Data[u8Cnt]);            
            printf("0x%2X ,", g_sRxMsgFrame.au8Data[u8Cnt]);
        }

        printf("\r\n");
        memset(&g_sRxMsgFrame, 0, sizeof(g_sRxMsgFrame));

    }
}

void CAN_SendMessage(CANFD_FD_MSG_T *psTxMsg, E_CANFD_ID_TYPE eIdType, uint32_t u32Id, uint8_t u8Len)
{
    uint8_t u8Cnt;

    psTxMsg->u32Id = u32Id;
    psTxMsg->eIdType = eIdType;
    psTxMsg->eFrmType = eCANFD_DATA_FRM;
    psTxMsg->bBitRateSwitch = 0;
    psTxMsg->u32DLC = u8Len;

    for (u8Cnt = 0; u8Cnt < psTxMsg->u32DLC; u8Cnt++)
    {
        psTxMsg->au8Data[u8Cnt] = u8Cnt + num_cnt;
    } 

    /* use message buffer 0 */
    if (eIdType == eCANFD_SID)
        printf("Send to transmit message 0x%08x (11-bit)\n", psTxMsg->u32Id);
    else
        printf("Send to transmit message 0x%08x (29-bit)\n", psTxMsg->u32Id);

    // if (CANFD_TransmitTxMsg(CANFD0, 0, psTxMsg) != eCANFD_TRANSMIT_SUCCESS)
    // {
    //     printf("Failed to transmit message\n");
    // }

    while (CANFD_TransmitTxMsg(CANFD0, msg_tx_buffer_idx , psTxMsg) != eCANFD_TRANSMIT_SUCCESS);

    printf("transmit done\r\n");

}


/*---------------------------------------------------------------------------*/
/*  Get the CANFD interface Nominal bit rate Function                        */
/*---------------------------------------------------------------------------*/
uint32_t Get_CANFD_NominalBitRate(CANFD_T *psCanfd)
{
    uint32_t u32BitRate = 0;
    uint32_t u32CanClk  = 0;
    uint32_t u32CanDiv  = 0;
    uint8_t  u8Tq = 0;
    uint8_t  u8NtSeg1 = 0;
    uint8_t  u8NtSeg2 = 0;

    if (CLK_GetModuleClockSource(CANFD0_MODULE) == (CLK_CLKSEL0_CANFD0SEL_HCLK >> CLK_CLKSEL0_CANFD0SEL_Pos))
        u32CanClk = CLK_GetHCLKFreq();
    else
        u32CanClk = CLK_GetHXTFreq();

    u32CanDiv = ((CLK->CLKDIV4 & CLK_CLKDIV4_CANFD0DIV_Msk) >> CLK_CLKDIV4_CANFD0DIV_Pos) + 1;
    u32CanClk = u32CanClk / u32CanDiv;
    u8Tq = ((psCanfd->NBTP & CANFD_NBTP_NBRP_Msk) >> CANFD_NBTP_NBRP_Pos) + 1 ;
    u8NtSeg1 = ((psCanfd->NBTP & CANFD_NBTP_NTSEG1_Msk) >> CANFD_NBTP_NTSEG1_Pos);
    u8NtSeg2 = ((psCanfd->NBTP & CANFD_NBTP_NTSEG2_Msk) >> CANFD_NBTP_NTSEG2_Pos);
    u32BitRate = u32CanClk / u8Tq / (u8NtSeg1 + u8NtSeg2 + 3);

    return u32BitRate;
}
/*---------------------------------------------------------------------------*/
/*  Get the CANFD interface Data bit rate Function                           */
/*---------------------------------------------------------------------------*/
uint32_t Get_CANFD_DataBitRate(CANFD_T *psCanfd)
{
    uint32_t u32BitRate = 0;
    uint32_t u32CanClk  = 0;
    uint32_t u32CanDiv  = 0;
    uint8_t  u8Tq = 0;
    uint8_t  u8NtSeg1 = 0;
    uint8_t  u8NtSeg2 = 0;

    if (CLK_GetModuleClockSource(CANFD0_MODULE) == (CLK_CLKSEL0_CANFD0SEL_HCLK >> CLK_CLKSEL0_CANFD0SEL_Pos))
        u32CanClk = CLK_GetHCLKFreq();
    else
        u32CanClk = CLK_GetHXTFreq();

    u32CanDiv = ((CLK->CLKDIV4 & CLK_CLKDIV4_CANFD0DIV_Msk) >> CLK_CLKDIV4_CANFD0DIV_Pos) + 1;
    u32CanClk = u32CanClk / u32CanDiv;
    u8Tq = ((psCanfd->DBTP & CANFD_DBTP_DBRP_Msk) >> CANFD_DBTP_DBRP_Pos) + 1 ;
    u8NtSeg1 = ((psCanfd->DBTP & CANFD_DBTP_DTSEG1_Msk) >> CANFD_DBTP_DTSEG1_Pos);
    u8NtSeg2 = ((psCanfd->DBTP & CANFD_DBTP_DTSEG2_Msk) >> CANFD_DBTP_DTSEG2_Pos);
    u32BitRate = u32CanClk / u8Tq / (u8NtSeg1 + u8NtSeg2 + 3);

    return u32BitRate;
}

void CAN_Init(uint8_t on)
{
    CANFD_FD_T sCANFD_Config;

    #if defined (ENABLE_CAN_NORMAL)
    uint32_t u32NormBitRate = 1000000;
    uint32_t u32DataBitRate = 0;
    #elif defined (ENABLE_CAN_FD)
    uint32_t u32NormBitRate = 1000000;
    uint32_t u32DataBitRate = 4000000;
    #endif

    SYS_ResetModule(CANFD0_RST);

    /*Get the CAN configuration value*/
    CANFD_GetDefaultConfig(&sCANFD_Config, CANFD_OP_CAN_MODE);

    #if defined (ENABLE_LOOP_BACK)
    /*Enable internal loopback mode*/
    if (on)
    {
        sCANFD_Config.sBtConfig.bEnableLoopBack = TRUE;
    }
    else
    {
        sCANFD_Config.sBtConfig.bEnableLoopBack = FALSE;
    }
    #endif

    sCANFD_Config.sBtConfig.sNormBitRate.u32BitRate = u32NormBitRate;
    sCANFD_Config.sBtConfig.sDataBitRate.u32BitRate = u32DataBitRate;
    /*Open the CAN FD feature*/
    CANFD_Open(CANFD0, &sCANFD_Config);
    printf("CANFD monitoring Nominal baud rate(bps): %d\r\n", Get_CANFD_NominalBitRate(CANFD0));
    printf("CANFD monitoring Data baud rate(bps): %d\r\n", Get_CANFD_DataBitRate(CANFD0));

    #if defined (ENABLE_MONITOR_MODE)
    /*Enable the Bus Monitoring Mode */
    CANFD0->CCCR |= CANFD_CCCR_MON_Msk;
    #endif

    CANFD_SetGFC(CANFD0, eCANFD_ACC_NON_MATCH_FRM_RX_FIFO0, eCANFD_ACC_NON_MATCH_FRM_RX_FIFO0, 1, 1);
    /* Enable RX fifo0 new message interrupt using interrupt line 0. */
    // CANFD_EnableInt(CANFD0, (CANFD_IE_TOOE_Msk | CANFD_IE_RF0NE_Msk), 0, 0, 0);
    /* CAN FD0 Run to Normal mode  */
    CANFD_RunToNormal(CANFD0, TRUE);
}

void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            FLAG_PROJ_TIMER_PERIOD_1000MS = 1;//set_flag(flag_timer_period_1000ms ,ENABLE);
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void loop(void)
{
	// static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (FLAG_PROJ_TIMER_PERIOD_1000MS)//(is_flag_set(flag_timer_period_1000ms))
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 0;//set_flag(flag_timer_period_1000ms ,DISABLE);

        // printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        PB0 ^= 1;        

        if (FLAG_PROJ_CAN_PERIOD)
        {
            num_cnt += 0x10;
            CAN_SendMessage(&g_sTxMsgFrame, eCANFD_SID, 0x5555, 8);
        }
    }

    CAN_Rx_polling();
    
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART4);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		printf("press : %c\r\n" , res);
		switch(res)
		{
			case '1':    
                CAN_SendMessage(&g_sTxMsgFrame, eCANFD_SID, 0x1111, 8);
				break;
			case '2':
                CAN_SendMessage(&g_sTxMsgFrame, eCANFD_SID, 0x2222, 8);
				break;
			case '3':
                CAN_SendMessage(&g_sTxMsgFrame, eCANFD_XID, 0x3333, 8);        
				break;
			case '4':
                CAN_SendMessage(&g_sTxMsgFrame, eCANFD_XID, 0x4444, 8);    
				break;

            case '5':
                FLAG_PROJ_CAN_PERIOD = 1;
                break;
            case '6':
                FLAG_PROJ_CAN_LOOPBACK ^= 1;
                printf("loopback : %d\r\n",FLAG_PROJ_CAN_LOOPBACK);
                CAN_Init(FLAG_PROJ_CAN_LOOPBACK);
                break;


			case 'X':
			case 'x':
			case 'Z':
			case 'z':
                SYS_UnlockReg();
				// NVIC_SystemReset();	// Reset I/O and peripherals , only check BS(FMC_ISPCTL[1])
                // SYS_ResetCPU();     // Not reset I/O and peripherals
                SYS_ResetChip();    // Reset I/O and peripherals ,  BS(FMC_ISPCTL[1]) reload from CONFIG setting (CBS)	
				break;
		}
	}
}

void UART4_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART4, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART4) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART4->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART4, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART4_Init(void)
{
    SYS_ResetModule(UART4_RST);

    /* Configure UART4 and set UART4 baud rate */
    UART_Open(UART4, 115200);
    UART_EnableInt(UART4, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART4_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

    #if 0
    printf("FLAG_PROJ_TIMER_PERIOD_1000MS : 0x%2X\r\n",FLAG_PROJ_TIMER_PERIOD_1000MS);
    printf("FLAG_PROJ_REVERSE1 : 0x%2X\r\n",FLAG_PROJ_REVERSE1);
    printf("FLAG_PROJ_REVERSE2 : 0x%2X\r\n",FLAG_PROJ_REVERSE2);
    printf("FLAG_PROJ_REVERSE3 : 0x%2X\r\n",FLAG_PROJ_REVERSE3);
    printf("FLAG_PROJ_REVERSE4 : 0x%2X\r\n",FLAG_PROJ_REVERSE4);
    printf("FLAG_PROJ_REVERSE5 : 0x%2X\r\n",FLAG_PROJ_REVERSE5);
    printf("FLAG_PROJ_REVERSE6 : 0x%2X\r\n",FLAG_PROJ_REVERSE6);
    printf("FLAG_PROJ_REVERSE7 : 0x%2X\r\n",FLAG_PROJ_REVERSE7);
    #endif

}

void GPIO_Init (void)
{
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB0MFP_Msk)) | (SYS_GPB_MFPL_PB0MFP_GPIO);

    GPIO_SetMode(PB, BIT0, GPIO_MODE_OUTPUT);

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

   CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
   CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);	

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);	

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    // CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));    

    CLK_EnableModuleClock(GPB_MODULE);

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    /* Select CAN FD0 clock source is HCLK */
    CLK_SetModuleClock(CANFD0_MODULE, CLK_CLKSEL0_CANFD0SEL_HCLK, CLK_CLKDIV4_CANFD0(1));
    /* Enable CAN FD0 peripheral clock */
    CLK_EnableModuleClock(CANFD0_MODULE);


    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB12MFP_Msk)) | SYS_GPB_MFPH_PB12MFP_CAN0_RXD;
    SYS->GPB_MFPH = (SYS->GPB_MFPH & (~SYS_GPB_MFPH_PB13MFP_Msk)) | SYS_GPB_MFPH_PB13MFP_CAN0_TXD;


    /* Debug UART clock setting */
    UartDebugCLK();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    UartDebugMFP();


    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M251 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART4 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	GPIO_Init();
	UART4_Init();
	TIMER1_Init();

    SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    CAN_Init(1);


    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
