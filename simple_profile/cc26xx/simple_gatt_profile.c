/******************************************************************************

 @file       simple_gatt_profile.c

 @brief This file contains the Simple GATT profile sample GATT service profile
        for use with the BLE sample application.

 Group: CMCU, SCS
 Target Device: CC2640R2

 ******************************************************************************
 
 Copyright (c) 2010-2017, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 Release Name: simplelink_cc2640r2_sdk_1_40_00_45
 Release Date: 2017-07-20 17:16:59
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <icall.h>
#include "util.h"
/* This Header file contains all BLE API and icall structure definition */
#include "icall_ble_api.h"

#include "simple_gatt_profile.h"

//#include "hw_spi.h"
unsigned char am_index;
unsigned int data_count;//test
unsigned char test_count=0;//test
unsigned char *start,*write_data_end;//test
uint16  current_buf=0;//test
#define SERVAPP_NUM_ATTR_SUPPORTED        17

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFFF0
CONST uint8 simpleProfileServUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SIMPLEPROFILE_SERV_UUID), HI_UINT16(SIMPLEPROFILE_SERV_UUID)
};

// Characteristic 1 UUID: 0xFFF1
CONST uint8 simpleProfilechar1UUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SIMPLEPROFILE_CHAR1_UUID), HI_UINT16(SIMPLEPROFILE_CHAR1_UUID)
};

// Characteristic 2 UUID: 0xFFF2
CONST uint8 simpleProfilechar2UUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SIMPLEPROFILE_CHAR2_UUID), HI_UINT16(SIMPLEPROFILE_CHAR2_UUID)
};

// Characteristic 3 UUID: 0xFFF3
CONST uint8 simpleProfilechar3UUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SIMPLEPROFILE_CHAR3_UUID), HI_UINT16(SIMPLEPROFILE_CHAR3_UUID)
};

// Characteristic 4 UUID: 0xFFF4
CONST uint8 simpleProfilechar4UUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SIMPLEPROFILE_CHAR4_UUID), HI_UINT16(SIMPLEPROFILE_CHAR4_UUID)
};

// Characteristic 5 UUID: 0xFFF5
CONST uint8 simpleProfilechar5UUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SIMPLEPROFILE_CHAR5_UUID), HI_UINT16(SIMPLEPROFILE_CHAR5_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static simpleProfileCBs_t *simpleProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Simple Profile Service attribute
static CONST gattAttrType_t simpleProfileService = { ATT_BT_UUID_SIZE, simpleProfileServUUID };


// Simple Profile Characteristic 1 Properties
//static uint8 simpleProfileChar1Props = GATT_PROP_READ | GATT_PROP_NOTIFY;
//static uint8 simpleProfileChar1Props = GATT_PROP_WRITE;
static uint8 simpleProfileChar1Props = GATT_PROP_NOTIFY;

//static uint8 simpleProfileChar1[SIMPLEPROFILE_CHAR1_LEN] = { 11,22,33 };
// Simple Profile Characteristic 1 User Description
static uint8 simpleProfileChar1UserDesp[17] = "Characteristic 1";

static gattCharCfg_t *simpleProfileChar1Config;
// Simple Profile Characteristic 2 Properties
static uint8 simpleProfileChar2Props = GATT_PROP_READ;

// Characteristic 2 Value
static uint8 simpleProfileChar2 = 0;

// Simple Profile Characteristic 2 User Description
static uint8 simpleProfileChar2UserDesp[17] = "Characteristic 2";


// Simple Profile Characteristic 3 Properties
static uint8 simpleProfileChar3Props = GATT_PROP_WRITE;

// Characteristic 3 Value
static uint8 simpleProfileChar3[SIMPLEPROFILE_CHAR3_LEN] = { 0 };
//unsigned char  simpleProfileChar3[SIMPLEPROFILE_CHAR3_LEN] = { 0 };
// Simple Profile Characteristic 3 User Description
static uint8 simpleProfileChar3UserDesp[17] = "Characteristic 3";


// Simple Profile Characteristic 4 Properties
static uint8 simpleProfileChar4Props = GATT_PROP_WRITE;

// Characteristic 4 Value
static uint8 simpleProfileChar4 = 0;

// Simple Profile Characteristic 4 Configuration Each client has its own
// instantiation of the Client Characteristic Configuration. Reads of the
// Client Characteristic Configuration only shows the configuration for
// that client and writes only affect the configuration of that client.
static gattCharCfg_t *simpleProfileChar4Config;

// Simple Profile Characteristic 4 User Description
static uint8 simpleProfileChar4UserDesp[17] = "Characteristic 4";


// Simple Profile Characteristic 5 Properties
static uint8 simpleProfileChar5Props = GATT_PROP_READ;

// Characteristic 5 Value
//static uint8 simpleProfileChar5[SIMPLEPROFILE_CHAR5_LEN] = { 0, 0, 0, 0, 0 };

// Simple Profile Characteristic 5 User Description
//static uint8 simpleProfileChar5UserDesp[17] = "Characteristic 5";
unsigned char write_data[256];
/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simpleProfileAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] =
{
  // Simple Profile Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&simpleProfileService            /* pValue */
  },

    // Characteristic 1 Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &simpleProfileChar1Props
    },

      // Characteristic Value 1
      {
        { ATT_BT_UUID_SIZE, simpleProfilechar1UUID },
        0,
        0,
        simpleProfileChar1
      },
      // Characteristic 4 configuration
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&simpleProfileChar1Config
      },
      // Characteristic 1 User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        simpleProfileChar1UserDesp
      },

    // Characteristic 2 Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &simpleProfileChar2Props
    },

      // Characteristic Value 2
      {
        { ATT_BT_UUID_SIZE, simpleProfilechar2UUID },
        GATT_PERMIT_READ,
        0,
        &simpleProfileChar2
      },

      // Characteristic 2 User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        simpleProfileChar2UserDesp
      },

    // Characteristic 3 Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &simpleProfileChar3Props
    },

      // Characteristic Value 3
      {
        { ATT_BT_UUID_SIZE, simpleProfilechar3UUID },
        GATT_PERMIT_WRITE,
        0,
        simpleProfileChar3
      },

      // Characteristic 3 User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        simpleProfileChar3UserDesp
      },

    // Characteristic 4 Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &simpleProfileChar4Props
    },

      // Characteristic Value 4
      {
        { ATT_BT_UUID_SIZE, simpleProfilechar4UUID },
        0,
        0,
        &simpleProfileChar4
      },

      // Characteristic 4 configuration
      {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&simpleProfileChar4Config
      },

      // Characteristic 4 User Description
      {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        simpleProfileChar4UserDesp
      },

    // Characteristic 5 Declaration
