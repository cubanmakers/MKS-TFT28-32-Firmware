/********************   (C) COPYRIGHT 2014 www.makerbase.com.cn   ********************
 * �ļ���  ��mks_tft_gcode.c
 * ����    ��1.��u�̶�ȡԴ�ļ���ÿ�ζ�ȡ1k�ֽڣ�����д��udiskBuffer.buffer[0]��udiskBuffer.buffer[1]��
 						2. ��udiskBuffer.buffer[n]�ǿ�ʱ����ȡ����Чgcodeָ�����ǰ/��׺��,Push��gcodeTxFIFO���С�
 * ����    ��skyblue
**********************************************************************************/


#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "mks_tft_fifo.h"
#include "mks_tft_gcode.h"
//#include "main.h"
#include "mks_tft_com.h"
#include "printer.h"
#include "draw_ui.h"
#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"
#include "xprintf.h"

extern FIL fp_reprint_rw;

extern USBH_HOST                     USB_Host;

extern unsigned char path_bak[15];
extern unsigned char *path_reprint;
extern uint32_t rePrintOffset;

extern PRINT_TIME print_time;
extern void Close_machine_display();

extern void Btn_putdown_close_machine();
extern uint8_t IsChooseAutoShutdown;
extern uint8_t close_fail_flg;
extern uint16_t close_fail_cnt;

struct position Gcode_current_position[30];

uint8_t Chk_close_machine_flg = 0;

UDISK_DATA_BUFFER udiskBuffer;

unsigned char note_flag=1;	//ע�ͱ�־ init : 1
unsigned long gcodeLineCnt=0;	//ָ���кű�� Nxxxxx
unsigned long gcodeLineCntBak1=0;
unsigned long gcodeLineCntBak2=0;

UDISK_FILE_STAUS udiskFileStaus;			//�ļ�״̬

TARGER_TEMP targetTemp;
TEMP_STATUS	tempStatus;
void getFanStatus(unsigned char *gcode,unsigned char *end)
{
	unsigned char tempBuf[30];
	unsigned char i;
	unsigned char *p;
	
		if(*gcode == 'M' && *(gcode+1) == '1' && *(gcode+2) == '0'&& (*(gcode+3) == '6' || *(gcode+3) == '7' ))	//M106 M107
		{
			p = gcode;
			i=0;
			while(p<end)
			{
				tempBuf[i++]=*p++;
			}
			tempBuf[i] = '\n';
			
			pushFIFO(&gcodeCmdRxFIFO,&tempBuf[0]);
		}
		
}

