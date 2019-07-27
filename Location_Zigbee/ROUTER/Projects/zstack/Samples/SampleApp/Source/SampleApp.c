/**************************************************************************************************
Filename:       SampleApp.c
Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
Revision:       $Revision: 19453 $

Description:    Sample Application (no Profile).


Copyright 2007 Texas Instruments Incorporated. All rights reserved.

IMPORTANT: Your use of this Software is limited to those specific rights
granted under the terms of a software license agreement between the user
who downloaded the software, his/her employer (which must be your employer)
and Texas Instruments Incorporated (the "License").  You may not use this
Software unless you agree to abide by the terms of the License. The License
limits your use, and you acknowledge, that the Software may not be modified,
copied or distributed unless embedded on a Texas Instruments microcontroller
or used solely and exclusively in conjunction with a Texas Instruments radio
frequency transceiver, which is integrated into your product.  Other than for
the foregoing purpose, you may not use, reproduce, copy, prepare derivative
works of, modify, distribute, perform, display or sell this Software and/or
its documentation for any purpose.

YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
PROVIDED �AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

Should you have any questions regarding your right to use this Software,
contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/*********************************************************************
This application isn't intended to do anything useful, it is
intended to be a simple example of an application's structure.

This application sends it's messages either as broadcast or
broadcast filtered group messages.  The other (more normal)
message addressing is unicast.  Most of the other sample
applications are written to support the unicast message model.

Key control:
SW1:  Sends a flash command to all devices in Group 1.
SW2:  Adds/Removes (toggles) this device in and out
of Group 1.  This will enable and disable the
reception of the flash command.
*********************************************************************/

/*********************************************************************
* INCLUDES
*/
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"
#include "ds18b20.h"


/* user define*/
#include "string.h"
#include "stdlib.h"

/*********************************************************************
* MACROS
*/

/*********************************************************************
* CONSTANTS
*/

/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLESȫ�ֱ�������
*/
//����
#define MY_DEFINI_UART_PORT 0 
#ifndef RX_MAX_LENGTH
#define RX_MAX_LENGTH 20
#endif
uint8 RX_BUFFER[RX_MAX_LENGTH];
uint8 RX_Length;
void UART_CallBackFunction(uint8 port, uint8 event);//�����������ݵ���ʱ�����ô˺���




// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
* EXTERNAL VARIABLES
*/

/*********************************************************************
* EXTERNAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/
uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
// This variable will be received when
// SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;
afAddrType_t SampleApp_p2p_DstAddr;//�㲥��ַ

aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;
//ê�ڵ���Э����֮������ݴ���ʹ��
uint8 rssi_val[10];
uint8 rssi_index = 0; 
uint8 rssi_s[3] = {'\0'};
//���͸�Э����������

/*********************************************************************
* LOCAL FUNCTIONS
*/
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );


void SampleApp_P2P_SendMessage(void);
void SamplaAPP_ProcessBlindData( afIncomingMSGPacket_t *pkt);//�ο��ڵ㴦����յ���ê�ڵ������

/*********************************************************************
* NETWORK LAYER CALLBACKS
*/

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*********************************************************************
* @fn      SampleApp_Init
*
* @brief   Initialization function for the Generic App Task.
*          This is called during initialization and should contain
*          any application specific initialization (ie. hardware
*          initialization/setup, table initialization, power up
*          notificaiton ... ).
*
* @param   task_id - the ID assigned by OSAL.  This ID should be
*                    used to send messages and set timers.
*
* @return  none
*/
void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  SampleApp_NwkState = DEV_INIT;
  SampleApp_TransID = 0;
  halUARTCfg_t uartConfig;//��ʼ������ʹ��
  
  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().
  
#if defined ( BUILD_ALL_DEVICES )
  // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
  if ( readCoordinatorJumper() )
    zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  else
    zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES
  
