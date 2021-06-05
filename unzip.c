#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "unzip.h"
#include "unzip/unzip.h"

u32 zip_read = 0;

void ResetZipRead()
{
	zip_read = 0;
}

u32 GetZipRead()
{
	return zip_read;
}

u32 GetZipUncompressedSize(const char *zipfile)
{
	unz_global_info gi;
	unz_file_info file_info;

	unzFile uf = unzOpen(zipfile);
	if(uf == NULL) return -1;

	unzGetGlobalInfo(uf, &gi);

	u32 file_size = 0;

	u64 i;
    for(i = 0; i < gi.number_entry; i++)
    {
		unzGetCurrentFileInfo(uf, &file_info, NULL, 0, NULL, 0, NULL, 0);

		file_size += (u32)file_info.uncompressed_size;

        if((i + 1) < gi.number_entry) unzGoToNextFile(uf);
    }

    unzClose(uf);

    return file_size;
}

void ExtractFileInZip(unzFile uf, const char *filename, u32 size)
{
	unzOpenCurrentFile(uf);

	SceUID fd = sceIoOpen(filename, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(fd >= 0)
	{
		u32 size_buf = 0x10000;
		void *buf = malloc(size_buf);

		u32 remain = 0, seek = 0;
		while((remain = size - seek) > 0)
		{
			remain = (remain < size_buf) ? remain : size_buf;

			if(unzReadCurrentFile(uf, buf, remain) < 0) break;
			sceIoWrite(fd, buf, remain);

			seek += remain;

			zip_read += remain;
		}

		free(buf);

		sceIoClose(fd);
	}

	unzCloseCurrentFile(uf);
}

int ExtractEBOOT(const char *zipfile, char *temp)
{
	unz_global_info gi;
	unz_file_info file_info;

	unzFile uf = unzOpen(zipfile);
	if(uf == NULL) return -1;

	unzGetGlobalInfo(uf, &gi);

	u64 i;
    for(i = 0; i < gi.number_entry; i++)
    {
		unzGetCurrentFileInfo(uf, &file_info, NULL, 0, NULL, 0, NULL, 0);

		char *filename = malloc(file_info.size_filename + 1);
		unzGetCurrentFileInfo(uf, &file_info, filename, file_info.size_filename + 1, NULL, 0, NULL, 0);

		if(strstr(filename, "EBOOT.PBP") || strstr(filename, "FBOOT.PBP"))
		{
			ExtractFileInZip(uf, temp, sizeof(PBPHeader));

			SceUID fd = sceIoOpen(temp, PSP_O_RDONLY, 0);
			if(fd >= 0)
			{
				PBPHeader header;
				sceIoRead(fd, &header, sizeof(PBPHeader));
				sceIoClose(fd);

				ExtractFileInZip(uf, temp, header.snd0_offset);
			}

			free(filename);
			break;
		}

		free(filename);

        if((i + 1) < gi.number_entry) unzGoToNextFile(uf);
    }

    unzClose(uf);

    return 0;
}

int ExtractZip(const char *zipfile, const char *dir)
{
	unz_global_info gi;
	unz_file_info file_info;

	unzFile uf = unzOpen(zipfile);
	if(uf == NULL) return -1;

	unzGetGlobalInfo(uf, &gi);

	sceIoChdir(dir);

	u64 i;
    for(i = 0; i < gi.number_entry; i++)
    {
		unzGetCurrentFileInfo(uf, &file_info, NULL, 0, NULL, 0, NULL, 0);

		char *filename = malloc(file_info.size_filename + 1);
		unzGetCurrentFileInfo(uf, &file_info, filename, file_info.size_filename + 1, NULL, 0, NULL, 0);

		u8 c = filename[strlen(filename) - 1];
		if(c == '/')
		{
			filename[strlen(filename) - 1] = '\0';
			sceIoMkdir(filename, 0777);
		}
		else
		{
			ExtractFileInZip(uf, filename, (u32)file_info.uncompressed_size);
		}

		free(filename);

        if((i + 1) < gi.number_entry) unzGoToNextFile(uf);
    }

    unzClose(uf);

    return 0;
}