void getTargetTemp(unsigned char *gcode,unsigned char *end)
{
	int8_t *tmpStr_1 = 0;
	
	unsigned char tempBuf[80]="ok T:0 /210 B:0 /45 @:0 B@:0";
	unsigned char count;
	unsigned char *p;
	if(tempStatus == temp_ok )		return;
	
	p = &tempBuf[0];
	//��ȡ��λ mm or inch ,Ĭ��mm

	if(*gcode == 'G' && *(gcode+1) == '2' && *(gcode+2) == '0' )
		RePrintData.unit = 1;	//0 mm,1 inch
/*	
//20151019
	if(*gcode == 'M' && *(gcode+1) == '1' && (*(gcode+2) == '9' ||*(gcode+2) == '4' )&& *(gcode+3) == '0')	//M190 or M140
	{
		gcode += 4;
		count = 0;
		while(*gcode++ != 'S')
			if(count++ > 10) break;

		while(gcode < end)
		{
			if(*gcode == '.')break;
			*p++ = *gcode++;
			if(p >=&tempBuf[0]+10) break;
		}
		*p = '\0';
		targetTemp.bedTemp = atoi(&tempBuf[0]);
	}

	if(*gcode == 'M' && *(gcode+1) == '1' && *(gcode+2) == '0'&& (*(gcode+3) == '9' || *(gcode+3) == '4'))	//M109 or M104
	{
		gcode += 4;
		count = 0;
		while(*gcode++ != 'S')
			if(count++ > 10) break;

		while(gcode < end)
		{
			if(*gcode == '.') break;
			*p++ = *gcode++;
			if(p >=&tempBuf[0]+10) break;
		}
		*p = '\0';
		targetTemp.t0Temp = atoi(&tempBuf[0]);
	}
*/
	if(*gcode == 'M' && *(gcode+1) == '1' && (*(gcode+2) == '9' ||*(gcode+2) == '4' )&& *(gcode+3) == '0')	//M190 or M140
	{
		gcode += 4;
		count = 0;
		while(*gcode++ != 'S')
			if(count++ > 10) break;

		while(gcode < end)
		{
			if(*gcode == '.')break;
			*p++ = *gcode++;
			if(p >=&tempBuf[0]+10) break;
		}
		*p = '\0';
		targetTemp.bedTemp = atoi(&tempBuf[0]);
	}

	if(gCfgItems.sprayerNum == 1)
	{
		if(*gcode == 'M' && *(gcode+1) == '1' && *(gcode+2) == '0'&& (*(gcode+3) == '9' || *(gcode+3) == '4'))	//M109 or M104
		{
			gcode += 4;
			count = 0;
			while(*gcode++ != 'S')
				if(count++ > 10) break;

			while(gcode < end)
			{
				if(*gcode == '.') break;
				*p++ = *gcode++;
				if(p >=&tempBuf[0]+10) break;
			}
			*p = '\0';
			targetTemp.t0Temp = atoi(&tempBuf[0]);
		}
	}
	else
	{
		if(*gcode == 'M' && *(gcode+1) == '1' && *(gcode+2) == '0'&& (*(gcode+3) == '9' || *(gcode+3) == '4'))	//M109 or M104
		{
			if((int8_t *)strstr(gcode, "T0"))
			{
					tmpStr_1 = (int8_t *)strstr(gcode, "S");	
					if(tmpStr_1)
					{
						gcode = tmpStr_1+1;
						while(gcode < end)
						{
							if(*gcode == '.') break;
							*p++ = *gcode++;
							if(p >=&tempBuf[0]+10) break;
						}
						*p = '\0';
						targetTemp.t0Temp = atoi(&tempBuf[0]);						
					}
			}
			else if((int8_t *)strstr(gcode, "T1"))
			{
					tmpStr_1 = (int8_t *)strstr(gcode, "S");	
					if(tmpStr_1)
					{
						gcode = tmpStr_1+1;
						while(gcode < end)
						{
							if(*gcode == '.') break;
							*p++ = *gcode++;
							if(p >=&tempBuf[0]+10) break;
						}
						*p = '\0';
						targetTemp.t1Temp = atoi(&tempBuf[0]);						
					}			
			}
			else
			{
					tmpStr_1 = (int8_t *)strstr(gcode, "S");	
					if(tmpStr_1)
					{
						gcode = tmpStr_1+1;
						while(gcode < end)
						{
							if(*gcode == '.') break;
							*p++ = *gcode++;
							if(p >=&tempBuf[0]+10) break;
						}
						*p = '\0';
						if(RePrintData.spayerchoose == 1)
						{
							targetTemp.t1Temp = atoi(&tempBuf[0]);
						}
						else
						{
							targetTemp.t0Temp = atoi(&tempBuf[0]);						
						}		
					}
			}		

		}

	}
/*
	if((targetTemp.bedTemp > 0 && targetTemp.t0Temp >0) ||( gcodeLineCnt> 50))
	{
		//tempBuf[40]="ok T:0 /210 B:0 /45 @:0 B@:0";
		p = &tempBuf[0];	
		*p++ = 'o';*p++ = 'k';*p++ = ' ';*p++ = 'T';*p++ = ':';*p++ = '0';*p++ = ' ';*p++ = '/';
		*p++ = targetTemp.t0Temp/100+48;
		*p++ = (targetTemp.t0Temp/10)%10 + 48;
		*p++ = targetTemp.t0Temp%10 + 48;
		
		*p++ = ' ';	*p++ = 'B';*p++ = ':';*p++ = '0';*p++ = ' ';*p++ = '/';
		*p++ = targetTemp.bedTemp/10+48;
		*p++ = targetTemp.bedTemp%10 + 48;
		*p++ = ' ';*p++ = '@';*p++ = ':';*p++ = '0';*p++ = ' ';*p++ = 'B';*p++ = '@';*p++ = ':';*p++ = '0';*p++ = '\n';
		
		pushFIFO(&gcodeCmdRxFIFO,&tempBuf[0]);
		tempStatus = temp_ok;
	}
*/
	if((targetTemp.bedTemp > 0)||(targetTemp.t0Temp >0)||(targetTemp.t1Temp >0))
	{
		if(gCfgItems.sprayerNum == 1)
		{
			//tempBuf[40]="ok T:0 /210 B:0 /45 @:0 B@:0";
			p = &tempBuf[0];	
			*p++ = 'o';*p++ = 'k';*p++ = ' ';*p++ = 'T';*p++ = ':';
			// *p++ = '0';
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[0]))/100+48;
			*p++ = (((uint32_t)(gCfgItems.curSprayerTemp[0]))/10)%10 + 48;
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[0]))%10 + 48;
			*p++ = ' ';*p++ = '/';
			*p++ = targetTemp.t0Temp/100+48;
			*p++ = (targetTemp.t0Temp/10)%10 + 48;
			*p++ = targetTemp.t0Temp%10 + 48;
			
			*p++ = ' ';	*p++ = 'B';*p++ = ':';
			// *p++ = '0';
			*p++ = ((uint32_t)(gCfgItems.curBedTemp))/100+48;
			*p++ = (((uint32_t)(gCfgItems.curBedTemp))/10)%10+48;
			*p++ = ((uint32_t)(gCfgItems.curBedTemp))%10 + 48;
			*p++ = ' ';*p++ = '/';
			*p++ = targetTemp.bedTemp/100+48;
			*p++ = (targetTemp.bedTemp/10)%10+48;
			*p++ = targetTemp.bedTemp%10 + 48;
			*p++ = ' ';*p++ = '@';*p++ = ':';*p++ = '0';*p++ = ' ';*p++ = 'B';*p++ = '@';*p++ = ':';*p++ = '0';*p++ = '\n';
			
			pushFIFO(&gcodeCmdRxFIFO,&tempBuf[0]);
		}
		else
		{
			//ok T:0 /210 B:0 /45 T0:0/210 T1:0 /210 @:0 B@:0
			p = &tempBuf[0];	
			*p++ = 'o';*p++ = 'k';*p++ = ' ';*p++ = 'T';*p++ = ':';
			// *p++ = '0';
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[0]))/100+48;
			*p++ = (((uint32_t)(gCfgItems.curSprayerTemp[0]))/10)%10 + 48;
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[0]))%10 + 48;
			*p++ = ' ';*p++ = '/';
			*p++ = targetTemp.t0Temp/100+48;
			*p++ = (targetTemp.t0Temp/10)%10 + 48;
			*p++ = targetTemp.t0Temp%10 + 48;
			//B
			*p++ = ' ';	*p++ = 'B';*p++ = ':';
			// *p++ = '0';
			*p++ = ((uint32_t)(gCfgItems.curBedTemp))/100+48;
			*p++ = (((uint32_t)(gCfgItems.curBedTemp))/10)%10+48;
			*p++ = ((uint32_t)(gCfgItems.curBedTemp))%10 + 48;
			*p++ = ' ';*p++ = '/';
			*p++ = targetTemp.bedTemp/100+48;
			*p++ = (targetTemp.bedTemp/10)%10+48;
			*p++ = targetTemp.bedTemp%10 + 48;
			//T0
			*p++ = ' ';*p++ = 'T';*p++ = '0';*p++ = ':';
			// *p++ = '0';
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[0]))/100+48;
			*p++ = (((uint32_t)(gCfgItems.curSprayerTemp[0]))/10)%10 + 48;
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[0]))%10 + 48;			
			*p++ = ' ';*p++ = '/';
			*p++ = targetTemp.t0Temp/100+48;
			*p++ = (targetTemp.t0Temp/10)%10 + 48;
			*p++ = targetTemp.t0Temp%10 + 48;
			//T1
			*p++ = ' ';*p++ = 'T';*p++ = '1';*p++ = ':';
			// *p++ = '0';
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[1]))/100+48;
			*p++ = (((uint32_t)(gCfgItems.curSprayerTemp[1]))/10)%10 + 48;
			*p++ = ((uint32_t)(gCfgItems.curSprayerTemp[1]))%10 + 48;			
			*p++ = ' ';*p++ = '/';
			*p++ = targetTemp.t1Temp/100+48;
			*p++ = (targetTemp.t1Temp/10)%10 + 48;
			*p++ = targetTemp.t1Temp%10 + 48;

			
			*p++ = ' ';*p++ = '@';*p++ = ':';*p++ = '0';*p++ = ' ';*p++ = 'B';*p++ = '@';*p++ = ':';*p++ = '0';*p++ = '\n';

			pushFIFO(&gcodeCmdRxFIFO,&tempBuf[0]);			
		}
		
	}

	if(gcodeLineCnt> 50)
	{
		tempStatus = temp_ok;
	}

}

