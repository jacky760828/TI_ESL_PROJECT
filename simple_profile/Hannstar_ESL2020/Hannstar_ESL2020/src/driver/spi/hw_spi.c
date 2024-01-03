#include "board.h"
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26XXDMA.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include DeviceFamily_constructPath(driverlib/ssi.h)
#include <ti/sysbios/knl/Task.h>

#include "hw_spi.h"
#include "hw_uart.h"
/*********************************************************************
 * LOCAL PARAMETER
 */
SPI_Params      SPIparams;
SPI_Handle      spiHandle;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/****************************************************************
DCX
*****************************************************************/
PIN_State  DCXState;

PIN_Config DCXCfg[] = 
{
  IOID_1   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH  | PIN_PUSHPULL | PIN_DRVSTR_MAX,  
  IOID_4   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_DRVSTR_MAX, 
  IOID_7   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_DRVSTR_MAX, 
  IOID_8   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW  | PIN_PUSHPULL | PIN_DRVSTR_MAX, 
  Board_LCD_CSN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH  | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  Board_SPI_FLASH_CS   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH  | PIN_PUSHPULL | PIN_DRVSTR_MAX, 
  IOID_14   | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_DRVSTR_MAX, 
  PIN_TERMINATE
};
PIN_Handle DCXHandle;

void DCXInit(void)
{
  DCXHandle = PIN_open(&DCXState, DCXCfg);
}

void DCXOSet(uint8_t flag)
{
  PIN_setOutputValue(DCXHandle, IOID_8, flag);
}

void LCD_CSN(uint8_t flag)
{
  PIN_setOutputValue(DCXHandle, Board_LCD_CSN, flag);
}


/********************************************************************
Flash
*********************************************************************/
/* Instruction codes */
#define BLS_CODE_PROGRAM          0x02 /**< Page Program */
#define BLS_CODE_READ             0x03 /**< Read Data */
#define BLS_CODE_READ_STATUS      0x05 /**< Read Status Register */
#define BLS_CODE_WRITE_ENABLE     0x06 /**< Write Enable */
#define BLS_CODE_SECTOR_ERASE     0x20 /**< Sector Erase */
#define BLS_CODE_MDID             0x90 /**< Manufacturer Device ID */

#define BLS_CODE_DP               0xB9 /**< Power down */
#define BLS_CODE_RDP              0xAB /**< Power standby */

/* Bitmasks of the status register */
#define BLS_STATUS_SRWD_BM        0x80
#define BLS_STATUS_BP_BM          0x0C
#define BLS_STATUS_WEL_BM         0x02
#define BLS_STATUS_WIP_BM         0x01

#define BLS_STATUS_BIT_BUSY       0x01 /**< Busy bit of the status register */

/* Part specific constants */
#define BLS_PROGRAM_PAGE_SIZE     256
#define BLS_ERASE_SECTOR_SIZE     4096


static void extFlashSelect(void)
{
    PIN_setOutputValue(DCXHandle,Board_SPI_FLASH_CS,Board_FLASH_CS_ON);
}

static void extFlashDeselect(void)
{
    PIN_setOutputValue(DCXHandle,Board_SPI_FLASH_CS,Board_FLASH_CS_OFF);
}

static int Flash_read(uint8_t *buf, size_t len)
{
    SPI_Transaction masterTransaction;

    masterTransaction.count = len;
    masterTransaction.txBuf = NULL;
    masterTransaction.arg = NULL;
    masterTransaction.rxBuf = buf;

    return SPI_transfer(spiHandle, &masterTransaction) ? 0 : -1;
}


static int Flash_write(const uint8_t *buf, size_t len)
{
    SPI_Transaction masterTransaction;

    masterTransaction.count  = len;
    masterTransaction.txBuf  = (void*)buf;
    masterTransaction.arg    = NULL;
    masterTransaction.rxBuf  = NULL;

    return SPI_transfer(spiHandle, &masterTransaction) ? 0 : -1;
}

