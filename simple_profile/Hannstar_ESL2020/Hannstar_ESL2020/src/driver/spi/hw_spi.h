#ifndef SERIAL_SPI_H
#define SERIAL_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <ti/drivers/SPI.h>
  
//#define ESL_213

#define PICTURE1_ADDRESS  1 * 4096 * 10

#ifdef ESL_213
  #define PICTURE_CMD_SIZE 4125
#else 
  #define PICTURE_CMD_SIZE 40000
#endif
/*****************************************************
 * SPI任务初始化
*/  
void HwSPIInit(void);  

/*****************************************************
 * SPI通信函数
*/ 
//void HwSPITrans(uint8_t csnPin, uint8_t *txbuf, uint8_t *rxbuf ,uint16_t len);

#define LCD_Reverse
   
void DCXOSet(uint8_t flag);
void WriteIR(uint8_t IR_Address);
void WriteData7302(uint8_t IR_Address);
void WriteData7302Ex(uint8_t *txbuf, uint8_t *rxbuf ,uint16_t len);
void Init_Panel(void);

bool ExtFlash_open(void);
bool ExtFlash_erase(size_t offset, size_t length);
bool ExtFlash_read(size_t offset, size_t length, uint8_t *buf);
bool ExtFlash_write(size_t offset, size_t length, const uint8_t *buf);

void DrawPicture(size_t FlashAddress, int CommandSize);
void FlashTest(void);

void LCDPowerOnOff(uint8_t PowerOn);
void FlashOff(uint8_t PowerOn);
void SleepLCM(uint8_t PowerOn);

void Hannstar_createTask(void);
void setRunAnimation(uint8_t bRun, uint8_t bCounts);
void GetStatus();
void InternalFlash_Program(uint32_t index, uint8_t *buf);
void InternalTest();
void SPIReSet(uint8_t value);

void setFirstP(uint8_t bRun);
void DrawFirstP();

#ifdef __cplusplus
{
#endif // extern "C"

#endif // SERIAL_SPI_H