volatile RECOVER_SD rec_sd;
void udiskBufferInit(void)
{
	memset(udiskBuffer.buffer[0],'\n',sizeof(udiskBuffer.buffer[0]));
	memset(udiskBuffer.buffer[1],'\n',sizeof(udiskBuffer.buffer[1]));
	udiskBuffer.current = 0;
	udiskBuffer.p = udiskBuffer.buffer[udiskBuffer.current];
	udiskBuffer.state[udiskBuffer.current] = udisk_buf_full;
	udiskBuffer.state[(udiskBuffer.current+1)%2] = udisk_buf_empty;

	note_flag = 1;
	gcodeLineCnt = 0;
	RePrintData.record_line = 0;
	
	udiskFileStaus = udisk_file_ok;
	/*----------------*/
	targetTemp.bedTemp = 0;
	targetTemp.t0Temp = 0;
	targetTemp.t1Temp = 0;
	targetTemp.t2Temp = 0;
	tempStatus = temp_fail;
	/*----------------*/

	RePrintData.saveEnable = 0;
	
	initFIFO(&gcodeTxFIFO);
//	initFIFO(&gcodeRxFIFO);
	initFIFO(&gcodeCmdTxFIFO);
	initFIFO(&gcodeCmdRxFIFO);

	//chen 10.8
	rec_sd.read_disk_err = 0;
	rec_sd.just_delay_one = 0;
}