static void Flash_flash(void)
{
    /* make sure SPI hardware module is done  */
    while(SSIBusy(((SPICC26XXDMA_HWAttrsV1*)spiHandle->hwAttrs)->baseAddr))
    { };
}

static int ExtFlash_waitReady(void)
{
    const uint8_t wbuf[1] = { BLS_CODE_READ_STATUS };
    int ret;

    /* Throw away all garbage */
    extFlashSelect();
    Flash_flash();
    extFlashDeselect();

    for (;;)
    {
        uint8_t buf;

        extFlashSelect();
        Flash_write(wbuf, sizeof(wbuf));
        ret = Flash_read(&buf,sizeof(buf));
        extFlashDeselect();

        if (ret)
        {
            /* Error */
            return -2;
        }
        if (!(buf & BLS_STATUS_BIT_BUSY))
        {
            /* Now ready */
            break;
        }
    }

    return 0;
}


static bool extFlashPowerStandby(void)
{
    uint8_t cmd;
    bool success;

    cmd = BLS_CODE_RDP;
    extFlashSelect();
    success = Flash_write(&cmd,sizeof(cmd)) == 0;
    extFlashDeselect();

    if(success)
    {
        if (ExtFlash_waitReady() != 0)
        {   
          success = false;
        }
    }

    return success;
}

static int ExtFlash_writeEnable(void)
{
    const uint8_t wbuf[] = { BLS_CODE_WRITE_ENABLE };

    extFlashSelect();
    int ret = Flash_write(wbuf,sizeof(wbuf));
    extFlashDeselect();

    if (ret)
    {
        return -3;
    }
    return 0;
}

static bool ExtFlash_readInfo(void)
{
    const uint8_t wbuf[] = { BLS_CODE_MDID, 0xFF, 0xFF, 0x00 };

    uint8_t infoBuf[2] = {0};
    extFlashSelect();

    int ret = Flash_write(wbuf, sizeof(wbuf));
    if (ret)
    {
        extFlashDeselect();
        return false;
    }

    ret = Flash_read(infoBuf, sizeof(infoBuf));
    extFlashDeselect();

    return ret == 0;
}

bool ExtFlash_open(void)
{
      bool f;
      
      f = extFlashPowerStandby();
      
      if (f)
        {
            /* Verify manufacturer and device ID */
            f = ExtFlash_readInfo();
        }
      return f;
      
}

/* See ExtFlash.h file for description */
bool ExtFlash_read(size_t offset, size_t length, uint8_t *buf)
{
    uint8_t wbuf[4];

    /* Wait till previous erase/program operation completes */
    int ret = ExtFlash_waitReady();
    if (ret)
    {
        return false;
    }

    /* SPI is driven with very low frequency (1MHz < 33MHz fR spec)
    * in this temporary implementation.
    * and hence it is not necessary to use fast read. */
    wbuf[0] = BLS_CODE_READ;
    wbuf[1] = (offset >> 16) & 0xff;
    wbuf[2] = (offset >> 8) & 0xff;
    wbuf[3] = offset & 0xff;

    extFlashSelect();

    if (Flash_write(wbuf, sizeof(wbuf)))
    {
        /* failure */
        extFlashDeselect();
        return false;
    }

    ret = Flash_read(buf, length);

    extFlashDeselect();

    return ret == 0;
}