#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif
  
  // Setup for the periodic message's destination address
  // Broadcast to everyone��
  //�ڶ�λʵ���У�ä�ڵ�Ӧ��ʹ�ù㲥��ʽ���òο��ڵ��ܹ����ܽ��յ�����
  /*
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;
  
  */ 
  //�鲥��ʽ�ĳ�������Ӧ����ÿ����Ϊһ����ʹ�á����β���ʵ�飬����
  // Setup for the flash command's destination address - Group 1
  /*
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup;
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;
  */
  
  // �㲥���á���λʱ���ο��ڵ���Э����֮���ͨѶ��Ӧ���ǵ㲥���Ӷ���Э�����ܹ�����
  
  SampleApp_p2p_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  SampleApp_p2p_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_p2p_DstAddr.addr.shortAddr = 0x0000;//������Ϣ��Э����
  
  
  // Fill out the endpoint description.
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
    = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;
  
  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );
  
  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID );
  
  // By default, all devices start out in Group 1
  //Ĭ������£������豸������1�����ԶԴ˽������ã��Ӷ�ʱ��ͬ���豸���벻ͬ����
  SampleApp_Group.ID = 0x0001;
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
  
  
  //���ڳ�ʼ��
  uartConfig.configured       = TRUE;
  uartConfig.baudRate         = HAL_UART_BR_9600;
  uartConfig.flowControl      = FALSE;
  uartConfig.flowControlThreshold = MT_UART_THRESHOLD;//Ĭ��ʹ��������
  uartConfig.rx.maxBufSize     = 200;
  uartConfig.tx.maxBufSize     =200;
  uartConfig.idleTimeout       = MT_UART_IDLE_TIMEOUT;
  uartConfig.intEnable          =TRUE;
  uartConfig.callBackFunc       =UART_CallBackFunction;
  HalUARTOpen(MY_DEFINI_UART_PORT,&uartConfig);//ʹ���Զ���Ĵ��ڳ�ʼ����������������ͨѶ
  

  
//  osal_start_timerEx(SampleApp_TaskID,USER_EVT,500);
  
  
  
  
  
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SampleApp", HAL_LCD_LINE_1 );
#endif
}

/*********************************************************************
* @fn      SampleApp_ProcessEvent
*
* @brief   Generic Application Task event processor.  This function
*          is called to process all events for the task.  Events
*          include timers, messages and any other user defined events.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - events to process.  This is a bit map and can
*                   contain more than one event.
*
* @return  none
*/
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter
  
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a key is pressed
      case KEY_CHANGE:
        SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
        break;
        
        // Received when a messages is received (OTA) for this endpoint
      case AF_INCOMING_MSG_CMD:
        SampleApp_MessageMSGCB( MSGpkt );
        break;
        
        // Received whenever the device changes state in the network
      case ZDO_STATE_CHANGE:
        SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
        if (// (SampleApp_NwkState == DEV_ZB_COORD)|| //Э����ֻ����������ݣ�����������
            (SampleApp_NwkState == DEV_ROUTER)
              || (SampleApp_NwkState == DEV_END_DEVICE) )
        {
          // Start sending the periodic message in a regular interval.
          osal_start_timerEx( SampleApp_TaskID,
                             SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                             SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
        }
        else
        {
          // Device is no longer in the network
        }
        break;
        
      default:
        break;
      }
      
      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
      
      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }
    
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
 /* 
  if ( events & USER_EVT )//���ڶ�����p2p��Ŀ�Ľڵ�ΪЭ���������Դ˴����õ���ê�ڵ㷢������,ʹ���Զ��庯��
  {
    // Send the periodic message
    SampleApp_P2P_SendMessage();
    osal_start_timerEx(SampleApp_TaskID, USER_EVT, 5000);
    
    // return unprocessed events
    return (events ^ USER_EVT);
  }
  */
  // Discard unknown events
  return 0;
}

