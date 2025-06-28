

/***************************** Include Files *******************************/
#include "PL_Control_Unit.h"

/************************** Function Definitions ***************************/
void PL_Control_Set_Logging_State(u32 state){
	u32 temp;
    temp = PMU_PL_CONTROL_UNIT_mReadReg(LOG_CONTROL);
    temp &= 0xFFFFFFFE;
    PMU_PL_CONTROL_UNIT_mWriteReg(LOG_CONTROL, temp | state);
}

u32 PL_Control_Get_Logging_State(){
	u32 temp;
    temp = PMU_PL_CONTROL_UNIT_mReadReg(LOG_CONTROL);
    return temp & 0x1;
}

void PL_Control_Set_Fs_Cycles(u32 cycles){
    PMU_PL_CONTROL_UNIT_mWriteReg(Fs_CONF, (1<<31) | (u32)cycles);
}

u32 PL_Control_Get_Fs_Cycles(){
    return PMU_PL_CONTROL_UNIT_mReadReg(Fs_CONF);
}
void PL_Control_Set_Phase_Step(u32 Phase_Step){
    if((1 > Phase_Step) || ((MAX_SMPL_IN_CYCLE-1) < Phase_Step)){
        xil_printf("Requested Phase Step is out of range cuz Phase Width = %d and must be (1-%d)\r\n",MAX_SMPL_IN_CYCLE-1,SINE_WAVE_PHASE_WIDTH);
        return; 
    }
    PMU_PL_CONTROL_UNIT_mWriteReg(PHASE_STEP_CONF, (1<<31) | (u32)Phase_Step);
}

u32 PL_Control_Get_Phase_Step(){
    return PMU_PL_CONTROL_UNIT_mReadReg(PHASE_STEP_CONF);
}

void Set_Fs(u32 Fs){
    if (Fs > PL_CLK_FREQ) {
        xil_printf("Error setting Fs for Sine_Wave_Gen_0 module in PL: Fs out of range, min = 1, max = %f\r\n", PL_CLK_FREQ);
        return;
    }
    u32 cycles = PL_CLK_FREQ/Fs; 
    xil_printf("Actual reachable Fs = %d\r\n",PL_CLK_FREQ/cycles); 
    PL_Control_Set_Fs_Cycles(cycles);
}

u32 Get_Fs(void){
    u32 temp = PL_Control_Get_Fs_Cycles; 
    return (PL_CLK_FREQ/temp); 
}

void Set_Sine_Wave_Frq(u32 frq){
// phase_step = 2^8 x f_out/Fs
    u32 Fs = Get_Fs(); 
    u32 MinFout = Fs / MAX_SMPL_IN_CYCLE; 
    u32 MaxFout = (MAX_SMPL_IN_CYCLE-1) * Fs / MAX_SMPL_IN_CYCLE; 
    if(frq < MinFout || frq > MaxFout){
        xil_printf("Your requested freqency is out of range (%d-%d)\r\nThis is because Fs = %d\r\n",MinFout,MaxFout,Fs);
        return;
    }
    u32 Phase_Step = MAX_SMPL_IN_CYCLE * frq / Fs; 
    PL_Control_Set_Phase_Step(Phase_Step);
}