/* See ExtFlash.h file for description */
bool ExtFlash_erase(size_t offset, size_t length)
{
    /* Note that Block erase might be more efficient when the floor map
    * is well planned for OTA but to simplify for the temporary implementation,
    * sector erase is used blindly. */
    uint8_t wbuf[4];
    size_t i, numsectors;

    wbuf[0] = BLS_CODE_SECTOR_ERASE;

    {
        size_t endoffset = offset + length - 1;
        offset = (offset / BLS_ERASE_SECTOR_SIZE) * BLS_ERASE_SECTOR_SIZE;
        numsectors = (endoffset - offset + BLS_ERASE_SECTOR_SIZE - 1) /
            BLS_ERASE_SECTOR_SIZE;
    }

    for (i = 0; i < numsectors; i++)
    {
        /* Wait till previous erase/program operation completes */
        int ret = ExtFlash_waitReady();
        if (ret)
        {
            return false;
        }

        ret = ExtFlash_writeEnable();
        if (ret)
        {
            return false;
        }

        wbuf[1] = (offset >> 16) & 0xff;
        wbuf[2] = (offset >> 8) & 0xff;
        wbuf[3] = offset & 0xff;

        extFlashSelect();

        if (Flash_write(wbuf, sizeof(wbuf)))
        {
            /* failure */
            extFlashDeselect();
            return false;
        }
        extFlashDeselect();

        offset += BLS_ERASE_SECTOR_SIZE;
    }

    return true;
}

/* See ExtFlash.h file for description */
bool ExtFlash_write(size_t offset, size_t length, const uint8_t *buf)
{
    uint8_t wbuf[4];

    while (length > 0)
    {
        /* Wait till previous erase/program operation completes */
        int ret = ExtFlash_waitReady();
        if (ret)
        {
            return false;
        }

        ret = ExtFlash_writeEnable();
        if (ret)
        {
            return false;
        }

        size_t ilen; /* interim length per instruction */

        ilen = BLS_PROGRAM_PAGE_SIZE - (offset % BLS_PROGRAM_PAGE_SIZE);
        if (length < ilen)
        {
            ilen = length;
        }

        wbuf[0] = BLS_CODE_PROGRAM;
        wbuf[1] = (offset >> 16) & 0xff;
        wbuf[2] = (offset >> 8) & 0xff;
        wbuf[3] = offset & 0xff;

        offset += ilen;
        length -= ilen;

        /* Up to 100ns CS hold time (which is not clear
        * whether it's application only in between reads)
        * is not imposed here since above instructions
        * should be enough to delay
        * as much. */
        extFlashSelect();

        if (Flash_write(wbuf, sizeof(wbuf)))
        {
            /* failure */
            extFlashDeselect();
            return false;
        }

        if (Flash_write(buf,ilen))
        {
            /* failure */
            extFlashDeselect();
            return false;
        }
        buf += ilen;
        extFlashDeselect();
    }

    return true;
}


bool IsSaveAnimat = false;
uint8_t RunAnimat = 0;
uint8_t ACounts = 11;

void saveAnimat()
{
  FlashOff(0x01);
  uint8_t RData[256] = {0};
  bool flag = ExtFlash_read( 0x000000 , 256, RData);
  
  RData[0] = RunAnimat;
  RData[1] = ACounts;

  if(flag)
     flag = ExtFlash_erase(0x000000 ,4096);  
  if(flag)
     flag = ExtFlash_write(0x000000, 256, RData);

  IsSaveAnimat = false;

  FlashOff(0x00);
}

