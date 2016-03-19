#include <common.h>
#include <exports.h>

#define OK 0
#define ERROR 1

#define RF_BOARD_IO_BASE_ADDRESS 0x43C00000
#define BOARD_ID_OFFSET 0x10
#define CPRI_BOARD_ID   0x4100


#define BOARD_ID_CHECK_VARIABLE "board_id_check"
#define PASS "0"
#define FAIL "1"
#define PL_LOAD_FAIL "2"

#define BOOT_TYPE_VARIABLE "shboottype"
#define BOOT_TYPE_NORMAL "normal"
#define BOOT_TYPE_FALLBACK "fallback"

#define DEVCFG_INT_STS 0xf800700c
#define PL_DONE_INT    0x4

int main (int argc, char * const argv[])
{
	app_startup(argv);
	udelay(100 * 1000);	
	unsigned int pl_done = 	(unsigned int)(*(volatile unsigned int *)(0xf800700c));
	if((pl_done & PL_DONE_INT) != PL_DONE_INT){
		setenv(BOARD_ID_CHECK_VARIABLE,PL_LOAD_FAIL);
		setenv(BOOT_TYPE_VARIABLE,BOOT_TYPE_FALLBACK);
		return (ERROR);				
	}

	unsigned int board_id = (unsigned int)(*(volatile unsigned int *)(RF_BOARD_IO_BASE_ADDRESS + BOARD_ID_OFFSET));
	if((board_id & CPRI_BOARD_ID) == CPRI_BOARD_ID){
		setenv(BOARD_ID_CHECK_VARIABLE,PASS);
		setenv(BOOT_TYPE_VARIABLE,BOOT_TYPE_NORMAL);	
		return (OK);
	}
	else
	{	
		setenv(BOARD_ID_CHECK_VARIABLE,FAIL);
		setenv(BOOT_TYPE_VARIABLE,BOOT_TYPE_FALLBACK);				
		return (ERROR);				
	}
}
