#include <pspkernel.h>
#include <string.h>
#include <stdlib.h>

#include "iso.h"

#define SECTOR_SIZE 0x800

#define bin2int(var, addr)                        \
  {                                                 \
    *(((char *)(var)) + 0) = *(((char *)(addr)) + 0);  \
    *(((char *)(var)) + 1) = *(((char *)(addr)) + 1);  \
    *(((char *)(var)) + 2) = *(((char *)(addr)) + 2);  \
    *(((char *)(var)) + 3) = *(((char *)(addr)) + 3);  \
  }                                                 \

int ms_read(void *buf, const char *path, int pos, int size)
{
	SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
	if(fd < 0) return fd;

	sceIoLseek(fd, pos, PSP_SEEK_SET);
	int read = sceIoRead(fd, buf, size);
	sceIoClose(fd);
	return read;
}

int file_read(void *buf, const char *path, int type, int pos, int size)
{
	if(type == 0) return ms_read(buf, path, pos, size);
	else if(type == 1) return cso_read(buf, path, pos, size);
	return -1;
}

int iso_get_file_info(int *pos, int *size, int *size_pos, const char *path, int type, const char *name)
{
	char work[256];
	char s_path[256];
	char s_file[256];
	int path_table_addr;
	int path_table_size;
	int dir_recode_addr = 0;
	short int befor_dir_num = 0x0001;
	int now_dir_num = 1;
	int ret;

	if(*name == '/') name++;

	strcpy(work, name);

	char *ptr = strrchr(work, '/');
	if(ptr != NULL)
	{
		*ptr++ = '\0';
		strcpy(s_path, work);
	}
	else
	{
		s_path[0] = '\0';
		ptr = (char *)work;
	}

	strcpy(s_file, ptr);

///////
	u8 header[8];
	file_read(header, path, type, 0x8000, sizeof(header));

	u8 magic[8] = { 0x01, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01, 0x00 };

	//Invalid Magic
	if(memcmp(header, magic, sizeof(header)) != 0) return -1;
///////

	file_read(&path_table_size, path, type, 0x8084, 4);
	file_read(&path_table_addr, path, type, 0x808C, 4);
	path_table_addr *= SECTOR_SIZE;

	char *table_buf = malloc(path_table_size);
	if(table_buf < 0) return (int)table_buf;

	ret = file_read(table_buf, path, type, path_table_addr, path_table_size);

	if(s_path[0] == '\0')
	{
		bin2int(&dir_recode_addr, &table_buf[2]);
	}
	else
	{
		befor_dir_num = 0x0001;

		int tbl_ptr = 0;
		now_dir_num = 0x0001;
		ptr = s_path;

		while(tbl_ptr < path_table_size)
		{
			u8 len_di = (u8)table_buf[tbl_ptr];
			if(len_di == 0) break;

			tbl_ptr += 6;

			if(befor_dir_num == *(short int *)&table_buf[tbl_ptr])
			{
				tbl_ptr += 2;

				if(strncasecmp(&table_buf[tbl_ptr], ptr, len_di) == 0)
				{
					befor_dir_num = now_dir_num;
					ptr = strchr(ptr, '/');
					if(ptr != NULL) ptr++;
					else
					{
						bin2int(&dir_recode_addr, &table_buf[tbl_ptr - 6]);
						break;
					}
				}
			}
			else
			{
				tbl_ptr += 2;
			}

			tbl_ptr += (len_di + 1) & ~1; // padding
			now_dir_num++;
		}
	}

	free(table_buf);

	if(dir_recode_addr == 0) return -1;

	dir_recode_addr *= SECTOR_SIZE;

	int dir_record_size;

	ret = file_read(&dir_record_size, path, type, dir_recode_addr + 10, 4);

	char *dir_buf = malloc(dir_record_size);
	if(dir_buf < 0) return (int)dir_buf;

	int dir_ptr = 0;
	ret = -1;

	file_read(dir_buf, path, type, dir_recode_addr, dir_record_size);

	while(dir_ptr < dir_record_size)
	{
		u8 len_dr = (u8)dir_buf[dir_ptr];
		if(len_dr == 0)
		{
			dir_ptr++;
		}
		else
		{
			if(strncasecmp(&dir_buf[dir_ptr + 33], s_file, dir_buf[dir_ptr + 32]) == 0)
			{
				bin2int(pos, &dir_buf[dir_ptr + 2]);
				*pos *= SECTOR_SIZE;

				bin2int(size, &dir_buf[dir_ptr + 10]);

				*size_pos = dir_recode_addr + dir_ptr + 10;

				ret = 0;
				break;
			}
			dir_ptr += len_dr;
		}
	}

	free(dir_buf);

	return ret;
}