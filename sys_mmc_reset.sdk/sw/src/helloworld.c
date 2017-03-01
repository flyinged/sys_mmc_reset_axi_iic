#include <stdio.h>
#include "xil_printf.h"
#include "xiic_l.h"
#include <assert.h>
#include "gpio.h"

#define MMC_I2C_ADDR7 0x3E

/* MMC register map */
#define MMC_XCMD_WREG        0x800 //execute command
//#define MMC_CMD_RESULT_WREG  0x804 //result of last command, read only
#define MMC_ADDR_WREG        0x808 //address used to read/write flash and SD-card
#define MMC_DATA_WREG        0x80C //data register used by commands that need a data argument (eg: set file length)
#define MMC_SECURE_KEY_WREG  0x810 //used to grant access to secured commands

#define MMC_XCMD_RREG        0x900 //execute command
#define MMC_CMD_RESULT_RREG  0x904 //result of last command
#define MMC_ADDR_RREG        0x908 //address used to read/write flash and SD-card
#define MMC_DATA_RREG        0x90C //data register used by commands that need a data argument (eg: set file length)
#define MMC_SECURE_KEY_RREG  0x910 //used to grant access to secured commands

#define MMC_FLASH_WBUF_ADDR 0x1000
#define MMC_FLASH_RBUF_ADDR 0x1100

/* other constants */
#define MMC_SECURE_KEY      0x4F50454E //security key used to unlock commands (ASCII for 'OPEN')
#define MMC_FLASH_BUF_LEN   256
#define FLASH_SECTOR_SIZE   0x10000
#define SD_SECTOR_SIZE      0x200
/* MMC command codes */
#define MMC_CMD_NULL   0x0000 //no effect. can be used to set the read address for the command registers
#define MMC_CMD_FREAD  0x0001 //read 256B from FLASH's address stored in MMC_ADDR_REG. Data is stored in FLASH_RBUF_ADDR
#define MMC_CMD_FERASE 0x0002 //erase a 64K FLASH sector starting from address MMC_ADDR_REG. Address shall be 64K aligned (n*0x10000)
#define MMC_CMD_FPROG  0x0003 //Programs 256B into flash FLASH at address stored in MMC_ADDR_REG. Data is taken from FLASH_WBUF_ADDR
#define MMC_CMD_IAP0   0x0004 //Executes IAP (MMC FW update) using firmware tored in FLASH at address 0x0
#define MMC_CMD_IAP1   0x0005 //Executes IAP (MMC FW update) using firmware tored in FLASH at address 0x100000
#define MMC_CMD_RESET  0x0006 //Resets MMC's CPU (causes a power cycle)
#define MMC_CMD_SDREAD 0x0007 //read 16B from SDcard's address stored in MMC_ADDR_REG. Data is stored in FLASH_RBUF_ADDR
#define MMC_CMD_SDPROG 0x0008 //write 16B to SDcard's address stored in MMC_ADDR_REG. Data is taken from FLASH_WBUF_ADDR
#define MMC_CMD_TSD    0x0009 //Execute timed shutdown (switch off power supply for 5 seconds, then switch on again)
#define MMC_CMD_WRLEN  0x000A //Write file length. Uses the file address from the ADDR register, and the length from the DATA register.
#define MMC_CMD_CRC    0x000B //Compute CRC on file. Uses current address register to select file, and stored file length.

#define buf8_to_16(x) ((x[0]<<8) | x[1])
#define buf8_to_32(x) ((x[0]<<24) | (x[1]<<16) | (x[2]<<8) | x[3] )

char inbyte(void);

void wait_us(u32 n)
{
	u32 i = 0;

	assert(n < ((u32) -1) / 10);
	for (i = 0; i < 10 * n; i++)
	{
	}
}

void wait_ms(u32 n)
{
	u32 i = 0;

	for (i = 0; i < n; i++)
	{
		wait_us(1000);
	}
}

void wait_s(u32 n)
{
	u32 i = 0;

	for (i = 0; i < n; i++)
	{
		wait_ms(1000);
	}
}


/* Send a 16 bit I2C transaction to the MMC */
void mmc_send16(u8 c1, u8 c2)
{
	u32 n = 0;

	u8 buf[2] = {c1, c2};

	n = XIic_Send(XPAR_AXI_IIC_0_BASEADDR, MMC_I2C_ADDR7, buf, 2, XIIC_STOP);
	assert(n == 2);
	wait_ms(1);
}


/* get a 32bit hex from STDIN
 * MSG: string printed before asking for user input
 * NMAX: max number of input characters (input can also be terminated before reaching
 * the maximum by hitting CR on the keyboard)
 *
 * returns: input string converted from HEX to UINT32
 */