void DrawPicture(size_t FlashAddress, int CommandSize)
{
#ifdef LCD_Reverse   
  uint8_t Data[256] = {0};
#endif  
  uint8_t RData[256] = {0};
  bool flag;

  for(int i=0 ; i < (CommandSize/256);i ++)
  {
      flag = ExtFlash_read( FlashAddress + i * 256, 256, RData);

      if(flag)
      {            
#ifdef LCD_Reverse   
          for(int j=0;j<256;j+=2)        
          {
          
            Data[j] = ~RData[j];    
            Data[j+1] = ~RData[j+1];    
            if((RData[j]&0xA0) ==0xA0)    
              Data[j] |= 0xA0;     
            else if((RData[j]&0xA0) == 0x00)
              Data[j] &= ~0xA0;     

            if((RData[j]&0x50) ==0x50)
              Data[j] |= 0x50;
            else if((RData[j]&0x50) ==0x00)
              Data[j] &= ~0x50;
                  
            if(((RData[j]&0x08) ==0x08) && ((RData[j+1]&0x80) ==0x80))          
            {    
              Data[j] |= 0x08;     
              Data[j+1] |= 0x80;          
            }
            else if(((RData[j]&0x08) ==0x00) && ((RData[j+1]&0x80) ==0x00))          
            {    
              Data[j] &= ~0x08;     
              Data[j+1] &= ~0x80;          
            }
 
            
            if(((RData[j]&0x04) ==0x04) && ((RData[j+1]&0x40) ==0x40))    
            {    
              Data[j] |= 0x04;      
              Data[j+1] |= 0x40;          
            }
            else if(((RData[j]&0x04) ==0x00) && ((RData[j+1]&0x40) ==0x00))    
            {    
              Data[j] &= ~0x04;      
              Data[j+1] &= ~0x40;          
            }
                  
            
            if((RData[j+1]&0x28) ==0x28)    
              Data[j+1] |= 0x28;      
            else if((RData[j+1]&0x28) ==0x00)    
              Data[j+1] &= ~0x28;      
            if((RData[j+1]&0x14) ==0x14)
              Data[j+1] |= 0x14;      
            else if((RData[j+1]&0x14) ==0x00)
              Data[j+1] &= ~0x14;      

          }
           WriteData7302Ex(Data,Data, 256);
#else
          WriteData7302Ex(RData,RData, 256);
#endif 
          
    }    
  }  
     
  flag = ExtFlash_read( FlashAddress + CommandSize - CommandSize%256  , CommandSize%256, RData);     
      
  if(flag)  
  {
#ifdef LCD_Reverse       
    for(int j=0;j<CommandSize%256;j+=2)        
    {
    
      Data[j] = ~RData[j];    
      Data[j+1] = ~RData[j+1];    
      if((RData[j]&0xA0) ==0xA0)    
        Data[j] |= 0xA0;      
      if((RData[j]&0x50) ==0x50)
        Data[j] |= 0x50;
            
      if(((RData[j]&0x08) ==0x08) && ((RData[j+1]&0x80) ==0x80))          
      {    
        Data[j] |= 0x08;     
        Data[j+1] |= 0x80;          
      }
      
      if(((RData[j]&0x04) ==0x04) && ((RData[j+1]&0x40) ==0x40))    
      {    
        Data[j] |= 0x04;      
        Data[j+1] |= 0x40;          
      }
            
      
      if((RData[j+1]&0x28) ==0x28)    
        Data[j+1] |= 0x28;      
      if((RData[j+1]&0x14) ==0x14)
        Data[j+1] |= 0x14;      
    }
    
    WriteData7302Ex(Data,Data, CommandSize%256);    
#else 
    WriteData7302Ex(RData,RData, CommandSize%256);
#endif
  }  
}

void RunWarning(void)
{
#ifdef ESL_213
    WriteIR(0x3A); 
    WriteData7302(0x11); 
    
    WriteIR(0x2A); 
    WriteData7302(0x19); 
    WriteData7302(0x23); 

    WriteIR(0x2B); 
    WriteData7302(0x00); 
    WriteData7302(0x7C); 

    WriteIR(0x2C); 

   // DrawPicture(PICTURE1_ADDRESS,4125);
#else 
    WriteIR(0x2A);
    WriteData7302(0x05);
    WriteData7302(0x36);  
  
    WriteIR(0x2B);
    WriteData7302(0x00);
    WriteData7302(0xC7);  
  
    WriteIR(0x2C);       
   
   // DrawPicture(PICTURE1_ADDRESS,PICTURE_CMD_SIZE);
#endif
}

/*********************************************************************
 * @fn      GY_SpiTask_Init
 *
 * @brief   SPI任务初始化及启动
 *
 * @param   spiIndex -> 0:Board_SPI0 | 1:Board_SPI1
 *          spiMode ->  0:SPI_MASTER | 1:SPI_SLAVE
 *
 * @return  None.
 */