//    {
//      { ATT_BT_UUID_SIZE, characterUUID },
//      GATT_PERMIT_READ,
//      0,
//      &simpleProfileChar5Props
//    },
//
//      // Characteristic Value 5
//      {
//        { ATT_BT_UUID_SIZE, simpleProfilechar5UUID },
//        GATT_PERMIT_AUTHEN_READ,
//        0,
//        simpleProfileChar5
//      },
//
//      // Characteristic 5 User Description
//      {
//        { ATT_BT_UUID_SIZE, charUserDescUUID },
//        GATT_PERMIT_READ,
//        0,
//        simpleProfileChar5UserDesp
//      },
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method);
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// Simple Profile Service Callbacks
// Note: When an operation on a characteristic requires authorization and
// pfnAuthorizeAttrCB is not defined for that characteristic's service, the
// Stack will report a status of ATT_ERR_UNLIKELY to the client.  When an
// operation on a characteristic requires authorization the Stack will call
// pfnAuthorizeAttrCB to check a client's authorization prior to calling
// pfnReadAttrCB or pfnWriteAttrCB, so no checks for authorization need to be
// made within these functions.
CONST gattServiceCBs_t simpleProfileCBs =
{
  simpleProfile_ReadAttrCB,  // Read callback function pointer
  simpleProfile_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SimpleProfile_AddService( uint32 services )
{
  uint8 status;
   // Allocate Client Characteristic Configuration table
  simpleProfileChar1Config = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *linkDBNumConns );
  if ( simpleProfileChar1Config == NULL )
  {
    return ( bleMemAllocError );
  } 
  // Allocate Client Characteristic Configuration table
//  simpleProfileChar4Config = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *
//                                                            linkDBNumConns );
//  if ( simpleProfileChar4Config == NULL )
//  {
//    return ( bleMemAllocError );
//  }

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar1Config );
//  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar4Config );

  if ( services & SIMPLEPROFILE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( simpleProfileAttrTbl,
                                          GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &simpleProfileCBs );
  }
  else
  {
    status = SUCCESS;
  }

  return ( status );
}