volatile long total;
volatile FRESULT res_ok;
volatile int usb_cnt;
volatile int usb_cnt_bak;
//volatile uint8_t sd_cd=0;
/* ������Ƶ32M,��1K�ļ�ʱ��Ϊ7ms,������Ƶ8M��1K�ļ�ʱ��Ϊ5ms*/
void udiskFileR(FIL *srcfp)		//��ȡu���ļ���д��udiskBuffer
{		
		unsigned int readByteCnt=0;
		if((udiskBuffer.state[(udiskBuffer.current+1)%2] == udisk_buf_full) && (udiskFileStaus == udisk_file_ok))
			return;
		//if(sd_cd==1)
			//return;
/*--------------------------------*/
//FRESULT f_read (
//	FIL *fp, 		/* Pointer to the file object */
//	void *buff,		/* Pointer to data buffer */
//	UINT btr,		/* Number of bytes to read */
//	UINT *br		/* Pointer to number of bytes read */
//)

/*--------------------------------*/	
		SPI1_SetSpeed(SPI_BaudRatePrescaler_32);//���õ� 2.5MHz
		switch(udiskFileStaus)
		{
			case udisk_file_ok:
				res_ok = f_read(srcfp,udiskBuffer.buffer[(udiskBuffer.current+1)%2],UDISKBUFLEN,&readByteCnt);

				//if(res_ok == FR_OK)
				//chen 10.8
				if(res_ok == FR_OK && rec_sd.read_disk_err ==0)
				{
					udiskBuffer.state[(udiskBuffer.current+1)%2] = udisk_buf_full;
					total += readByteCnt;
					if((readByteCnt < UDISKBUFLEN)&&(srcfp->fsize <= total))
					//if(readByteCnt < UDISKBUFLEN)
					{
						udiskFileStaus = udisk_file_end;
						fileEndCnt = 30000; 
						total = 0;
					}
				}
				else
				{
					memset(udiskBuffer.buffer[(udiskBuffer.current+1)%2],'\n',sizeof(udiskBuffer.buffer[(udiskBuffer.current+1)%2]));				
					//if((readByteCnt < UDISKBUFLEN)&&(srcfp->fsize > total))
					if(srcfp->fsize > total)
					{
						//chen 9.29
						//���յ�ok���ٷ���gcode����
						usart2Data.prWaitStatus = pr_wait_idle;
						
						if(rec_sd.just_delay_one == 0)
						tftDelay(2000);  //��ֹ�ظ���ʱ
						
						rec_sd.just_delay_one = 1;
						
						Restart_data_init();
					} 					
				}
					//RePrintData.offset1 =  f_tell(srcfp);	//��ȡ�ļ�ƫ��λ��
				break;
			case udisk_file_end:
					//if((fileEndCnt == 0)||(udiskBuffer.state[0] == udisk_buf_empty && udiskBuffer.state[1] == udisk_buf_empty && checkFIFO(&gcodeTxFIFO)== fifo_empty)) //��ӡ����
					if((udiskBuffer.state[0] == udisk_buf_empty && udiskBuffer.state[1] == udisk_buf_empty && checkFIFO(&gcodeTxFIFO)== fifo_empty)) //��ӡ����
					{
						tftDelay(3);
						printerInit();
						tftDelay(3);

						I2C_EE_Init(100000);

						I2C_EE_BufferRead(&dataToEeprom, BAK_REPRINT_INFO,  4); 
						dataToEeprom &= 0x00ffffff;
						dataToEeprom |= (uint32_t)(printer_normal << 24 ) & 0xff000000;
						I2C_EE_BufferWrite(&dataToEeprom, BAK_REPRINT_INFO,  4); 		// �����־(uint8_t) | ��λunit (uint8_t) | saveFlag(uint8_t)| null(uint8_t)
						
						printerStaus = pr_idle;		//��ӡ����
						usart2Data.printer = printer_idle;
						usart2Data.prWaitStatus = pr_wait_idle;
						usart2Data.timer = timer_stop;						//�����ʱ��

						#ifdef SAVE_FROM_SD
						//ɾ�����������ļ���
						if(gCfgItems.pwroff_save_mode == 0)
						{
							if(gCfgItems.fileSysType == FILE_SYS_SD)
							{
								strcpy(path_bak, "1:");
								strcat(path_bak,path_reprint);						
								f_unlink(path_bak);
							}
							else
							{
								//strcpy(path_bak, "0:");
							}
						}
						#endif
						
						if((gCfgItems.print_finish_close_machine_flg == 1)&&(IsChooseAutoShutdown == 1))
						{
							Close_machine_display();
							IsChooseAutoShutdown = 0;
							#if 0
							//Print_finish_close_machine();
							Btn_putdown_close_machine();
							IsChooseAutoShutdown = 0;
							clear_cur_ui();
							//GUI_SetFont(&FONT_TITLE);
							if(gCfgItems.language == LANG_COMPLEX_CHINESE)
							{
								GUI_SetFont(&GUI_FontHZ16);
							}
							else
							{
								GUI_SetFont(&FONT_TITLE);
							}

							if(gCfgItems.language == LANG_ENGLISH)
							{
								GUI_DispStringAt("Print end! Closing Machine...", 50, 120);
							}
							else 	if(gCfgItems.language == LANG_COMPLEX_CHINESE)
							{
								GUI_DispStringAt("��ӡ���!�����P�C...", 50, 120);
							}
							else
							{
								GUI_DispStringAt("��ӡ���! ���ڹػ�...", 50, 120);
							}
							close_fail_flg = 1;
							close_fail_cnt = 0;
							while(close_fail_flg);
							clear_cur_ui();
							draw_dialog(DIALOG_TYPE_M80_FAIL);
							#endif
						}
					}

					if(udiskBuffer.state[udiskBuffer.current] == udisk_buf_empty)
					{
							udiskBuffer.current = (udiskBuffer.current+1)%2;
							udiskBuffer.p = udiskBuffer.buffer[udiskBuffer.current];
					}

				break;
				default : break;
		}
		SPI1_SetSpeed(SPI_BaudRatePrescaler_8);//���õ� 10MHz
}