u32 hex_from_console(const char* msg, u8 nmax){

    u8 i, step, inchar[9];
    u32 outval;

    if (nmax>8) {
        xil_printf("\r\nERROR: nmax shall be <9\r\n");
        return 0;
    }

    xil_printf(msg);
    //13=CR
    //48='0'

    //first get string
    step = 0;
    while(1) {
        inchar[step] = inbyte();

        if (inchar[step] == 13) { //check if termination character
            break;
        } else if (
                inchar[step]<48 ||
                (inchar[step]>57 && inchar[step]<65) ||
                (inchar[step]>70 && inchar[step]<97) ||
                inchar[step]>102 ) { //first check if character is valid
            step = 0;
            xil_printf("\r\nNot a valid hex digit\r\n");
        } else if (step == 8) { // && inchar[step] != 13) { //then check if string is too long
            step = 0;
            xil_printf( "\r\nHex number is too big: maximum 32 bits allowed.\r\n");
        } else { //continue
            inchar[step+1] = 0;
            xil_printf("%s", inchar+step);
            step++;
        }

        if (step == nmax) break;
    }

    xil_printf("\n\r");

    //now process the value
    outval = 0;
    for (i = 0; i < step; i++) {
        switch (inchar[i]) {
        case 48 ... 57:
            outval += ( (inchar[i]-48) << ((step-i-1)*4) );
            break;
        case 65 ... 70:
            outval += ( (inchar[i]-55) << ((step-i-1)*4) );
            break;
        case 97 ... 102:
            outval += ( (inchar[i]-87) << ((step-i-1)*4) );
            break;
        }
    }

    return outval;
}

/* Send a 32 bit I2C transaction to the MMC
 *   Transaction's payload : addr(MSB), addr(LSB), data(MSB), data(LSB)
 *   returns the number of bytes sent (shall be always 4)
 */
unsigned mmc_send32(u16 addr, u16 data) {

	u8 txbuf[4];

	txbuf[0] = addr >> 8;
	txbuf[1] = addr & 0xFF;
	txbuf[2] = data >> 8;
	txbuf[3] = data & 0xFF;

	return( XIic_Send(XPAR_AXI_IIC_0_BASEADDR, MMC_I2C_ADDR7, txbuf, 4, XIIC_STOP) );
}

/* Read N bytes of data via I2C from MMC
 * The read address shall be set previously with a write transaction (either 32 or 16 bit)
 * RXBUF: buffer used to store the received data
 * N    : number of bytes requested
 * returns the number of bytes received (shall be N)
 */
unsigned mmc_read(u8 *rxbuf, u16 n) {

	return( XIic_Recv(XPAR_AXI_IIC_0_BASEADDR, MMC_I2C_ADDR7, rxbuf, n, XIIC_STOP) );

}

/* Execute GPAC3 command
 * Writes the CMD argument to address MMC_XCMD_REG (0x800)
 * The command outcome shall be always checked by reading the RESULT register with mmc_get_cmd_res()
 *
 * returns the number of bytes sent (shall be 4)
 */
unsigned mmc_execute_cmd(u16 cmd) {

	return( mmc_send32(MMC_XCMD_WREG, cmd) );

}

/* write MMC address register
 *
 * returns number of bytes sent (shall be 8)
 */
unsigned mmc_set_addr(u32 addr) {
	unsigned ret = 0;

	ret += mmc_send32(MMC_ADDR_WREG, addr>>16);
	ret += mmc_send32(MMC_ADDR_WREG+2, addr&0xFFFF);

	return ret;
}

/* get current address register stored in MMC */
u32 mmc_get_addr(void) {
	u8 rxbuf[4];

	mmc_send32(MMC_ADDR_RREG, 0);
	mmc_read(rxbuf, 4);

	return (buf8_to_32(rxbuf));
}

/* write MMC data register */
void mmc_set_data(u32 data) {
	mmc_send32(MMC_DATA_WREG, data>>16);
	mmc_send32(MMC_DATA_WREG+2, data&0xFFFF);
}

/* get current data register stored in MMC */
u32 mmc_get_data(void) {
	u8 rxbuf[4];

	mmc_send32(MMC_DATA_RREG, 0);
	mmc_read(rxbuf, 4);

	return (buf8_to_32(rxbuf));
}

/* get command result register stored in MMC */
u32 mmc_get_cmd_res(void) {
	u8 rxbuf[4];

	mmc_send32(MMC_CMD_RESULT_RREG, 0);
	mmc_read(rxbuf, 4);

	return (buf8_to_32(rxbuf));
}


/* Get FLASH data buffer from MMC
 * returns number of bytes read
 */
