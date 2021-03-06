/*
 * Author : Alessandro Malatesta
 * Date   : 06 April 2017
 * Info   : Debug application used to test the GPAC3 specific I2C commands for the MMC
 */

#include <stdio.h>
#include "xil_printf.h"
#include "xiic_l.h"
#include <assert.h>
#include "gpio.h"

#define MMC_I2C_ADDR7 0x3E //MMC's I2C address (7-bits)

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
#define FLASH_SECTOR_SIZE   0x10000 //64KiB: minimum erasable size in MMC's FLASH
#define FLASH_INFO_ID       15 //ID of 1MB Flash section reserved to store info on other 1MB sections
#define FLASH_FILE_SIZE     0x100000 //FLASH is divided in 1MiB sections, one file per section
#define SD_SECTOR_SIZE      0x200 //SD card sector size. If only part of a sector is written, all the rest is erased.
/* MMC command codes */
#define MMC_CMD_NULL   0x0000 //no effect. can be used to read back the whole command register seciton
#define MMC_CMD_FREAD  0x0001 //read 256B from FLASH's address stored in MMC_ADDR_REG. Data is stored at FLASH_RBUF_ADDR
#define MMC_CMD_FERASE 0x0002 //erase a 64K FLASH sector starting from address MMC_ADDR_REG. Address shall be 64K aligned (n*0x10000)
#define MMC_CMD_FPROG  0x0003 //Programs 256B into flash FLASH at address stored in MMC_ADDR_REG. Data is taken from FLASH_WBUF_ADDR
#define MMC_CMD_IAP0   0x0004 //Executes IAP (MMC FW update) using firmware tored in FLASH at address 0x0
#define MMC_CMD_IAP1   0x0005 //Executes IAP (MMC FW update) using firmware tored in FLASH at address 0x100000
#define MMC_CMD_RESET  0x0006 //Resets MMC's CPU (causes a power cycle)
#define MMC_CMD_SDREAD 0x0007 //read 256B from SDcard's address stored in MMC_ADDR_REG. Data is stored in FLASH_RBUF_ADDR
#define MMC_CMD_SDPROG 0x0008 //write 256B to SDcard's address stored in MMC_ADDR_REG. Data is taken from FLASH_WBUF_ADDR
#define MMC_CMD_TSD    0x0009 //Execute timed shutdown (switch off power supply for 5 seconds, then switch on again)
#define MMC_CMD_WRLEN  0x000A //Write file length. Uses the file address from the ADDR register, and the length from the DATA register.
#define MMC_CMD_CRC    0x000B //Compute CRC on file. Uses current address register to select file, and stored file length.

#define buf8_to_16(x) ((x[0]<<8) | x[1])
#define buf8_to_32(x) ((x[0]<<24) | (x[1]<<16) | (x[2]<<8) | x[3] )
#define info_addr(x)  ((FLASH_INFO_ID * FLASH_FILE_SIZE) + (x * FLASH_SECTOR_SIZE))

char inbyte(void); //jist the declaration. to make the compile happy


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
 *
 * MSG : string printed when asking for user input
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
 *  ADDR: 1st and 2nd byte in payload (MSB first)
 *  DATA: 3rd and 4th byte in payload (MSB first)
 *
 *  I2C transaction: START | 0x7C | ADDR(15:8) | ADDR(7:0) | DATA(15:8) | DATA(7:0) | (ACK) | STOP
 *
 *  returns: the number of bytes sent (shall be always 4)
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
 *  RXBUF: buffer used to store the received data
 *  N    : number of bytes requested
 *
 *  The read address shall be set previously with a write transaction (either 32 or 16 bit)
 *
 *  returns: the number of bytes received (shall be N)
 */
unsigned mmc_read(u8 *rxbuf, u16 n) {

	return( XIic_Recv(XPAR_AXI_IIC_0_BASEADDR, MMC_I2C_ADDR7, rxbuf, n, XIIC_STOP) );

}

