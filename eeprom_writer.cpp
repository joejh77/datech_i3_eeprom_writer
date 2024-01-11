// file_checksum_add.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "bb_micom.h"
#include "datypes.h"

#define MAX_RETRY_CNT	2
/*
EEPROM MAP

	EEPROM_INIT_TIME = 0,
	EEPROM_SERIAL_NO,
	EEPROM_HW_CONFIG,

	EEPORM_AUTO_ID, //pai-r use
-----------------------
# (U32)Address	value	Auto-increase
	0						0			0
	1						0			1
	2						0			0
	3						0			0
*/

typedef struct{
	u32 reg;
    u32 value;
	u32	auto_inc;
}ST_EEPROM_DATA, *LPST_EEPROM_DATA;

typedef std::list<ST_EEPROM_DATA>					EEPROM_DATA_POOL;
typedef EEPROM_DATA_POOL::iterator					ITER_EEPROM_DATA;

int eeprom_data_get_list(FILE *file, EEPROM_DATA_POOL& list, char * map_file_buffer, int * map_file_length)
{
	int string_length = 0;
	char msg_buffer[10 * 1024] = {0, };
	ST_EEPROM_DATA data;

	char strLine[1024] = { 0,};
	u32 strLineIndex = 0;
	int msg_size = 0;
	int data_count = 0;
	int i;

	fseek( file, 0, SEEK_SET );
	msg_size =  fread( (void *)msg_buffer, 1, sizeof(msg_buffer), file);
	
	
	for(i=0, strLineIndex=0; i < msg_size; i++){
		if( msg_buffer[i] == '\r' || msg_buffer[i] == '\n' || msg_buffer[i] == '\0'){
			strLine[strLineIndex]  = 0;

			if(strLineIndex == 0)
				continue;

			if(sscanf(strLine, "%u\t%u\t%u", &data.reg, &data.value, &data.auto_inc) == 3) {
				printf("%u	%u	%u\r\n", data.reg, data.value, data.auto_inc);
				
				list.push_back(data);
				memset((void *)&data, 0, sizeof(data));
				data_count++;
			}
			else {
				string_length += sprintf(&map_file_buffer[string_length], "%s\r\n", strLine);
			}
			
			strLineIndex = 0;
		}
		else if(strLineIndex < sizeof(strLine)-1)
			strLine[strLineIndex++] = msg_buffer[i];
	}

	*map_file_length = string_length;
	
	return data_count;
}

int main(int argc, char* argv[])
{
	int map_file_update = 0;
	int map_file_length = 0;
	char map_file_buffer[10 * 1024] = {0, };
	
	EEPROM_DATA_POOL data_list;
	u8 slave_address = 0;
	const char * i2c_driver;
	const char * eeprom_map_path;
	char *stop;
	
	if(argc  < 2)
	{
		///dev/i2c-0 slave address 0x8
		printf("USAGE: %s [eeprom map file path]\r\n", argv[0]);
		printf("EXAMPLE: %s /mnt/extsd/eeprom_map.txt\r\n", argv[0]);
		return 0;
	}
	
	eeprom_map_path = argv[1];

	FILE *file = fopen(eeprom_map_path, "rb+");
	if(file){
		int file_size = 0;
		int retry = 0;

		CBbMicom sub_mcu;

		sub_mcu.Initialize();	
		sub_mcu.SetWatchdogTme(60);

				
		fseek( file, 0, SEEK_END );
		file_size = ftell( file );
		fseek( file, 0, SEEK_SET );

		printf("file size : %d Byte\r\n", file_size);

		if(eeprom_data_get_list(file, data_list, map_file_buffer, &map_file_length)){
			int tr_size = data_list.size();
			if(tr_size){
				
				
				ITER_EEPROM_DATA iTI_S = data_list.begin();
								
				for(int i = 0; i < tr_size ; i++, iTI_S++){
					u32 read_data = sub_mcu.GetEepromData(iTI_S->reg);

					usleep(50 * 1000);
					
					if(read_data != iTI_S->value && (!iTI_S->auto_inc || read_data != (iTI_S->value - 1) || read_data == 0 )){
						do{
							sub_mcu.SetEepromData(iTI_S->reg, iTI_S->value);

							read_data = sub_mcu.GetEepromData(iTI_S->reg);

							if(read_data != iTI_S->value){
								printf("WRITE ERROR!	Reg:%ul, DATA: %u (%u)\r\n", iTI_S->reg, read_data, iTI_S->value);
								system("aplay /data/wav/event.wav");
							}
							else {
								printf("WRITE Reg:%u, DATA:%u\r\n", iTI_S->reg, read_data);

								if(iTI_S->auto_inc){
									map_file_update++;
									iTI_S->value++;
								}
										
								break;
							}
						}while(retry++ < MAX_RETRY_CNT);
					}
					else {
						printf("Reg:%u, DATA:%u\r\n", iTI_S->reg, read_data);
					}

					map_file_length += sprintf(&map_file_buffer[map_file_length], "%u\t%u\t%u\r\n", iTI_S->reg, iTI_S->value, iTI_S->auto_inc);
				}
			}
		}

		fclose(file);

		if(map_file_update && map_file_length){
			file = fopen(eeprom_map_path, "wb");
			
			fwrite((void *)map_file_buffer, 1, map_file_length + 1, file);
			printf("write file :\r\n%s\r\n", map_file_buffer);
			fclose(file);

			system("aplay /data/wav/WAVE1.wav");
			system("aplay /data/wav/WAVE2.wav");
			system("aplay /data/wav/WAVE3.wav");			
		}
		else if(retry >= MAX_RETRY_CNT){
			sub_mcu.SetWatchdogTme(0);
			printf("go to rebooting...\r\n");
			sub_mcu.SetPowerMode(BB_MICOM_POWER_OFF_FALL);
			sleep(1);
			system("reboot");
			sleep(1);
		}
	}
	else {
		printf("file Open Error! : %s \r\n", eeprom_map_path);
	}

	return 0;
}