/*********************************************************************
* Event Generation Functions
*/
/*********************************************************************
* @fn      SampleApp_HandleKeys
*
* @brief   Handles all key events for this device.
*
* @param   shift - true if in shift/alt.
* @param   keys - bit field for key events. Valid entries:
*                 HAL_KEY_SW_2
*                 HAL_KEY_SW_1
*
* @return  none
*/
void SampleApp_HandleKeys( uint8 shift, uint8 keys )
{
  (void)shift;  // Intentionally unreferenced parameter
  
  if ( keys & HAL_KEY_SW_1 )
  {
    /* This key sends the Flash Command is sent to Group 1.
    * This device will not receive the Flash Command from this
    * device (even if it belongs to group 1).
    */
    //SampleApp_SendFlashMessage( SAMPLEAPP_FLASH_DURATION );
  }
  
  if ( keys & HAL_KEY_SW_2 )
  {
    /* The Flashr Command is sent to Group 1.
    * This key toggles this device in and out of group 1.
    * If this device doesn't belong to group 1, this application
    * will not receive the Flash command sent to group 1.
    */
    aps_Group_t *grp;
    grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    if ( grp )
    {
      // Remove from the group
      aps_RemoveGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    }
    else
    {
      // Add to the flash group
      aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
    }
  }
}

/*********************************************************************
* LOCAL FUNCTIONS
*/

/*********************************************************************
* @fn      SampleApp_MessageMSGCB
*
* @brief   Data message processor callback.  This function processes
*          any incoming data - probably from other devices.  So, based
*          on cluster ID, perform the intended action.
*
* @param   none
*
* @return  none
*/
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )//�ڱ�������Ӧ�öԽ��յ���ä�ڵ��RSSIֵ��Ϣ����
{
  switch ( pkt->clusterId )
  {
    //����ȷ��ֻ����·�����вű���˺�����Э����������
  case SAMPLEAPP_BLIND_CLUSTERID://˵����ä�ڵ㷢���������ݣ�ֻ�вο��ڵ����ä�ڵ����Ϣ
    //    SamplaAPP_ProcessBlindData(pkt );
   
//  HalUARTWrite(0, pkt->cmd.Data, pkt->cmd.DataLength);   
    rssi_val[rssi_index++] = ((~pkt->rssi)+1); //�õ�rssi��ֵ��������   
    

  uint16 val = 0;    
   
  
  // Shu Xuan Yang do the next
    if(rssi_index == 16)
    {
      
      uint8 smooth_listener = 0;
      uint8 location = 0;
      uint8 current_max_length = 0;
      uint8 max_length = 0;
        
      uint8 offset[3] = {1, 7, 11};
      uint8 i = 0;
      uint8 j = 0;
      uint16 temp = 0;
      for (i = 0; i < 3; i++)
      {
        for (j = 0; ((j + offset[i]) < 16) && (rssi_val[j] > rssi_val[j + offset[i]]); j += offset[i])
        {
          temp = rssi_val[j];
          rssi_val[j] = rssi_val[j + offset[i]];
          rssi_val[j + offset[i]] = temp;
        }
      }
      
#define BARABLE_MISTAKE 8
      
      for (i = 0; i < 15; i++)
      {
        smooth_listener = rssi_val[i + 1] - rssi_val[i]; 
        if (smooth_listener > BARABLE_MISTAKE)
        {
          if (current_max_length > max_length)
          {
            max_length = current_max_length;
            current_max_length = 0;
            location = i;
          }
        }
        else
          current_max_length++;
      }
      
#undef BARABLE_MISTAKE
      for (i = 0; i < max_length; i++)
      {
        val += rssi_val[location - i];
      }
      val /= max_length;
    }

      
      //      rssi_s[0] ='A';
      rssi_s[0] = val/100+0x30;//ASCII���� 0x30��Ӧ�������� 0������������������Ҫ��ʾ�����ַ�
      rssi_s[1] = val%100/10+0x30;
      rssi_s[2] = val%10+0x30;//��������ֱ����õ����ף����ף�����      

      SampleApp_P2P_SendMessage();
      rssi_index = 0;//ÿ�յ�ʮ��RSSIֵ����һ����Ҫ��RSSI��ֵ

         break; 
    }

 
}


