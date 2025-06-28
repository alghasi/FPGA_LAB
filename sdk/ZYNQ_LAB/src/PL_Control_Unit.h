
#ifndef PL_CONTROL_UNIT_H
#define PL_CONTROL_UNIT_H


/****************** Include Files ********************/
#include "xil_types.h"
#include "xstatus.h"
#include "xparameters.h"
#include "xil_io.h"

#define Fs_CONF                                 0
#define PHASE_STEP_CONF                         4
#define LOG_CONTROL                             8

#define PMU_PL_CONTROL_UNIT_BASE_ADDR           XPAR_PL_CONTROL_UNIT_0_S00_AXI_BASEADDR
#define PL_CLK_FREQ                             100000000UL                             /* in Hz */
#define PL_CLK_PERIOD                           (1000000000.0 / (double)PL_CLK_FREQ)    /* in ns */
#define SINE_WAVE_PHASE_WIDTH                   8
#define MAX_SMPL_IN_CYCLE                       (1<<SINE_WAVE_PHASE_WIDTH) 




/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a PMU_PL_CONTROL_UNIT register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the PMU_PL_CONTROL_UNITdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void PMU_PL_CONTROL_UNIT_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define PMU_PL_CONTROL_UNIT_mWriteReg(RegOffset, Data) \
  	Xil_Out32((PMU_PL_CONTROL_UNIT_BASE_ADDR) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a PMU_PL_CONTROL_UNIT register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the PMU_PL_CONTROL_UNIT device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 PMU_PL_CONTROL_UNIT_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define PMU_PL_CONTROL_UNIT_mReadReg(RegOffset) \
    Xil_In32((PMU_PL_CONTROL_UNIT_BASE_ADDR) + (RegOffset))

/************************** Function Prototypes ****************************/
void PL_Control_Set_Logging_State(u32 state);
u32 PL_Control_Get_Logging_State();
void PL_Control_Set_Fs_Cycles(u32 cycles);
u32 PL_Control_Get_Fs_Cycles();
void PL_Control_Set_Phase_Step(u32 Phase_Step);
u32 PL_Control_Get_Phase_Step();
void Set_Fs(u32 Fs);
u32 Get_Fs(void);
void Set_Sine_Wave_Frq(u32 frq);
#endif // PMU_PL_CONTROL_UNIT_H