extern uint8_t gCurDir[100];


void Restart_data_init()
{
	volatile uint8_t record_cnt;
	volatile uint16_t last_tick;
	
	memset(gCurDir, 0, sizeof(gCurDir));
		
	f_close(srcfp);
	strcpy(gCurDir, "1:");
	SD_Initialize();
	ShowSDFiles();

	res_ok=f_open(srcfp,curFileName,FA_READ);
	if(res_ok == FR_OK)
	{	
		rec_sd.just_delay_one = 0;
		//�ҵ���Ӧ�е��ļ�ƫ��
		do
		{
			for(record_cnt=0;record_cnt<30;record_cnt++)
			{
				if(Gcode_current_position[record_cnt].Gcode_LineNumb == (RePrintData.record_line))
				{
						RePrintData.offset = Gcode_current_position[record_cnt].Gcode_fileOffset;
						break;
				}
			}
		}while(record_cnt == 30 && RePrintData.record_line--);
		
		f_lseek(srcfp, RePrintData.offset);

		initFIFO(&gcodeTxFIFO);
		initFIFO(&gcodeCmdTxFIFO);
		initFIFO(&gcodeCmdRxFIFO);
		
		memset(udiskBuffer.buffer[0],'\n',sizeof(udiskBuffer.buffer[0]));
		memset(udiskBuffer.buffer[1],'\n',sizeof(udiskBuffer.buffer[1]));
		udiskBuffer.current = 0;
		udiskBuffer.p = udiskBuffer.buffer[udiskBuffer.current];
		udiskBuffer.state[udiskBuffer.current] = udisk_buf_full;
		udiskBuffer.state[(udiskBuffer.current+1)%2] = udisk_buf_empty;

		note_flag = 1;
		gcodeLineCnt = 0;
		RePrintData.record_line = 0;
		
		//��ָ��
		while(checkFIFO(&gcodeTxFIFO)!= fifo_full)
		{
			udiskFileR(srcfp);												
			pushTxGcode();	
			/*last_tick=getTick();
			if(getTickDiff(last_tick, getTick()) > 4000)
				break;	*/
		}	
		//��N-1 M110*15
		printerInit();
	}	
}

