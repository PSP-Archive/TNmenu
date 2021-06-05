#include <pspkernel.h>
#include <zlib.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ciso.h"

#define SECTOR_SIZE 0x800

int inflate_cso(char *o_buff, int o_size, const char *i_buff, int i_size)
{
	z_stream z;
	int size;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;
	z.next_in = Z_NULL;
	z.avail_in = 0;
	if(inflateInit2(&z, -15) != Z_OK) return -1;

	z.next_in = (u8 *)i_buff;
	z.avail_in = i_size;
	z.next_out = (u8 *)o_buff;
	z.avail_out = o_size;

	inflate(&z, Z_FINISH);

	size = o_size - z.avail_out;

	if(inflateEnd(&z) != Z_OK) return -1;

	return size;
}

int cso_read_fd(char *buf, SceUID fd, int pos, int size)
{
	static CISO_H ciso;
	int index = 0;
	int index2 = 0;
	char tmp_buf[SECTOR_SIZE * 2];
	char tmp_buf_2[SECTOR_SIZE * 2];
	int ret;
	int err;

	err = sceIoLseek(fd, 0, PSP_SEEK_SET);
	if(err < 0) return err;

	err = sceIoRead(fd, &ciso, sizeof(ciso));
	if(err < 0) return err;

	if((pos + size) > ciso.total_bytes) size = ciso.total_bytes - pos;

	int max_sector = ciso.total_bytes / ciso.block_size - 1;
	int start_sec = pos / SECTOR_SIZE;
	int end_sec = (pos + size - 1) / SECTOR_SIZE;
	int sector_num = start_sec;

	if(sector_num > max_sector) return -1;

	if(end_sec > max_sector) end_sec = max_sector;

	ret = 0;
	while(sector_num <= end_sec)
	{
		err = sceIoLseek(fd, sizeof(ciso) + (sector_num * 4), PSP_SEEK_SET);
		if(err < 0) return err;

		err = sceIoRead(fd, &index, 4);
		if(err < 0) return err;

		u32 zip_flag = index & 0x80000000;
		index = (index & 0x7FFFFFFF) << ciso.align;

		err = sceIoRead(fd, &index2, 4);
		if(err < 0) return err;

		int read_size = ((index2 & 0x7FFFFFFF) << ciso.align) - index;

		err = sceIoLseek(fd, index, PSP_SEEK_SET);
		if(err < 0) return err;

		if(zip_flag != 0)
		{
			err = sceIoRead(fd, tmp_buf, ciso.block_size);
			if(err < 0) return err;
		}
		else
		{
			err = sceIoRead(fd, tmp_buf_2, read_size);
			if(err < 0) return err;
			err = inflate_cso(tmp_buf, ciso.block_size, tmp_buf_2, read_size);
			if(err < 0) return err;
		}

		if((sector_num > start_sec) && (sector_num < end_sec))
		{
			memcpy(buf, tmp_buf, ciso.block_size);
			read_size = ciso.block_size;
		}
		else if((sector_num == start_sec) || (sector_num == end_sec))
		{
			int start_pos = 0;
			int end_pos = ciso.block_size;
			if(sector_num == start_sec) start_pos = pos - (start_sec * ciso.block_size);
			if(sector_num == end_sec) end_pos = (pos + size) - (end_sec * ciso.block_size);
			read_size = end_pos - start_pos;
			memcpy(buf, &tmp_buf[start_pos], read_size);
		}

		buf += read_size;
		ret += read_size;
		sector_num++;
	}

	return ret;
}

int cso_read(char *buf, const char *path, int pos, int size)
{
	SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
	if(fd < 0) return fd;

	int ret = cso_read_fd(buf, fd, pos, size);
	sceIoClose(fd);

	return ret;
}