/* Execute GPAC3 command
 *  Writes the CMD argument to address MMC_XCMD_REG (0x800)
 *  The command outcome shall be always checked by reading the RESULT register with mmc_get_cmd_res()
 *
 *  returns: the number of bytes sent (shall be 4)
 */
unsigned mmc_execute_cmd(u16 cmd) {

	return( mmc_send32(MMC_XCMD_WREG, cmd) );

}

/* write MMC address register
 *  ADDR: 32-bit address
 *
 *  returns: number of bytes sent (shall be 8)
 */
unsigned mmc_set_addr(u32 addr) {
	unsigned ret = 0;

	ret += mmc_send32(MMC_ADDR_WREG, addr>>16);
	ret += mmc_send32(MMC_ADDR_WREG+2, addr&0xFFFF);

	return ret;
}

/* get current address register stored in MMC
 *
 *  returns: 32-bit address read from MMC's command address register
 */
u32 mmc_get_addr(void) {
	u8 rxbuf[4];

	mmc_send32(MMC_ADDR_RREG, 0);
	mmc_read(rxbuf, 4);

	return (buf8_to_32(rxbuf));
}

/* write MMC data register
 *  DATA: 32-bit value to be written
 *
 *  returns: nothing
 */
void mmc_set_data(u32 data) {
	mmc_send32(MMC_DATA_WREG, data>>16);
	mmc_send32(MMC_DATA_WREG+2, data&0xFFFF);
}

/* get current data register stored in MMC
 *
 *  returns: 32-bit value read from MMC's command data register
 */
u32 mmc_get_data(void) {
	u8 rxbuf[4];

	mmc_send32(MMC_DATA_RREG, 0);
	mmc_read(rxbuf, 4);

	return (buf8_to_32(rxbuf));
}

/* get command result register stored in MMC
 *  The register's 2 MSB contain the code of the last executed command,
 *  the 2 LSB contain the command's result code
 *
 *  returns: 32-bit value read from MMC's command result register
 */
u32 mmc_get_cmd_res(void) {
	u8 rxbuf[4];

	mmc_send32(MMC_CMD_RESULT_RREG, 0);
	mmc_read(rxbuf, 4);

	return (buf8_to_32(rxbuf));
}


/* Get data buffer from MMC
 *  BUF: local buffer where read data will be stored. Shall be able to contain at least N bytes
 *  N  : number of bytes to read (maximum 256)
 *
 *  returns: number of bytes read (shall be N)
 */
unsigned mmc_get_buffer(u8 *buf, u16 n) {

	n = (n>MMC_FLASH_BUF_LEN)?MMC_FLASH_BUF_LEN:n;

	mmc_send32(MMC_FLASH_RBUF_ADDR, 0);
	return (mmc_read(buf, n)) ;
}

/* Write contant of data buffer in MMC
 *  BUF: local buffer containing data to be written. Shall contain at least N bytes
 *  N  : number of bytes to write (maximum 256)
 *
 *  returns: number of bytes written (shall be N)
 */
unsigned mmc_set_buffer(u8 *buf, u16 n) {

	unsigned ret = 0, i;

	n = (n>MMC_FLASH_BUF_LEN)?MMC_FLASH_BUF_LEN:n;

	for (i=0; i<n; i+=2) {
		ret += mmc_send32(i+MMC_FLASH_WBUF_ADDR, buf8_to_16((buf+i)) );
	}

	return (ret);
}

/* get all command registers from MMC
 *  BUF: buffer where the read data is stored (shall be able to contain at least 20B
 *
 *  returns: number of bytes read (shall be 20)
 */
unsigned mmc_get_cmd_regs(u8 *buf) {

	mmc_send32(MMC_XCMD_RREG, 0);
	return (mmc_read(buf, 20)) ;
}

/* display any local buffer on UART console
 *  BUF: pointer to local buffer
 *  N:   number of bytes to display
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

/* Send unlock code to enable secured commands
 *  this function shall be executed before sending any of the protected commands
 *
 *  returns: nothing
 */