void HwSPIInit(void)
{  
  DCXInit();
  PIN_setOutputValue(DCXHandle, IOID_1, 1);
  PIN_setOutputValue(DCXHandle, IOID_7, 1);
  PIN_setOutputValue(DCXHandle, IOID_14, 1);
  PIN_setOutputValue(DCXHandle, IOID_4, 1);

  
  SPI_init();
  SPI_Params_init(&SPIparams);
  SPIparams.bitRate  = 10000000;  
  //1MHz
  SPIparams.dataSize = 8; 
  SPIparams.frameFormat = SPI_POL0_PHA0;           //相位0极性0
  SPIparams.mode = SPI_MASTER;                     //SPI主从模式
  SPIparams.transferCallbackFxn = NULL;
  SPIparams.transferMode = SPI_MODE_BLOCKING;      //阻塞
  SPIparams.transferTimeout = SPI_WAIT_FOREVER;

  spiHandle = SPI_open(CC2640R2_LAUNCHXL_SPI0, &SPIparams);
}

//============================================
//LCD

void WriteIR(uint8_t IR_Address)
{
  //uint8_t csnPin = Board_LCD_CSN;
     uint8_t txbuf[1] = {IR_Address};
    SPI_Transaction spiTransaction;
    spiTransaction.arg = NULL;
    spiTransaction.count = 1;
    spiTransaction.txBuf = &txbuf;
    spiTransaction.rxBuf = &txbuf;
    
    LCD_CSN(0);
    SPI_transfer(spiHandle, &spiTransaction);
    SPI_transferCancel(spiHandle);
    LCD_CSN(1);
}

void WriteData7302(uint8_t IR_Address)
{
     uint8_t txbuf[1] = {IR_Address};
    SPI_Transaction spiTransaction;
    spiTransaction.arg = NULL;
    spiTransaction.count = 1;
    spiTransaction.txBuf = &txbuf;
    spiTransaction.rxBuf = &txbuf;
    
    DCXOSet(1);
    LCD_CSN(0);
    SPI_transfer(spiHandle, &spiTransaction);
    SPI_transferCancel(spiHandle);
    LCD_CSN(1);
    DCXOSet(0);
}


void WriteData7302Ex(uint8_t *txbuf, uint8_t *rxbuf ,uint16_t len)
{
  DCXOSet(1);
  
  SPI_Transaction spiTransaction;
  spiTransaction.arg = NULL;
  spiTransaction.count = len;
  spiTransaction.txBuf = txbuf;
  spiTransaction.rxBuf = rxbuf;

 
  LCD_CSN(0);
  SPI_transfer(spiHandle, &spiTransaction);
  SPI_transferCancel(spiHandle);
  LCD_CSN(1);
  DCXOSet(0);
  
}

