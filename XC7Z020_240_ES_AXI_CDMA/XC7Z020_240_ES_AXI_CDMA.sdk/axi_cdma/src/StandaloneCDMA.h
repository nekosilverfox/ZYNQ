/*
 * StandaloneCDMA.c
 *
 *  Created on: 2021��10��1��
 *      Author: mi
 */

#ifndef StandaloneCDMA
#define StandaloneCDMA

#include "xparameters.h"
#include <stdio.h>
#include "xaxicdma.h"
#include "xil_cache.h"
#include "xscutimer.h"
#define TIMER_LOAD_VALUE    0xFFFFFFFF
#define BRAM_GP0_ADDR 0x40000000
XAxiCdma_Config *axi_cdma_cfg;
XAxiCdma axi_cdma;
#define PS_OCM_Addr 0x10000000 // some address in OCM
#define PL_BRAM_Addr 0xC0000000 // Not 'seen' by the PS

//#define BUFF_LEN 16*4
//16384*32bit/8=65536Byte
//32768*32bit/8=131072Byte
#define BUFF_LEN 131072

XScuTimer Timer;
void hs_timer(void)
{
    int Status;
    XScuTimer_Config *ConfigPtr;
    XScuTimer *TimerInstancePtr=&Timer;
    /*
     * Initialize the Scu Private Timer so that it is ready to use.
     */
    ConfigPtr = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);

    /*
     * This is where the virtual address would be used, this example
     * uses physical address.
     */
    Status = XScuTimer_CfgInitialize(TimerInstancePtr, ConfigPtr, ConfigPtr->BaseAddr);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    else
    xil_printf("XScuTimer_CfgInitialize OK\n\r");
}

int RunStandaloneCDMA()
{
xil_printf("%c[2J",27);

    int i,k;
    int Status;
    volatile int CntValue1,CntValue2;
    u32 *Saddr2 = (u32 *)0x02000000;

    u32 *rx_buffer = (u32 *) PS_OCM_Addr;
    u32 *tx_buffer = (u32 *) PL_BRAM_Addr;
    u32 *rd_ram    = (u32 *) BRAM_GP0_ADDR;
    hs_timer()      ;
    xil_printf("-----------------------*-----------------------\n\r");
    xil_printf("-Simple DMA demo based on Zybo board          -\n\r");
    xil_printf("-write some data to DDR                       -\n\r");
    xil_printf("-move those data to bram and read it from GP0 -\n\r");
    xil_printf("-----------------------*-----------------------\n\r");

    // Set up the AXI CDMA
    printf("--Set up the AXI CDMA\n\r");
    axi_cdma_cfg = XAxiCdma_LookupConfig(XPAR_AXICDMA_0_DEVICE_ID);
    if (!axi_cdma_cfg) {
        printf("AXAxiCdma_LookupConfig failed\n\r");
    }

    Status = XAxiCdma_CfgInitialize(&axi_cdma, axi_cdma_cfg, axi_cdma_cfg->BaseAddress);
    if (Status == XST_SUCCESS ){
        printf("XAxiCdma_CfgInitialize succeed\n\r");
    }
    printf("--Disable Interrupt of AXI CDMA\n\r");
    XAxiCdma_IntrDisable(&axi_cdma, XAXICDMA_XR_IRQ_ALL_MASK);

    if (XAxiCdma_IsBusy(&axi_cdma)) {
    printf("AXI CDMA is busy...\n\r");
    while (XAxiCdma_IsBusy(&axi_cdma));
    }


    Xil_DCacheFlush();

    XScuTimer_LoadTimer(&Timer, TIMER_LOAD_VALUE);
    CntValue1 = XScuTimer_GetCounterValue(&Timer);
    XScuTimer_Start(&Timer);
    Status = XAxiCdma_SimpleTransfer(
                                     &axi_cdma,
                                     (u32) tx_buffer,
                                     (u32) rx_buffer,
                                     BUFF_LEN,
                                     NULL,
                                     NULL);

    Xil_DCacheFlush();

    CntValue2 = XScuTimer_GetCounterValue(&Timer);
    XScuTimer_Stop(&Timer);


    printf("=========================================\n");
    printf("DMA Move 32768*32bit Total Time: %d us\n\r", (CntValue1 - CntValue2) * 3 / 1000);
    printf("=========================================\n");

    // Wait until core isn't busy
    if (XAxiCdma_IsBusy(&axi_cdma)) {
    printf("AXI CDMA is busy...\n\r");
    while (XAxiCdma_IsBusy(&axi_cdma));
    }

    Xil_DCacheInvalidateRange((u32)PS_OCM_Addr, BUFF_LEN);

    for(i=0; i<4; i++)
    //for(i=0; i<BUFF_LEN/4; i++)
    {
        k = *(rx_buffer + i);
        xil_printf("The DMA read from address = %2d, the     read value = %8x-----------------------\n\r",i,k);
        xil_printf("The DMA read from address = %2d, the MSB read value = %8d-----------------------\n\r",i,k >> 16);
        xil_printf("The DMA read from address = %2d, the LSB read value = %8d-----------------------\n\n\n\r",i,k & 0x0000FFFF);
    }
    for(i=32764; i<=32767; i++)
    //for(i=0; i<BUFF_LEN/4; i++)
    {
        k = *(rx_buffer + i);
        xil_printf("The DMA read from address = %2d, the     read value = %8x-----------------------\n\r",i,k);
        xil_printf("The DMA read from address = %2d, the MSB read value = %8d-----------------------\n\r",i,k >> 16);
        xil_printf("The DMA read from address = %2d, the LSB read value = %8d-----------------------\n\r",i,k & 0x0000FFFF);
    }

    printf("================ Done! ================");

    return 0;
}

#endif