/*********************************************************************
 * @fn      SimpleProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t SimpleProfile_RegisterAppCBs( simpleProfileCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    simpleProfile_AppCBs = appCallbacks;

    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

/*********************************************************************
 * @fn      SimpleProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR1:
     if (len == SIMPLEPROFILE_CHAR1_LEN)
      {
        VOID memcpy( simpleProfileChar1, value, SIMPLEPROFILE_CHAR1_LEN );
        GATTServApp_ProcessCharCfg( simpleProfileChar1Config, simpleProfileChar1, FALSE,
                                simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                INVALID_TASK_ID, simpleProfile_ReadAttrCB );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR2:
      if ( len == sizeof ( uint8 ) )
      {
        simpleProfileChar2 = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR3:
      if ( len == sizeof ( uint8 ) )
      {
        VOID memcpy( simpleProfileChar3, value, SIMPLEPROFILE_CHAR3_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR4:
      if ( len == sizeof ( uint8 ) )
      {
        simpleProfileChar4 = *((uint8*)value);

        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( simpleProfileChar4Config, &simpleProfileChar4, FALSE,
                                    simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                    INVALID_TASK_ID, simpleProfile_ReadAttrCB );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    case SIMPLEPROFILE_CHAR5:
      if ( len == SIMPLEPROFILE_CHAR5_LEN )
      {
       // VOID memcpy( simpleProfileChar5, value, SIMPLEPROFILE_CHAR5_LEN );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}
//====================================================================
//hannstar Jack Add
//====================================================================

uint8_t SelectIndex = 0;
//uint8_t EraseCount = 0; 

size_t GetFlashAddress(uint8_t iImageIndex)
{
    return PICTURE1_ADDRESS + (iImageIndex * 4096 * 10);
}

void DrawImage(void)
{
 static  unsigned char  finish=0; 
 static unsigned  char FlashWriteIndex=0,EraseCount = 0;
  size_t StartAdd = GetFlashAddress(SelectIndex) + 4096 * EraseCount;
  bool flag = false;    
  data_count=0;
   
   
  
     
   
    if(FlashWriteIndex == 0)
    {
       flag = ExtFlash_erase(StartAdd ,4096);
    }
    
    flag = ExtFlash_write(StartAdd + FlashWriteIndex * 256, 256, write_data);
//     for(int i=0;i<256;i++)
//   {write_data[i]=0x00;}
    
    if(flag)
    {
      FlashWriteIndex++;
      if(FlashWriteIndex == 4096/256)//16
      {
        EraseCount++;
        FlashWriteIndex = 0;
      }  
      
      finish=(unsigned char)(EraseCount*(4096/256))+FlashWriteIndex;
      
    }

#ifdef ESL_213
    if(finish == 0x10) {
#else
    if(finish>0x9c) //9c
    {//9d//9b
#endif 
      Write_IR(0x2C);      
      DrawPicture(GetFlashAddress(SelectIndex), PICTURE_CMD_SIZE);
      setFirstP(simpleProfileChar3[1]);
      EraseCount=0;
      FlashWriteIndex = 0;
      finish=0;
      Write_IR(0x29);
      FlashOff(0x00);
    }   
 } 

void ConfigSet()
{ 
  
  static unsigned char re_data=0,i,j;
  
  if( simpleProfileChar3[0] == 0x02)//
 {
   
   //SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR1, SIMPLEPROFILE_CHAR1_LEN, simpleProfileChar1);
    //UserProfile_Notify(SIMPLEPROFILE_CHAR1,simpleProfileChar1,3);
      if( simpleProfileChar3[1] == 0x01)//
     {
         FlashOff(0x01);
         Write_IR(0x28); //close lcd
     
         current_buf=simpleProfileChar3[4];
         write_data_end=&write_data[256];
           if(current_buf==0)
           {
             start=&simpleProfileChar3[7];
             SelectIndex=(simpleProfileChar3[6]-1);//第一
             re_data=238;
             data_count=0;
         //re_data=6;
           }else
           { 
            
             start=&simpleProfileChar3[6];//第二
              re_data=239;             
            
           }
        
              if(current_buf==0xa7)//最後-筆
             {                  
                  re_data=23;//87
                 //write_data_end=&write_data[255];//232
                  for(i=0;i<23;i++)
                  { 
                    write_data[data_count]=simpleProfileChar3[8+i];
                    data_count++;
                  }
                  
                  Write_IR(0x2C);
                  DrawImage();
                  
                  for(i=0;i<65;i++)
                  { 
                    write_data[data_count]=simpleProfileChar3[8+22+i];
                    data_count++;
                  }
                  
                  Write_IR(0x2C);
                  DrawImage();
                  
                  
              }
            else
           {
                  for(i=0;i<re_data;i++)//
	         { 
            	    if(data_count<re_data)
                     {
                        write_data[data_count]=*start;
                       start++;
                       // 
                 	data_count+=1;          	
		      }
		        else
  	              {
		         if(&(write_data[data_count])<write_data_end)//256
		          {
		 	
		 	     write_data[data_count]=*start;
		 	      start++;		 	                
		 	     data_count+=1;
                            
		           }else
		           {
		 	           i-=1;
		 	           Write_IR(0x2C);
                                   DrawImage();
		 	
		           }
            
		       }
		          //data_count+=1; 
	           }
                  
              }
          }
     //else
                //{
                
                
                   //data_count=0;
//                  Write_IR(0x2C);
//                  DrawImage();
                
             //  }
     
       
     else if( simpleProfileChar3[1] == 0x02)//畫
     {
       Write_IR(0x2C);
       DrawPicture(GetFlashAddress(SelectIndex), PICTURE_CMD_SIZE);
       Write_IR(0x29);
      }
      else if( simpleProfileChar3[1] == 0x03)
     {
        FlashOff(0x01);

        SelectIndex = simpleProfileChar3[2]-1;//1開始
    
        if(simpleProfileChar3[3] == 0x03)
        {
           Write_IR(0x2C);
            DrawPicture(GetFlashAddress(SelectIndex), PICTURE_CMD_SIZE);

            Write_IR(0x29);
            setFirstP(SelectIndex);

            FlashOff(0x00);
        }
    }
  else if( simpleProfileChar3[1] == 0x04)
  {
    LCDPowerOnOff(simpleProfileChar3[1]);
  }
  else if( simpleProfileChar3[1] == 0x05)
  {
    FlashOff(simpleProfileChar3[1]);
  }
  else if( simpleProfileChar3[1] == 0x06)
  {
    if(simpleProfileChar3[2] == 0x00)
      HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_0_DBM);
    else if(simpleProfileChar3[2] == 0x01)
      HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_1_DBM);
    else if(simpleProfileChar3[2] == 0x02)
      HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_2_DBM);
    else if(simpleProfileChar3[2] == 0x03)
      HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_3_DBM);
    else if(simpleProfileChar3[2] == 0x04)
      HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_4_DBM);    
    else if(simpleProfileChar3[2] == 0x05)
      HCI_EXT_SetTxPowerCmd(HCI_EXT_TX_POWER_5_DBM);
    // check cmd is Has do it.
    {
      //Write_IR(0x2C);
      //SelectIndex = simpleProfileChar3[1];
      //DrawPicture(GetFlashAddress(SelectIndex), PICTURE_CMD_SIZE);
      //Write_IR(0x29);
    }
  }
  else if( simpleProfileChar3[1] == 0x07)
  {
    if(simpleProfileChar3[2] == 0x00)
      HCI_EXT_SetRxGainCmd(HCI_EXT_RX_GAIN_STD);
    else if(simpleProfileChar3[2] == 0x01)
      HCI_EXT_SetRxGainCmd(HCI_EXT_RX_GAIN_HIGH);  
  }
  else if( simpleProfileChar3[1] == 0x08)
  {
    Write_IR(0xCB);
    Write_Data7302(simpleProfileChar3[1]);
  }
  else if( simpleProfileChar3[1] == 0x09)
  {
    Write_IR(0xC2);
    Write_Data7302(simpleProfileChar3[1]);
    Write_Data7302(simpleProfileChar3[1]);
    Write_Data7302(simpleProfileChar3[1]);
    Write_Data7302(simpleProfileChar3[1]);
  }
  else if(simpleProfileChar3[1] == 0x20)
  {
    
         am_index= simpleProfileChar3[2];
    
    //if(simpleProfileChar3[4] == 0x03)
     //{
      setRunAnimation(0x02, simpleProfileChar3[3]);
    // }
  }
  
   else if( simpleProfileChar3[1] == 0x21)
  {
    setRunAnimation(0x00, simpleProfileChar3[3]);
  }
  
  
  
  else if( simpleProfileChar3[1] == 0x22)
  {
    if(simpleProfileChar3[1] == 0x00)
    {
      //TE Off
      Write_IR(0x34);
    }
    else 
    {
      //TE ON;
      Write_IR(0x35);
      Write_Data7302(0x00);

      Write_IR(0x44);
      Write_Data7302(0x00);       
     }
  }
    else if( simpleProfileChar3[1] == 0x13)
  {
       switch(simpleProfileChar3[2])
      {
      case 0x00: //0.25Hz
        Write_IR(0x39); //LPM
  
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x00); 
        break;
      case 0x01: //0.05Hz
        Write_IR(0x39); //LPM
  
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x01); 
        break;
      case 0x02: //1Hz
        Write_IR(0x39); //LPM
  
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x02); 
        break;
      case 0x03: //2Hz
        Write_IR(0x39); //LPM
  
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x03); 
        break;
      case 0x04: //4Hz
        Write_IR(0x39); //LPM
  
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x04); 
        break;
      case 0x05: //8Hz
        Write_IR(0x39); //LPM
  
        Write_IR(0xB2);
        Write_Data7302(0x00);        
        Write_Data7302(0x05); 
        break;
      case 0x06: //16Hz
        Write_IR(0x38); //HPM
  
        Write_IR(0xC7);        
        Write_Data7302(0xA0);
        Write_Data7302(0xE9);
        
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x00); 
        break;
      case 0x07: //25.5Hz
        Write_IR(0x38); //HPM
  
        Write_IR(0xC7);        
        Write_Data7302(0x80);
        Write_Data7302(0xE9);
        
        Write_IR(0xB2);
        Write_Data7302(0x00); 
        Write_Data7302(0x00); 
        break;
      case 0x08: //32Hz
        Write_IR(0x38); //HPM
  
        Write_IR(0xC7);        
        Write_Data7302(0xA0);
        Write_Data7302(0xE9);
        
        Write_IR(0xB2);
        Write_Data7302(0x01); 
        Write_Data7302(0x00); 
        break;
      case 0x09: //51Hz
        Write_IR(0x38); //HPM
  
        Write_IR(0xC7);        
        Write_Data7302(0x80);
        Write_Data7302(0xE9);
        
        Write_IR(0xB2);
        Write_Data7302(0x01); 
        Write_Data7302(0x00); 
        break;
      }   
  } 
  	}
  
}


/*********************************************************************
 * @fn      SimpleProfile_GetParameter
 *
 * @brief   Get a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR1:
      //*((uint8*)value) =  EraseCount*4 + FlashWriteIndex; 
      //DrawImage();
      VOID memcpy( value, simpleProfileChar1,SIMPLEPROFILE_CHAR1_LEN );
      break;

    case SIMPLEPROFILE_CHAR2:
      *((uint8*)value) = simpleProfileChar2;
      break;

    case SIMPLEPROFILE_CHAR3:
      ConfigSet();
      //*((uint8*)value) = simpleProfileChar3;
      break;

    case SIMPLEPROFILE_CHAR4:
      *((uint8*)value) = simpleProfileChar4;
      break;

    case SIMPLEPROFILE_CHAR5:
     // VOID memcpy( value, simpleProfileChar5, SIMPLEPROFILE_CHAR5_LEN );
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @fn          simpleProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method)
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
      // gattserverapp handles those reads

      // characteristics 1 and 2 have read permissions
      // characteritisc 3 does not have read permissions; therefore it is not
      //   included here
      // characteristic 4 does not have read permissions, but because it
      //   can be sent as a notification, it is included here
      case SIMPLEPROFILE_CHAR1_UUID:
//        *pLen = 1;
//         pValue[0] =  EraseCount*(4096/256) + FlashWriteIndex; 
//        break;
        
        *pLen = SIMPLEPROFILE_CHAR1_LEN;
        VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR1_LEN );
        break;
        
      case SIMPLEPROFILE_CHAR2_UUID:
      case SIMPLEPROFILE_CHAR4_UUID:
        *pLen = 1;
        pValue[0] = *pAttr->pValue;
        break;

      case SIMPLEPROFILE_CHAR5_UUID:
        *pLen = SIMPLEPROFILE_CHAR5_LEN;
        VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR5_LEN );
        break;

      default:
        // Should never get here! (characteristics 3 and 4 do not have read permissions)
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method)
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      case SIMPLEPROFILE_CHAR1_UUID:
        if(offset == 0)
        {
          if(len > SIMPLEPROFILE_CHAR1_LEN)
          {
              status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else 
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        if(status == SUCCESS)
        {
          uint8_t *pCurValue = (uint8_t *)pAttr->pValue;
          
          memset(pCurValue, 0 , SIMPLEPROFILE_CHAR1_LEN);
          memcpy(pCurValue, pValue, len);
          
          notifyApp = SIMPLEPROFILE_CHAR1;
        }
        
        break;
    case SIMPLEPROFILE_CHAR3_UUID:

        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          
          
          
          if ( len > SIMPLEPROFILE_CHAR3_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }

        //Write the value
        if ( status == SUCCESS )
        {
          uint8_t *pCurValue = (uint8_t *)pAttr->pValue;
          
          memset(pCurValue, 0 , SIMPLEPROFILE_CHAR3_LEN);
          memcpy(pCurValue, pValue, len);
          
          notifyApp = SIMPLEPROFILE_CHAR3;
        }

        break;

      case GATT_CLIENT_CHAR_CFG_UUID:
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                                 offset, GATT_CLIENT_CFG_NOTIFY );
        break;

      default:
        // Should never get here! (characteristics 2 and 4 do not have write permissions)
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }

  // If a characteristic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnSimpleProfileChange )
  {
    simpleProfile_AppCBs->pfnSimpleProfileChange( notifyApp );
  }

  return ( status );
}
//static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
//                                           gattAttribute_t *pAttr,
//                                           uint8_t *pValue, uint16_t len,
//                                           uint16_t offset, uint8_t method)
//{
//  bStatus_t status = SUCCESS;
//  uint8 notifyApp = 0xFF;
//
//  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
//  {
//    // 16-bit UUID
//    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
//    switch ( uuid )
//    {
//      case SIMPLEPROFILE_CHAR1_UUID:
//          if ( offset == 0 )
//          {
//              if ( len > SIMPLEPROFILE_CHAR1_LEN )
//              {
//                  status = ATT_ERR_INVALID_VALUE_SIZE;
//              }
//          }
//          else
//          {
//              status = ATT_ERR_ATTR_NOT_LONG;
//          }
//          //Write the value
//          if ( status == SUCCESS )
//          {
//              VOID memcpy(pAttr->pValue,  pValue, len );
//
//
//
//
//              notifyApp = SIMPLEPROFILE_CHAR1;
//          }
//
//          break;
//      case SIMPLEPROFILE_CHAR3_UUID:
//
//        //Validate the value
//        // Make sure it's not a blob oper
//        if ( offset == 0 )
//        {
//          if ( len != 1 )
//
//
//
//          {
//            status = ATT_ERR_INVALID_VALUE_SIZE;
//          }
//        }
//        else
//        {
//          status = ATT_ERR_ATTR_NOT_LONG;
//        }
//
//        //Write the value
//        if ( status == SUCCESS )
//        {
//          uint8 *pCurValue = (uint8 *)pAttr->pValue;
//          *pCurValue = pValue[0];
////          if( pAttr->pValue == &simpleProfileChar1 )
////          {
////            notifyApp = SIMPLEPROFILE_CHAR1;
////          }
////          else
////          {
//              notifyApp = SIMPLEPROFILE_CHAR3;
////          }
//        }
//
//        break;
//
//      case GATT_CLIENT_CHAR_CFG_UUID:
//         //CHAR1 的通知??
////          if(pAttr->handle == simpleProfileAttrTbl[ATTRTBL_CHAR1_CCC_IDX].handle)
////          {
////              status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
////                                                      offset, GATT_CLIENT_CFG_NOTIFY );
////          }
////          //CHAR4 的通知??
////          else if(pAttr->handle == simpleProfileAttrTbl[ATTRTBL_CHAR4_CCC_IDX].handle)
////          {
////              status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
////                                                                   offset, GATT_CLIENT_CFG_NOTIFY );
////          }
////          else
////          {
////              status = ATT_ERR_INVALID_HANDLE;
////          }
//                 status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
//                                                offset, GATT_CLIENT_CFG_NOTIFY );
//        break;
//
//      default:
//        // Should never get here! (characteristics 2 and 4 do not have write permissions)
//        status = ATT_ERR_ATTR_NOT_FOUND;
//        break;
//    }
//  }
//  else	//其他情?不打?通知??
//  {
//    // 128-bit UUID
//    status = ATT_ERR_INVALID_HANDLE;
//  }
//
//  // If a characteristic value changed then callback function to notify application of change
//  if ( (notifyApp != 0xFF ) && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnSimpleProfileChange )
//  {
//    simpleProfile_AppCBs->pfnSimpleProfileChange( notifyApp );
//  }
//
//  return ( status );
//}
///*********************************************************************
//*********************************************************************/