unsigned mmc_get_buffer(u8 *buf, u16 n) {

	n = (n>MMC_FLASH_BUF_LEN)?MMC_FLASH_BUF_LEN:n;

	mmc_send32(MMC_FLASH_RBUF_ADDR, 0);
	return (mmc_read(buf, n)) ;
}

/* Set FLASH data buffer in MMC */
unsigned mmc_set_buffer(u8 *buf, u16 n) {

	unsigned ret = 0, i;

	n = (n>MMC_FLASH_BUF_LEN)?MMC_FLASH_BUF_LEN:n;

	for (i=0; i<n; i+=2) {
		ret += mmc_send32(i+MMC_FLASH_WBUF_ADDR, buf8_to_16((buf+i)) );
	}

	return (ret);
}

/* get all command registers from MMC */
unsigned mmc_get_cmd_regs(u8 *buf) {

	mmc_send32(MMC_XCMD_RREG, 0);
	return (mmc_read(buf, 20)) ;
}

/* display any local buffer
 * BUF: pointer to local buffer
 * N:   number of bytes to display
 */
void mmc_display_buffer(u8 *buf, u16 n) {
	u16 i;

	n = (n>MMC_FLASH_BUF_LEN)?MMC_FLASH_BUF_LEN:n;

	for(i=0; i<n; i++) {
		if (i%16 == 0) xil_printf("\n\r%02x:", i);
		if (i%4  == 0) xil_printf(" ");
		xil_printf("%02x", buf[i]);
	}
}

/* Send unlock code to enable secured commands */
void mmc_unlock() {
	mmc_send32(MMC_SECURE_KEY_WREG,   MMC_SECURE_KEY>>16);
	mmc_send32(MMC_SECURE_KEY_WREG+2, MMC_SECURE_KEY&0xFFFF);
}

/* TODO: copy a file between different section of the FLASH memory */
void copy_flash_file(u8 src_id, u8 dst_id) {

}

/********************** MAIN *************************/
int main()
{
	u32 addr, res;
	u8  rxbuf[256], sector;
	char c;


	xil_printf("Hello World SYS-FPGA (compiled %s on %s)\r\n", __DATE__, __TIME__);

	addr = 0;

	for(;;) {
		/* read current address register */
		addr   = mmc_get_addr();
		sector = addr/FLASH_SECTOR_SIZE;

		xil_printf("\n\n\r");
		xil_printf("    1: Read flash\n\r");
		xil_printf("    2: Erase flash sector %d\n\r", sector);
		xil_printf("    3: Program flash\n\r");
		xil_printf("    4: Run IAP0\n\r");
		xil_printf("    5: Run IAP1\n\r");
		xil_printf("    6: Reset CPU\n\r");
		xil_printf("    7: Read SD card\n\r");
		xil_printf("    8: Write SD card\n\r");
		xil_printf("    9: Timed Shutdown\n\r");
		xil_printf("    A: Set address\n\r");
		xil_printf("    B: Set programming file length\n\r");
		xil_printf("    C: Compute file's CRC\n\r");
		xil_printf("    D: Display MMC's data buffer\n\r");
		xil_printf("\n\rSelect option (current address 0x%08X):\n\r", addr);

		//c = getchar();
		c= inbyte();
		xil_printf("%c\n\r", c);

		switch (c) {
		case '0':
			mmc_get_cmd_regs(rxbuf);
			mmc_display_buffer(rxbuf, 16);
			break;
		case '1': //read flash
			mmc_execute_cmd(MMC_CMD_FREAD);
			res = mmc_get_cmd_res();
			if (res &0xFFFF) {
				xil_printf("Got error 0x%08X while trying to read FLASH\n\r", res);
			} else {
				mmc_get_buffer(rxbuf, MMC_FLASH_BUF_LEN);
				mmc_display_buffer(rxbuf, MMC_FLASH_BUF_LEN);
			}
			break;
		case '2': //erase flash sector
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_FERASE);
			res = mmc_get_cmd_res();
			if (res &0xFFFF) {
				xil_printf("Got error 0x%08X while trying to erase FLASH sector\n\r", res);
			} else {
				xil_printf("DONE\n\r");
			}
			break;
		case '3': //program flash with local buffer's content
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_FPROG);
			res = mmc_get_cmd_res();
			if (res &0xFFFF) {
				xil_printf("Got error 0x%08X while trying to write FLASH\n\r", res);
			} else {
				xil_printf("DONE\n\r");
			}
			break;
		case '4': //IAP0
			xil_printf("System is going down...\n\r");
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_IAP0);
			break;
		case '5': //IAP1
			xil_printf("System is going down...\n\r");
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_IAP1);
			break;
		case '6': //Reset
			xil_printf("System is going down...\n\r");
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_RESET);
			break;
		case '7': //Read SDCard
			mmc_execute_cmd(MMC_CMD_SDREAD);
			res = mmc_get_cmd_res();
			if (res & 0xFFFF) {
				xil_printf("Got error 0x%08X while reading SD card\n\r", res);
			} else {
				mmc_get_buffer(rxbuf, MMC_FLASH_BUF_LEN);
				mmc_display_buffer(rxbuf, MMC_FLASH_BUF_LEN);
			}
			break;
		case '8': //Write SDCard
			if (addr % SD_SECTOR_SIZE) {
				xil_printf("ERROR: address shall be sector-aligned (N*0x200)\n\r"); //TODO remove
			} else {
				mmc_unlock();
				mmc_execute_cmd(MMC_CMD_SDPROG);
				res = mmc_get_cmd_res();
				if (res & 0xFFFF) {
					xil_printf("Got error 0x%08X while writing SD card\n\r", res);
				} else {
					xil_printf("DONE\n\r");
				}
			}
			break;
		case '9': //timed shutdown
			xil_printf("System is going down...\n\r");
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_TSD);
			break;
		case 'A':
			addr = hex_from_console("Address = 0x ",8);
			mmc_set_addr(addr);
			xil_printf("Set address register to 0x%08x\n\r", addr);
			break;
		case 'B': //set file length (info section address calculated automatically)
			res = hex_from_console("File length (max 1MB) = 0x", 6);
			//write file length in MMC's data register
			mmc_set_data(res);
			//Execute command (uses current address register)
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_WRLEN);
			//check result
			res = mmc_get_cmd_res();
			if (res & 0xFFFF) {
				xil_printf("Got error 0x%08X while trying to set file length\n\r", res);
			} else {
				xil_printf("DONE\n\r");
			}
			break;
		case 'C': //compute CRC
			//Execute command (uses current address register)
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_CRC);
			xil_printf("Computing...");
			//check result
			res = mmc_get_cmd_res();
			if (res & 0xFFFF) {
				xil_printf("Got error 0x%08X while trying to conpute CRC\n\r", res);
			} else {
				xil_printf("DONE\n\r");
			}
			break;
		case 'D': //display current MMC buffer
			mmc_get_buffer(rxbuf, MMC_FLASH_BUF_LEN);
			mmc_display_buffer(rxbuf, MMC_FLASH_BUF_LEN);
			break;
		default:
			xil_printf("Unsupported command\n\r");
		}