void pushTxGcode(void)		//��udiskBuffer��������ȡ����Ч��gcodeָ��������кţ�push��gcodeTxFIFO
{
	//tan 20170111
	int8_t *Ztemp;
	unsigned char i=0;

	static unsigned char position_cnt = 0;
	unsigned char numb_cnt = 0;
	
	unsigned char gcode[FIFO_SIZE];		//�洢��udiskBuffer��ȡ��һ��gcode
	unsigned char *p=gcode;				//ָ��gcode��ָ��
	unsigned char gcode_tx[FIFO_SIZE];	//�ɷ��͵�gcodeָ������кź�У����
	unsigned char *p_tx=gcode_tx;		//ָ��gcode_tx��ָ��
	unsigned long gcodeLineCnt_b;		//�ݴ�gcodeLineCnt
	unsigned char lineCntBuf[20];		//�洢�к��ַ���
	unsigned char *p_cnt=lineCntBuf;	
	unsigned char checkSum=0;			//У���
	unsigned char ulockCnt=0;			//��ע�� ��������ֹ ���������ݣ����²��ܴ�udisk��ȡ�ļ����������

	if(checkFIFO(&gcodeTxFIFO)== fifo_full)			//������
		return;

	if(udiskBuffer.state[udiskBuffer.current] == udisk_buf_empty)	//buffer��
		return;

			while(*udiskBuffer.p != '\n'  && *udiskBuffer.p != '\r')	//�н���
			{
				if(p-gcode > (FIFO_SIZE-10))	//һ��ָ��̫������������ע�͵������ַ�
				{
					*(udiskBuffer.p +1)= ';';
					break;
				}

				//if(ulockCnt++ > FIFO_SIZE && p == gcode)		//��ֹ��ע�� �������
				//{
				//	return;
				//}

				

				if(*udiskBuffer.p == ';')	//ȥ�� ';' �����ע��
					note_flag =  0;

				if(note_flag)
					*p++ = *udiskBuffer.p++;	//��ȡ��Чgcodeָ��
				else
					udiskBuffer.p++;

				if(udiskBuffer.p == udiskBuffer.buffer[udiskBuffer.current]+ UDISKBUFLEN)	//��ǰbuffer ��ȡ����,ת������һbuffer
				{
					memset(udiskBuffer.buffer[udiskBuffer.current],'\n',sizeof(udiskBuffer.buffer[0]));		//buffer ������'\n'
					udiskBuffer.state[udiskBuffer.current] = udisk_buf_empty;								//buffer ״̬��empty
					udiskBuffer.current = (udiskBuffer.current+1)%2;										//ת��һ��buffer
					udiskBuffer.p = udiskBuffer.buffer[udiskBuffer.current];								//��ַָ����һ��buffer
				}

				if(ulockCnt++ > FIFO_SIZE && p == gcode)		//��ֹ��ע�� �������
				{
					return;
				}


			}
			udiskBuffer.p++;	//����'\n'�ַ�
			if(udiskBuffer.p == udiskBuffer.buffer[udiskBuffer.current]+ UDISKBUFLEN)	//��ǰbuffer ��ȡ����,ת������һbuffer
				{
					memset(udiskBuffer.buffer[udiskBuffer.current],'\n',sizeof(udiskBuffer.buffer[0]));		//buffer ������'\n'
					udiskBuffer.state[udiskBuffer.current] = udisk_buf_empty;								//buffer ״̬��empty
					udiskBuffer.current = (udiskBuffer.current+1)%2;										//ת��һ��buffer
					udiskBuffer.p = udiskBuffer.buffer[udiskBuffer.current];								//��ַָ����һ��buffer
				}

			note_flag = 1;		

			if(p > gcode)		//��ȡ����gcodeָ��
			{
				while(*(--p) == 32);	//ȥ��gcodeָ������Ŀո�
					p++;
				
				*p_tx++ = 'N';					//��'N'	
				
				gcodeLineCnt_b = gcodeLineCnt;			//���к�
				
			
				*p_cnt++=gcodeLineCnt_b%10 + 48;
				gcodeLineCnt_b /= 10;
				while(gcodeLineCnt_b!=0)
				{
					*p_cnt++=gcodeLineCnt_b%10 + 48;
					gcodeLineCnt_b /= 10;
				}


				while(p_cnt>lineCntBuf)
					*p_tx++ = *--p_cnt;
				
				*p_tx++ = 32;							//�ӿո�

				gcodeLineCnt++;
				//��˫��ͷ�����ж�
				if((gcode[0]=='T')&&(gcode[1]=='0'))
				{
					RePrintData.spayerchoose = 0;
				}
				if((gcode[0]=='T')&&(gcode[1]=='1'))
				{
					RePrintData.spayerchoose = 1;
				}
				//
				getTargetTemp(&gcode[0],p);			//��ȡĿ���¶�
				getFanStatus(&gcode[0],p);				//��ȡ����״̬
				
				p_cnt=gcode;								//��gcodeָ��,��ʱʹ��p_cnt
				while(p_cnt<p)								
				{
				*p_tx++ = *p_cnt++;
				}
				*p_tx++ = '*';										//��'*'

															//��У��
				p_cnt= gcode_tx;
				while(*p_cnt != '*')
					checkSum ^= *p_cnt++;
				
				if(checkSum/100 != 0)				
				{
					*p_tx++ = checkSum/100 + 48;
					*p_tx++ = (checkSum/10)%10 + 48;
					*p_tx++ = checkSum%10 + 48;
				}
				else if(checkSum/10 != 0)
				{
					*p_tx++ = checkSum/10 + 48;
					*p_tx++ = checkSum%10 + 48;
				}
				else
					*p_tx++ = checkSum%10 + 48;
				
				*p_tx++ = '\n';								//��'\n'

				//USART2_CR1 &= 0xff9f;
				pushFIFO(&gcodeTxFIFO,&gcode_tx[0]);			//�����
				//tan 20170111
				//if(gCfgItems.pwroff_save_mode == 0)
				//ֻ��SD�������ļ��ϵ�����ķ�ʽ
				//if(gCfgItems.fileSysType == FILE_SYS_SD)
				{
					Ztemp = (int8_t *)strstr(&gcode_tx[0],"Z");
					if(Ztemp)
					{
						i=0;
						strcpy(gCfgItems.z_display_pos_bak,gCfgItems.z_display_pos);
						memset(gCfgItems.z_display_pos,0,sizeof(gCfgItems.z_display_pos));
						while((*(Ztemp+1+i)!=' ')&&(*(Ztemp+1+i)!='*')&&(*(Ztemp+1+i)!='\r')&&(*(Ztemp+1+i)!='\n'))
						{
						#ifdef SAVE_FROM_SD
							gCfgItems.sd_saving = 1;
							
							gCfgItems.sd_save_flg = 1;
							gcodeLineCntBak1 = gcodeLineCnt;
						#endif
							if((*(Ztemp+1+i) =='+')||(*(Ztemp+1+i) =='-'))
							{
								strcpy(gCfgItems.z_display_pos,gCfgItems.z_display_pos_bak);
								break;
							}
							gCfgItems.z_display_pos[i] = *(Ztemp+1+i);
							i++;
							if(i>20)break;
						}
					}
					/*
					if(gCfgItems.sd_save_flg == 1)
					{
						gcodeLineCntBak2 = gcodeLineCnt;
					}
					if(gcodeLineCntBak2-gcodeLineCntBak1 >= 10)
					{
						gcodeLineCntBak2 = 0;
						gcodeLineCntBak1 = 0;
						gCfgItems.sd_save_flg = 0;
						gCfgItems.sd_saving = 1;
					}
					*/
				}
				RePrintData.offset =  f_tell(srcfp)-UDISKBUFLEN;
				if(udiskBuffer.state[(udiskBuffer.current+1)%2] == udisk_buf_full)
					RePrintData.offset -= UDISKBUFLEN;
				RePrintData.offset += udiskBuffer.p - udiskBuffer.buffer[udiskBuffer.current];
				//USART2_CR1 |= 0x0060;

				//20151012
				Gcode_current_position[position_cnt].Gcode_LineNumb= gcodeLineCnt;
				Gcode_current_position[position_cnt++].Gcode_fileOffset= RePrintData.offset;
				if(position_cnt >= 30)
				{
					position_cnt = 0;
				}
				

			}
}
#ifdef SAVE_FROM_SD
unsigned char path_bak[15]= {0};
unsigned char *path_reprint = "/mks_pft.sys";
FIL fp_reprint_rw;