void mmc_unlock() {
	mmc_send32(MMC_SECURE_KEY_WREG,   MMC_SECURE_KEY>>16);
	mmc_send32(MMC_SECURE_KEY_WREG+2, MMC_SECURE_KEY&0xFFFF);
}

/* copy a file between different section of the FLASH memory
 *  SRC_ID: ID of source file. File's address is calculated as 0x100000*ID. File size is fixed to 1MiB.
 *  DST_ID: ID of destination file. File's address is calculated as 0x100000*ID. File size is fixed to 1MiB.
 *          WARNING: This file will be erased and overwritten.
 *
 * returns: 0 on success, 1 on failure
 */
int mmc_flash_file_copy(u8 src_id, u8 dst_id) {
	u32 src_adr, dst_adr, file_size, file_crc, res;
	u16 nbuffers, i, progress;
	u8  rxbuf[256];

	/* parameter checks */
	if (src_id > 14 || dst_id > 14) {
		xil_printf("copy_flash_file()::ERROR::Maximum allowed ID is 14\n\r");
		return 1;
	} else if (src_id == dst_id) {
		xil_printf("copy_flash_file()::ERROR::Source ID shall be different from destination ID\n\r");
		return 1;
	}

	/* compute addresses */
	src_adr = src_id * FLASH_FILE_SIZE;
	dst_adr = dst_id * FLASH_FILE_SIZE;

	/* get file size and CRC */
	mmc_set_addr( info_addr(src_id) );
	mmc_execute_cmd(MMC_CMD_FREAD);
	res = mmc_get_cmd_res();
	if ( res & 0xFFFF) {
		xil_printf("copy_flash_file()::ERROR::Cannot read source file info (error 0x%08X)\n\r", res);
		return 1;
	}
	mmc_get_buffer(rxbuf, 12);
	file_size = buf8_to_32((rxbuf+4));
	file_crc  = buf8_to_32((rxbuf+8));

	/* compute number of buffers to write */
	nbuffers = file_size / MMC_FLASH_BUF_LEN;
	if (file_size % MMC_FLASH_BUF_LEN) nbuffers++; //one more if file is not an integer multiple of MMC_FLASH_BUF_LEN
	xil_printf("copy_flash_file()::INFO::file_size = 0x%08X (%d buffers), CRC = 0x%08X\n\r", file_size, nbuffers, file_crc);

	/* read all buffers and write them to destination address */
	for(i = 0; i < nbuffers; i++) {
		progress = (100*(u32)i)/nbuffers;
		xil_printf("\rProgress: %03d%%", progress);

		/* read current buffer */
		mmc_set_addr( src_adr );
		mmc_execute_cmd(MMC_CMD_FREAD); //read from FLASH into MMC's buffer
		res = mmc_get_cmd_res();
		if ( res & 0xFFFF) {
			xil_printf("\n\rcopy_flash_file()::ERROR::Could not read FLASH address 0x%08X\n\r", src_adr);
			return 1;
		}

		/* load MMC's read buffer into local buffer */
		mmc_get_buffer(rxbuf, MMC_FLASH_BUF_LEN);
		/* write read buffer to MMC's write buffer */
		mmc_set_buffer(rxbuf, MMC_FLASH_BUF_LEN);

		/* write buffer to destination */
		mmc_set_addr( dst_adr );

		//If address is a start-of-sector, then erase sector before writing
		if ((dst_adr % FLASH_SECTOR_SIZE) == 0) {
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_FERASE);
			res = mmc_get_cmd_res();
			if ( res & 0xFFFF) {
				xil_printf("\n\rcopy_flash_file()::ERROR::Could not erase FLASH sector 0x08X\n\r", src_adr);
				return 1;
			}
		}

		//Perform FLASH write
		mmc_unlock();
		mmc_execute_cmd(MMC_CMD_FPROG); //use previously read buffer
		res = mmc_get_cmd_res();
		if ( res & 0xFFFF) {
			xil_printf("\n\rcopy_flash_file()::ERROR::Could not write FLASH address 0x08X\n\r", dst_adr);
			return 1;
		}

		/* increment addresses for next buffer */
		src_adr += MMC_FLASH_BUF_LEN;
		dst_adr += MMC_FLASH_BUF_LEN;
	}
	xil_printf("\n\r");


	/* write file size */
	dst_adr = dst_id * FLASH_FILE_SIZE;
	mmc_set_addr( dst_adr );
	mmc_set_data(file_size);
	mmc_unlock();
	mmc_execute_cmd(MMC_CMD_WRLEN);
	res = mmc_get_cmd_res();
	if ( res & 0xFFFF) {
		xil_printf("copy_flash_file()::ERROR::Could not write destination file size\n\r");
		return 1;
	}

	/* compute CRC */
	xil_printf("copy_flash_file()::INFO::Computing CRC...");
	mmc_unlock();
	mmc_execute_cmd(MMC_CMD_CRC);
	res = mmc_get_cmd_res();
	if ( res & 0xFFFF) {
		xil_printf("ERROR::Could not compute destination file's CRC\n\r");
		return 1;
	} else {
		xil_printf("DONE\n\r");
	}

	/* get computed CRC */
	mmc_set_addr( info_addr(dst_id) );
	mmc_execute_cmd(MMC_CMD_FREAD);
	res = mmc_get_cmd_res();
	if ( res & 0xFFFF) {
		xil_printf("copy_flash_file()::ERROR::Cannot read destination file info (error 0x%08X)\n\r", res);
		return 1;
	}
	mmc_get_buffer(rxbuf, 12);

	/* compare source vs destination CRCs */
	if (file_crc == buf8_to_32((rxbuf+8))) {
		xil_printf("copy_flash_file()::INFO::CRC check successful\n\r");
	} else {
		xil_printf("copy_flash_file()::ERROR::Destination file's CRC does not match the source file's CRC\n\r");
		return 1;
	}

	return 0;
}

