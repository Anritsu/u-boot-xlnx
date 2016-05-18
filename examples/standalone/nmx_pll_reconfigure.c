/*
 * This program checks the board ID and loads the NMx PLL clock phase registers.
 * SS 03/16/2016
 *
 */
#include <common.h>
#include <exports.h>

#define OK 0
#define ERROR 1

#define RF_BOARD_IO_BASE_ADDRESS 0x43C00000
#define BOARD_ID_OFFSET 0x10
#define CPRI_BOARD_ID   0x4100
#define BOARD_ID_MASK   (~0xFF)

#define ZYNQ_CPU0_RUN_LEVEL_REG   0x48000008
#define PLL_LOAD_COMPLETE         0x1


#define BOARD_ID_CHECK_VARIABLE "board_id_check"
#define PASS "pass"
#define FAIL "fail"
#define PL_LOAD_FAIL "2"

#define BOOT_TYPE_VARIABLE "shboottype"
#define BOOT_TYPE_NORMAL "normal"
#define BOOT_TYPE_FALLBACK "fallback"

#define DEVCFG_INT_STS 0xf800700c
#define PL_DONE_INT    0x4

#define NMX_IO_BASE   0x48000000 // Zynq base address.
#define NMX_IO_SIZE   0x10000

#define NMX_PLL_RESET_BASE   0x48200000 // NMx FPGA PLL reset.
#define NMX_PLL_RESET_SIZE   0x1000    // IO size.
#define NMX_PLL_RESET_PLL_A  0x1 // bit 0
#define NMX_PLL_RESET_PLL_B  0x2 // bit 1

#define NMX_PLL_CLOCK_PHASE_BASE   0x49000000 // NMx FPGA PLL clock phase registers.
#define NMX_PLL_CLOCK_PHASE_PLL_A_ENABLE_OFFSET     0x100
#define NMX_PLL_CLOCK_PHASE_PLL_B_ENABLE_OFFSET     0x300
#define NMX_PLL_CLOCK_PHASE_MODULE_RX1_OFFSET        0x20  // PLL A
#define NMX_PLL_CLOCK_PHASE_MODULE_RX2_OFFSET        0x24
#define NMX_PLL_CLOCK_PHASE_MODULE_TX1_OFFSET        0x28
#define NMX_PLL_CLOCK_PHASE_MODULE_TX2_OFFSET        0x2C
#define NMX_PLL_CLOCK_PHASE_PROD_B_RX1_OFFSET       0x220  // PLL B so different base 0x200.
#define NMX_PLL_CLOCK_PHASE_PROD_B_RX2_OFFSET       0x224
#define NMX_PLL_CLOCK_PHASE_PROD_B_TX1_OFFSET        0x38
#define NMX_PLL_CLOCK_PHASE_PROD_B_TX2_OFFSET        0x3C

static void SetClockPhaseRegister1(unsigned int offset, unsigned int k);
static void SetClockPhaseRegister2(unsigned int offset, unsigned int k);
static void WriteWord32(unsigned int address, unsigned int value);
static unsigned int ReadWord32(unsigned int address);
static int board_id_check();

int main (int argc, char * const argv[])
{
	int retVal = OK;	
	app_startup(argv);
	
	retVal = board_id_check();
	if(retVal != OK)
		return (retVal);

	unsigned int nmxPllClockPhaseIoBaseAddr = (unsigned int)NMX_PLL_CLOCK_PHASE_BASE; 
	//---------------
	// NMx FPGA PLL reset register space
	unsigned int nmxPllResetSystemBaseAddr = (unsigned int)NMX_PLL_RESET_BASE;

	// Clock Phase register initialization
	// Enable
	WriteWord32(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_PLL_A_ENABLE_OFFSET, 0xFFFF);
	WriteWord32(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_PLL_B_ENABLE_OFFSET, 0xFFFF);
	udelay(1);

	// Set clock phase registers PLL A & B
	// Module <--> Controller registers.
	SetClockPhaseRegister1(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_MODULE_RX1_OFFSET, 4);
	SetClockPhaseRegister2(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_MODULE_RX2_OFFSET, 0);
	SetClockPhaseRegister1(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_MODULE_TX1_OFFSET, 6);
	SetClockPhaseRegister2(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_MODULE_TX2_OFFSET, 0);

	// Module to Product B registers
	SetClockPhaseRegister1(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_PROD_B_RX1_OFFSET, 2);
	SetClockPhaseRegister2(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_PROD_B_RX2_OFFSET, 0);
	SetClockPhaseRegister1(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_PROD_B_TX1_OFFSET, 2);
	SetClockPhaseRegister2(nmxPllClockPhaseIoBaseAddr + NMX_PLL_CLOCK_PHASE_PROD_B_TX2_OFFSET, 0);

	// Reset the FPGA PLL's to guarentee clock phases (from Eric T.)
	WriteWord32(nmxPllResetSystemBaseAddr, NMX_PLL_RESET_PLL_A | NMX_PLL_RESET_PLL_B);
	WriteWord32(nmxPllResetSystemBaseAddr, 0);

	//Set Run Level
	WriteWord32(ZYNQ_CPU0_RUN_LEVEL_REG, PLL_LOAD_COMPLETE);

	return (retVal);
}

static int board_id_check()
{
	udelay(100 * 1000);	
	unsigned int pl_done = 	ReadWord32(DEVCFG_INT_STS);
	if((pl_done & PL_DONE_INT) != PL_DONE_INT){
		setenv(BOARD_ID_CHECK_VARIABLE,PL_LOAD_FAIL);
		//setenv(BOOT_TYPE_VARIABLE,BOOT_TYPE_FALLBACK);
		return (ERROR);				
	}

	unsigned int board_id = (unsigned int)(*(volatile unsigned int *)(RF_BOARD_IO_BASE_ADDRESS + BOARD_ID_OFFSET));
	if((board_id & BOARD_ID_MASK) == CPRI_BOARD_ID){
		setenv(BOARD_ID_CHECK_VARIABLE,PASS);
		//setenv(BOOT_TYPE_VARIABLE,BOOT_TYPE_NORMAL);	
		return (OK);
	}
	else
	{	
		setenv(BOARD_ID_CHECK_VARIABLE,FAIL);
		//setenv(BOOT_TYPE_VARIABLE,BOOT_TYPE_FALLBACK);				
		return (ERROR);				
	}
}

// Set Reg1 clock phase registers.
static void SetClockPhaseRegister1(unsigned int address, unsigned int k)
{
	unsigned int tmp;

	tmp = ReadWord32(address);
	tmp = (tmp & 0x1FFF) | (k << 13);
	WriteWord32(address, tmp);
	udelay(1);

}


// Set Reg2 clock phase registers.
static void SetClockPhaseRegister2(unsigned int address, unsigned int k)
{
	unsigned int tmp;

	tmp = ReadWord32(address);
	tmp = (tmp & 0xFFE0) | k;
	WriteWord32(address, tmp);
	udelay(1);

}

static void WriteWord32(unsigned int address, unsigned int value)
{
	*((volatile unsigned int*)(unsigned long)(address)) = value;
}

static unsigned int ReadWord32(unsigned int address)
{
	unsigned int retVal = *(volatile unsigned int*)(unsigned long)(address);
	return retVal;
}