void Init_Panel(void)
{
  
  //  Task_sleep(100000);

//    PIN_setOutputValue(DCXHandle, Hannstar_LCD_RSTB, 0);
//    Task_sleep(10000);
//    PIN_setOutputValue(DCXHandle, Hannstar_LCD_RSTB, 1);
//    Task_sleep(10000);


    //Enable OTP.
    WriteIR(0xEB);
    WriteData7302(0x02);
    
    //OTP Load Control.
    WriteIR(0xD7);
    WriteData7302(0x68);
    
    //Auto Power Control.
    WriteIR(0xD1);
    WriteData7302(0x01);

    //Gate Voltage Setting VGH=12V; VGL = 5V.
    WriteIR(0xC0);
    WriteData7302(0x80);

    //VSH Setting VSH=4.65.
    WriteIR(0xC1);
    WriteData7302(0x1A);
    WriteData7302(0x1A);
    WriteData7302(0x1A);
    WriteData7302(0x1A);
    WriteData7302(0x14);
    WriteData7302(0x00);

#ifdef ESL_213
    //VSL Setting VSL=0.
    WriteIR(0xC2);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    
    //VCOMH Setting.
    WriteIR(0xCB);
    WriteData7302(0x14);
   
#else        
//4.2蝗は    

    //VSL Setting VSL=0.
    WriteIR(0xC2);
    WriteData7302(0x06);
    WriteData7302(0x06);
    WriteData7302(0x06);
    WriteData7302(0x06);
    
    //VCOMH Setting.
    WriteIR(0xCB);
    WriteData7302(0x0C);
#endif   
    //Gete EQ Setting HPM EQ LPM EQ.
    WriteIR(0xB4);
    WriteData7302(0xE5);
    WriteData7302(0x77);
    WriteData7302(0xF1);
    WriteData7302(0xFF);
    WriteData7302(0xFF);
    WriteData7302(0x4F);
    WriteData7302(0xF1);
    WriteData7302(0xFF);
    WriteData7302(0xFF);
    WriteData7302(0x4F);

    //Sleep out.
    WriteIR(0x11);

    //Delay 100ms.
    Task_sleep(10000);

    //OSC Setting LPM.
    WriteIR(0xC7);
    WriteData7302(0xA6);
    WriteData7302(0xE9);
    
    //Duty Setting.
    WriteIR(0xB0);
    //250duty/4=63.
#ifdef ESL_213
    WriteData7302(0x3F);
#else 
    WriteData7302(0x64);
#endif

#ifdef ESL_213
    //Memory Data Access Control.
    WriteIR(0x36);
    // Cmd36 == 0x00 タ盽  0x4C ?辊? 
    WriteData7302(0x00);
#else 

	#ifdef LCD_Reverse    
	    //Memory Data Access Control.
	    WriteIR(0x36);
	    WriteData7302(0x4C);
	#else    
	    WriteIR(0x36);
	    // Cmd36 == 0x00 タ盽  0x4C 棵辊腁 
	    WriteData7302(0x00);
	#endif
    
#endif
    //Data Format Select 4 write for 24bit.
    WriteIR(0x3A);
    WriteData7302(0x10);

    //Source Setting.
    WriteIR(0xB9);
    //23:mono 03:4-Gamma.
    WriteData7302(0x23);

    //Panel Setting Frame inversion
    WriteIR(0xB8);
    WriteData7302(0x09);
   
    //Column Adrress Setting S61~S182
    WriteIR(0x2A);
#ifdef ESL_213
    WriteData7302(0x19);
    WriteData7302(0x2C);
#else 
    WriteData7302(0x05);
    WriteData7302(0x36);
#endif    
    //Row Address Stting G1~G250
    WriteIR(0x2B);
    WriteData7302(0x00);
    WriteData7302(0xC7);
    

    WriteIR(0xD0);
    WriteData7302(0x1F);

    WriteIR(0x29); //Display on.

    //enable CLR RAM
    WriteIR(0xB9);
    WriteData7302(0xE3);
 
    //Delay 100ms.
    Task_sleep(10000);

    //enable CLR RAM
    WriteIR(0xB9);
    WriteData7302(0x23);
    
    WriteIR(0x72);
    WriteData7302(0x00); //Destree OFF.
#ifdef ESL_213
#else     
    WriteIR(0xB3);
    WriteData7302(0x94); //Destree OFF.
#endif
    WriteIR(0x39); //LPM
  
    //0.25Hz
    WriteIR(0xB2);
    WriteData7302(0x00); 
    WriteData7302(0x00); 
    
    RunWarning();
}

void LCDPowerOnOff(uint8_t PowerOn)
{
  //do
 // {
      //PIN_setOutputValue(DCXHandle, Hannstar_LCD_Power, 0);
      //PIN_setOutputValue(DCXHandle, Hannstar_Flash_Power, 0);
      //Task_sleep(500000);
 /* if(PowerOn == 0x00)
  {
    
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Power, 1);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_CS, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Hold, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_WP, 0);
      
      

      PIN_setOutputValue(DCXHandle, Hannstar_LCD_Power, 1);
      PIN_setOutputValue(DCXHandle, Hannstar_LCD_CSB, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_LCD_AO, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_LCD_RSTB, 0);
  }
  else
  {
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Hold, 1);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_WP, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_LCD_Power, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Power, 0);

      Init_Panel();
  }
      //  Task_sleep(500000);

    
*/
}

