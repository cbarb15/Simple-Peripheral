#include <stdint.h>
#include <stddef.h>

#include <xdc/std.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/display/Display.h>
#include <ti/drivers/ADC.h>

/* Example/Board Header files */
#include "Board.h"
#include "myData.h"

Task_Struct myTask;
#define THREADSTACKSIZE    1024

extern Display_Handle dispHandle;
uint8_t myTaskStack[THREADSTACKSIZE];
uint16_t adcValue;
Semaphore_Handle adcSem;

Clock_Struct clkStruct;
Clock_Handle clkHandle;

void clock_Handler(UArg arg) {
    Semaphore_post(adcSem);
}



/********** myThread **********/
Task_FuncPtr *myThread(void *arg0) {

    ADC_Handle   adc;
    ADC_Params   params;

    Semaphore_Params semParams;
    Semaphore_Params_init(&semParams);

    adcSem = Semaphore_create(0, &semParams, Error_IGNORE);
    if(adcSem == NULL)
    {
        /* Semaphore_create() failed */
        Display_print0(dispHandle, 0, 0, "adcSem Semaphore creation failed\n");
        while (1);
    }

//    /* Initialize ADC drivers */
    ADC_init();
    ADC_Params_init(&params);
    adc = ADC_open(Board_ADC7, &params);

    if (adc == NULL) {
            Display_printf(dispHandle, 6, 0, "Error initializing ADC channel 0\n");
            while (1);
    }

    Clock_Params clkParams;

    Clock_Params_init(&clkParams);
    clkParams.period = 5000/Clock_tickPeriod;
    clkParams.startFlag = FALSE;

    /* Construct a periodic Clock Instance */
    Clock_construct(&clkStruct, (Clock_FuncPtr)clock_Handler,
                               5000/Clock_tickPeriod, &clkParams);

    clkHandle = Clock_handle(&clkStruct);
    Clock_start(clkHandle);


    while (1) {

        /* Pend on semaphore, tmp116Sem */
        Semaphore_pend(adcSem, BIOS_WAIT_FOREVER);

        /* Blocking mode conversion */
        ADC_convert(adc, &adcValue);
        MyData_SetParameter(MYDATA_DATA_ID, MYDATA_DATA_LEN, &adcValue);
        Display_printf(dispHandle, 18, 0, "ADC value: %d\n", adcValue);
        Clock_start(clkHandle);
    }
}

/********** myThread_create **********/
void myThread_create(void) {
    Task_Params taskParams;

    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = myTaskStack;
    taskParams.stackSize = THREADSTACKSIZE;
    taskParams.priority = 1;

   Task_construct(&myTask, myThread, &taskParams, Error_IGNORE);
}