extern volatile uint8_t per_second_save_sd;
void sd_saved_data()
{
	unsigned char sd_buf_w[100];
	//FIL fp_reprint_w;
	unsigned int bw_repint;
	unsigned char i=0;
	if((gCfgItems.sd_saving)&&(printerStaus == pr_working)&&(per_second_save_sd==1))
	{
		per_second_save_sd=0;
		memset(sd_buf_w,0,sizeof(sd_buf_w));
		sprintf(sd_buf_w,"P:%d|T0:%.2f|T1:%.2f|B:%.2f|FanOn:%d|FanSp:%d|h:%d|m:%d|Z:%s|C:%d|",\
			RePrintData.offset,gCfgItems.desireSprayerTemp[0],gCfgItems.desireSprayerTemp[1],\
			gCfgItems.desireBedTemp,gCfgItems.fanOnoff,gCfgItems.fanSpeed,\
			print_time.hours,print_time.minutes,gCfgItems.z_display_pos,gCfgItems.curSprayerChoose);
		//strcat(sd_buf_w,gCfgItems.z_display_pos);
		//sd_buf_w��ÿ��ֵ����0x4D���(0x4D��Ӧ��ASCII��M)
		
		for(i=0;i<strlen(sd_buf_w);i++)
		{
			sd_buf_w[i] = sd_buf_w[i]^0x4d; 
		}
		
		if(gCfgItems.fileSysType == FILE_SYS_SD)
		{
			strcpy(path_bak, "1:");

			strcat(path_bak,path_reprint);
			if(f_open(&fp_reprint_rw, path_bak, FA_WRITE|FA_OPEN_ALWAYS)== FR_OK)
			{
				f_write(&fp_reprint_rw, sd_buf_w, 100,&bw_repint);
				f_close(&fp_reprint_rw);	
				gCfgItems.sd_saving = 0;
			}
		}
		else
		{
			//strcpy(path_bak, "0:");
		}
	}
}