#if 0
		xil_printf("\n\r   Testing command execution...");
		for (cnt=1; cnt < 11; cnt++ ) {
			mmc_execute_cmd(cnt);
		}
		xil_printf("DONE\n\r");

		xil_printf("\n\r   Writing command registers...");
		mmc_send32( MMC_ADDR_REG,   0x1234 ); //TODO make a function to write address
		mmc_send32( MMC_ADDR_REG+2, 0x5678 );
		mmc_send32( MMC_SECURE_KEY_REG, MMC_SECURE_KEY>>16 ); //TODO make a function to unlock
		mmc_send32( MMC_SECURE_KEY_REG+2, MMC_SECURE_KEY&0xFFFF );
		xil_printf("DONE\n\r");


		xil_printf("\n\r   Reading command registers...");
		mmc_execute_cmd(0);
		mmc_read(rxbuf, 16);
		xil_printf("DONE\n\r");

		for (cnt = 0; cnt < 16; cnt++) {
			xil_printf("%02x ", rxbuf[cnt]);
		}
		xil_printf("\n\r");



		addr = MMC_FLASH_WBUF_ADDR;
		cnt  = 0;
		xil_printf("    writing buffer...");
		for(addr = MMC_FLASH_WBUF_ADDR; addr < (MMC_FLASH_WBUF_ADDR+MMC_FLASH_BUF_LEN); addr+=2) {
			mmc_send32( addr, ((cnt<<8) | (cnt+1)) );
			cnt+=2;
		}
		xil_printf(" DONE\n\r");

		xil_printf("\n\r    reading buffer...");
		mmc_send32( MMC_FLASH_RBUF_ADDR, 0);
		mmc_read(rxbuf, 256);
		xil_printf(" DONE\n\r");

		/* display read data */
		for (cnt = 0; cnt < 256; cnt++) {
			if (cnt%16 == 0) {
				xil_printf("\n\r%04x:", cnt);
			}
			if (cnt%4 == 0) {
				xil_printf(" ");
			}

			xil_printf("%02x", rxbuf[cnt]);
		}
#endif
	} //for(;;)

#if 0
	// Reset MBU via MMC
	mmc_send(6, 'C');
	mmc_send(6, 'M');
	mmc_send(6, 'D');

	xil_printf("Done\r\n");

	while (1)
	{
		gpio_toggle(&led_r_green);
		wait_s(1);
		xil_printf(".");
	}
#endif

	return 0;
}
