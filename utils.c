#include <pspkernel.h>
#include "main.h"

u32 removed = 0;
u32 copied = 0;

int tnUtilsGetInfo(const char *path, u32 *size, u32 *folders, u32 *files)
{
	SceUID dfd = sceIoDopen(path);
	if(dfd >= 0)
	{
		struct SceIoDirent dir;
		memset(&dir, 0, sizeof(SceIoDirent));

		while(sceIoDread(dfd, &dir) > 0)
		{
			if(dir.d_name[0] != '.')
			{
				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				sprintf(new_path, "%s/%s", path, dir.d_name);

				int res = tnUtilsGetInfo(new_path, size, folders, files);

				free(new_path);

				if(res < 0) return res;
			}

			memset(&dir, 0, sizeof(SceIoDirent));
		}

		sceIoDclose(dfd);

		(*folders)++;
	}
	else
	{
		SceIoStat stat;
		memset(&stat, 0, sizeof(SceIoStat));

		int res = sceIoGetstat(path, &stat);
		if(res < 0) return res;

		(*size) += stat.st_size;
		(*files)++;
	}

	return 0;
}

u32 tnUtilsGetRemoved()
{
	return removed;
}

int tnUtilsRemove(const char *path, int reset)
{
	if(reset) removed = 0;

	SceUID dfd = sceIoDopen(path);
	if(dfd >= 0)
	{
		struct SceIoDirent dir;
		memset(&dir, 0, sizeof(SceIoDirent));

		while(sceIoDread(dfd, &dir) > 0)
		{
			if(dir.d_name[0] != '.')
			{
				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				sprintf(new_path, "%s/%s", path, dir.d_name);

				int res = tnUtilsRemove(new_path, 0);

				free(new_path);

				if(res < 0) return res;
			}

			memset(&dir, 0, sizeof(SceIoDirent));
		}

		sceIoDclose(dfd);

		sceIoRmdir(path);
		removed++;
	}
	else
	{
		sceIoRemove(path);
		removed++;
	}

	return 0;
}

u32 tnUtilsGetCopied()
{
	return copied;
}

int tnUtilsCopy(const char *src, const char *dest, int reset)
{
	if(reset) copied = 0;

	SceUID dfd = sceIoDopen(src);
	if(dfd >= 0)
	{
		struct SceIoDirent dir;
		memset(&dir, 0, sizeof(SceIoDirent));

		while(sceIoDread(dfd, &dir) > 0)
		{
			if(dir.d_name[0] != '.')
			{
				char *src_path = malloc(strlen(src) + strlen(dir.d_name) + 2);
				sprintf(src_path, "%s/%s", src, dir.d_name);

				char *dest_path = malloc(strlen(dest) + strlen(dir.d_name) + 2);
				sprintf(dest_path, "%s/%s", dest, dir.d_name);

				int res = tnUtilsCopy(src_path, dest_path, 0);

				free(dest_path);
				free(src_path);

				if(res < 0) return res;
			}

			memset(&dir, 0, sizeof(SceIoDirent));
		}

		sceIoDclose(dfd);

		sceIoMkdir(dest, 0777);
	}
	else
	{
		SceUID fdsrc = sceIoOpen(src, PSP_O_RDONLY, 0);
		if(fdsrc < 0) return fdsrc;

		SceUID fddest = sceIoOpen(dest, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
		if(fddest < 0) return fddest;

		u32 size_buf = 0x10000;
		void *buf = malloc(size_buf);

		int read;
		while((read = sceIoRead(fdsrc, buf, size_buf)) > 0)
		{
			sceIoWrite(fddest, buf, read);
			copied += read;
		}

		free(buf);

		sceIoClose(fddest);
		sceIoClose(fdsrc);
	}

	return 0;
}

SceUID GetEntries(char *directory, char ***entries, int *n_entry)
{
	SceUID dfd = sceIoDopen(directory);
	if(dfd < 0) return dfd;

	struct SceIoDirent dir;
	memset(&dir, 0, sizeof(SceIoDirent));

	while(sceIoDread(dfd, &dir) > 0)
	{
		if(dir.d_name[0] != '.')
		{
			*entries = realloc(*entries, (*n_entry + 1) * sizeof(char **));

			(*entries)[*n_entry] = malloc(strlen(directory) + strlen(dir.d_name) + 1);
			strcpy((*entries)[*n_entry], directory);
			strcat((*entries)[*n_entry], dir.d_name);

			(*n_entry)++;
		}

		memset(&dir, 0, sizeof(SceIoDirent));
	}

	sceIoDclose(dfd);

	return 0;
}

void FreeEntries(char **entries, int n_entry)
{
	int i;
	for(i = 0; i < n_entry; i++) free(entries[i]);	
	free(entries);
}

char *GetSfoStringByName(void *sfo_param, char *name, int n)
{
	char *string = malloc(n);

	SFOHeader *header = (SFOHeader *)sfo_param;
	SFOEntry *entries = (SFOEntry *)((u32)header + sizeof(SFOHeader));

	int i;
	for(i = 0; i < header->count; i++)
	{
		if(strcmp((char *)((u32)header + header->keyofs + entries[i].nameofs), name) == 0)
		{
			memset(string, 0, n);
			strncpy(string, (void *)((u32)header + header->valofs + entries[i].dataofs), n);
			string[n-1] = '\0';
		}
	}

	return string;
}

int ReadPBPFile(SceUID fd, void **data, u32 *offset)
{
	int size = offset[1] - offset[0];
	*data = malloc(size);
	sceIoLseek32(fd, *offset, PSP_SEEK_SET);
	return sceIoRead(fd, *data, size);
}

int ReadISOFile(char *path, void **data, char *file)
{
	int type = 0;

	u32 magic;
	ms_read(&magic, path, 0, sizeof(magic));
	if(magic == 0x4F534943) type = 1;

	int pos, size, size_pos;
	int ret = iso_get_file_info(&pos, &size, &size_pos, path, type, file);
	if(ret < 0) return ret;

	*data = malloc(size);
	file_read(*data, path, type, pos, size);

	return size;
}

typedef struct
{
	u32 max_clusters;
	u32 free_clusters;
	u32 max_sectors;
	u32 sector_size;
	u32 sector_count;
} devInfo;

u64 ms_free_space()
{
	devInfo devinfo;
	devInfo *info = &devinfo;
	sceIoDevctl("ms0:", 0x02425818, &info, sizeof(info), NULL, 0);

	return (u64)devinfo.free_clusters * (u64)devinfo.sector_count * (u64)devinfo.sector_size;
}