extern FATFS fs; 

void sd_data_recover()
{
	char *sdstr_temp;
	unsigned char i=0;
	char sdread_temp[20]={0};
	//FIL fp_reprint_r;
	unsigned char sd_buf_r[100];
	unsigned int br_repint;
	
	if(gCfgItems.fileSysType == FILE_SYS_SD)
	{
		f_mount(1, &fs);
		strcpy(path_bak, "1:");

	strcat(path_bak,path_reprint);	
	if(f_open(&fp_reprint_rw, path_bak, FA_READ)== FR_OK)
	{
		memset(sd_buf_r,0,sizeof(sd_buf_r));
		f_read(&fp_reprint_rw, sd_buf_r, 100, &br_repint);
	//sd_buf_r��ÿ��ֵ����0x4D���(0x4D��Ӧ��ASCII��M)
		
		for(i=0;i<strlen(sd_buf_r);i++)
		{
			sd_buf_r[i] = sd_buf_r[i]^0x4d; 
		}
		
		sdstr_temp = strstr(sd_buf_r,"P:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+2+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			RePrintData.offset = atoi(sdread_temp);
			rePrintOffset = RePrintData.offset;
			total = rePrintOffset;
		}
		
		sdstr_temp = strstr(sd_buf_r,"T0:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+3+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+3+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			gCfgItems.desireSprayerTemp[0] = atoi(sdread_temp);
		}	
		sdstr_temp = strstr(sd_buf_r,"T1:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+3+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+3+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			gCfgItems.desireSprayerTemp[1] = atoi(sdread_temp);
		}	
		sdstr_temp = strstr(sd_buf_r,"B:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+2+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			gCfgItems.desireBedTemp = atoi(sdread_temp);
		}	
		sdstr_temp = strstr(sd_buf_r,"FanSp:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+6+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+6+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			gCfgItems.fanSpeed = atoi(sdread_temp);
		}
		sdstr_temp = strstr(sd_buf_r,"FanOn:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+6+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+6+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			gCfgItems.fanOnoff = atoi(sdread_temp);
		}		
		sdstr_temp = strstr(sd_buf_r,"h:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+2+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			print_time.hours = atoi(sdread_temp);
		}
		sdstr_temp = strstr(sd_buf_r,"m:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+2+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			print_time.minutes= atoi(sdread_temp);
		}
		sdstr_temp = strstr(sd_buf_r,"Z:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='|')
			{
				gCfgItems.z_display_pos[i] = *(sdstr_temp+2+i);
				i++;
				if(i>20)break;
			}
			gCfgItems.z_display_pos[i] = 0;
		}
		sdstr_temp = strstr(sd_buf_r,"C:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='|')
			{
				sdread_temp[i] = *(sdstr_temp+2+i);
				i++;
				if(i>20)break;
			}
			sdread_temp[i] = 0;
			gCfgItems.curSprayerChoose = atoi(sdread_temp);
		}		
/*
		sdstr_temp = strstr(sd_buf,"File:");
		if(sdstr_temp)
		{
			i=0;
			while(*(sdstr_temp+2+i)!='\n')
			{
				sdread_temp[i] = *(sdstr_temp+2+i);
				i++;
				if(i>100)break;
			}
			sdread_temp[i] = 0;
			strcpy(sdread_temp,curFileName);
		}			
*/
		f_close(&fp_reprint_rw);

		gCfgItems.rePrintFlag = printer_pwdwn_reprint;
		RePrintData.printerStatus = printer_pwdwn_reprint;
	}
	}
	else
	{
		//f_mount(0, &fs);
		//strcpy(path_bak, "0:");
	}
}
#endif