/*********************************************************************
*********************************************************************/
static void UART_CallBackFunction(uint8 port, uint8 event)
{
  uint8 RX_Flag = 0;
  RX_Length = 0;//�����ַ�������
  RX_Flag = RX_Length = Hal_UART_RxBufLen(MY_DEFINI_UART_PORT);// ���յ����ַ�������
  
  if(RX_Flag != 0)// �����ݴ���
  {
    //��ȡ��������
    HalUARTRead(MY_DEFINI_UART_PORT,RX_BUFFER,RX_Length);
    {
      //�����ݷ��ظ����ԣ�ʹ��hal_uart.h�Ľӿں���
      HalUARTWrite(MY_DEFINI_UART_PORT,RX_BUFFER,RX_Length);
    }
  }
  
}



/*
�������ǲο��ڵ㴦��ä�ڵ�����ݴ��������ڱ�������Ӧ�õõ�ê�ڵ���ä�ڵ�֮��ľ��룬
Ȼ��ê�ڵ��ڷ������ݵ�ʱ�򣬽��������ֵҲ���ͳ�ȥ
*/



/*********************************************************************
* @fn      SampleApp_P2P_SendMessage
*
* @brief   point to point.
*
* @param   none
*
* @return  none
��ÿ��·�������ն˽ڵ㷢������ʱ��ͨ���궨AF_DataRequest�еĴ�ID���Ϳ��Լ򵥵�
���豸���з��࣬��ͨ�����ò�ͬ��ID���궨��ͬ���豸��Ȼ����Э�����ˣ�ͨ���Խ��յ�
����Ϣ�Ĵ�ID������������ĸ��豸������������
*/
void SampleApp_P2P_SendMessage( void )
//���ڲο��ڵ��ڷ������ݵ�ʱ�򣬱��뷢�͵��ǲο��ڵ���ä�ڵ�֮��ľ���ֵ
{
  
  /*AF�ڷ������ݵ�ʱ���Ƿǳ�������Եģ����һ�������Ƿ���Ŀ�ĵصĵ�ַ֮��Ĳ�������SampleApp_Init��ʼ����ʱ��Ͷ����ˡ�
  Ȼ������豸��������ϵͳĬ�Ͼͺã�����SampleApp_InitҲ�����ˡ���������������ʹ���Զ����p2p���֣��������ý��պ�����Э������
  ��ȷ֪��������Ϣ���������ͣ��㲥���鲥���㲥�����Զ����p2p����
  ����һ��Ҫ��ȷ�������ݷ��͵�ʱ���Ѿ������˺ܶ�Ĺ涨��������ɽ��ն�������鷳  
  */
  


  
  if ( AF_DataRequest( &SampleApp_p2p_DstAddr, //����Ŀ�ĵ�ַ+�˵��ַ+����ģʽ
                      &SampleApp_epDesc,//Դ���𸴻�ȷ�ϣ��ն˵����������豸������
                      SAMPLEAPP_ROUT_BNODE_CLUSTERID,//�Ѿ����壨ϵͳ���Զ��壩�Ĵص�ID��������ʾ���͵����ݱ�ʶ���ý��պ���������������
                      3,//�������ݵĳ���
                      rssi_s,//�������ݻ�������������ֵ
                      &SampleApp_TransID,//����ID��ϵͳ����䣿
                      AF_DISCV_ROUTE,//��Чλ����ķ���ѡ��
                      AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )//����������ͨ��ʹ��Ĭ��ֵ
  {
  }
  else
  {
    // Error occurred in request to send.
  }
  
  

}

//