/********************** MAIN *************************/
int main()
{
	u32 addr, res;
	u8  rxbuf[256], sector;
	char c, src, dst;

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
		xil_printf("    4: Run IAP0 (causes GPAC shutdown)\n\r");
		xil_printf("    5: Run IAP1 (causes GPAC shutdown)\n\r");
		xil_printf("    6: Reset CPU (causes GPAC shutdown)\n\r");
		xil_printf("    7: Read SD card\n\r");
		xil_printf("    8: Write SD card\n\r");
		xil_printf("    9: Timed Shutdown (causes GPAC shutdown)\n\r");
		xil_printf("    A: Set address\n\r");
		xil_printf("    B: Set programming file length\n\r");
		xil_printf("    C: Compute file's CRC\n\r");
		xil_printf("    D: Display MMC's data buffer\n\r");
		xil_printf("    E: File copy\n\r");
		xil_printf("\n\rSelect option (Address Register = 0x%08X):\n\r", addr);

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
			mmc_unlock();
			mmc_execute_cmd(MMC_CMD_SDPROG);
			res = mmc_get_cmd_res();
			if (res & 0xFFFF) {
				xil_printf("Got error 0x%08X while writing SD card\n\r", res);
			} else {
				xil_printf("DONE\n\r");
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
		case 'E': //file copy between different FLASH sectors
			src = hex_from_console("Enter id of source file      (0x0-0xE) = 0x", 1);
			dst = hex_from_console("Enter id of destination file (0x0-0xE) = 0x", 1);
			xil_printf("Are you sure you want to copy file at FLASH address 0x%08X over file at address 0x%08X (y/N)?", src*FLASH_FILE_SIZE, dst*FLASH_FILE_SIZE);
			res = inbyte();
			if (res == 'y') {
				xil_printf("\n\r");
				mmc_flash_file_copy(src,dst);
			} else {
				xil_printf("Copy aborted\n\r");
			}
			break;
		default:
			xil_printf("Unsupported command\n\r");
		}

	} //for(;;)


	return 0;
}