void FlashOff(uint8_t PowerOn)
{
  /*
  if(PowerOn == 0x00)
  {
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Power, 1);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_CS, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Hold, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_WP, 0);
      
      PIN_setOutputValue(DCXHandle, Hannstar_LCD_CSB, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_LCD_AO, 0);
//      PIN_setOutputValue(DCXHandle, Hannstar_LCD_RSTB, 0);
  }
    else
  {
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Hold, 1);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_WP, 0);
//      PIN_setOutputValue(DCXHandle, Hannstar_LCD_Power, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_Power, 0);
      PIN_setOutputValue(DCXHandle, Hannstar_Flash_CS, 1);
  }
*/
}

void SleepLCM(uint8_t PowerOn)
{

 if(PowerOn == 0x00)
  {
    //Gate Voltage Setting VGH=12V; VGL = 5V.
    WriteIR(0xC0);
    WriteData7302(0x00);

    //VSH Setting VSH=4.65.
    WriteIR(0xC1);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);

    //VSL Setting VSL=0.
    WriteIR(0xC2);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
  }
 else
 {
    //Gate Voltage Setting VGH=12V; VGL = 5V.
    WriteIR(0xC0);
    WriteData7302(0x80);

    //VSH Setting VSH=4.65.
    WriteIR(0xC1);
    WriteData7302(0x28);
    WriteData7302(0x28);
    WriteData7302(0x28);
    WriteData7302(0x28);
    WriteData7302(0x14);
    WriteData7302(0x00);

    //VSL Setting VSL=0.
    WriteIR(0xC2);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
    WriteData7302(0x00);
 }
}


//=============================================================
uint8_t HannsterTaskStack[1024];
Task_Struct HannstarTask;
void Hannstar_taskFxn(UArg a0, UArg a1);

void Hannstar_createTask(void)
{
  //HwADCInit();
  Task_Params taskParams;

  // Configure task
  Task_Params_init(&taskParams);
  taskParams.stack = HannsterTaskStack;
  taskParams.stackSize = 1024;
  taskParams.priority = 1;

  Task_construct(&HannstarTask, Hannstar_taskFxn, &taskParams, NULL);
}

void Hannstar_taskFxn(UArg a0, UArg a1)
{
  uint8_t SelectIndex = 0x00;
  Task_sleep(100000);
	
  GetStatus();
  
  while (1) /* Run loop forever (unless terminated) */
  {
    if(IsSaveAnimat)
    {
      saveAnimat();
      SelectIndex = 0;
    }      
    else if(RunAnimat != 0x00)
    {   
      WriteIR(0x2C); 
      DrawPicture((0x000000 + 4096*10) + (SelectIndex * 4096 * 10), PICTURE_CMD_SIZE);

      if(ACounts == 0x00)
        SelectIndex = 0;
      else if(SelectIndex != (ACounts - 1))
        SelectIndex++;
      else 
        SelectIndex = 0x00;
      
        Task_sleep(200000);
        //Task_sleep(3000000);
     }
     else 
       Task_sleep(200000);
  }
}

void setFirstP(uint8_t bRun)
{
  uint8_t RData[256] = {0};
  bool flag = ExtFlash_read( 0x000000 , 256, RData);
  
  RData[2] = bRun;
  if(flag)
      flag = ExtFlash_erase(0x000000 ,4096);  
  if(flag)
      flag = ExtFlash_write(0x000000, 256, RData);
}

void setRunAnimation(uint8_t bRun, uint8_t bCounts)
{
  RunAnimat = bRun;
  ACounts = bCounts;  
  IsSaveAnimat = true;   
}

void GetStatus()
{
   uint8_t RData[256] = {0};
   bool flag = ExtFlash_read( 0x000000 , 256, RData);
         
   RunAnimat = RData[0];
   ACounts = RData[1];
   
   WriteIR(0x2C);
   DrawPicture(PICTURE1_ADDRESS + (RData[2] * 4096 * 10), PICTURE_CMD_SIZE);
  
   FlashOff(0x00);
}

