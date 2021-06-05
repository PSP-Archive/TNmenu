#include <pspkernel.h>
#include <oslib/oslib.h>

#include "main.h"
#include "cache.h"
#include "utils.h"
#include "unzip.h"

EntryCache *CreateEntryCache(char **entries, int n_entry, int *n_entrycache)
{
	EntryCache *cache = malloc(n_entry * sizeof(EntryCache));
	memset(cache, 0, n_entry * sizeof(EntryCache));

	int j = 0;

	void *icon0, *pic1;

	int i;
	for(i = 0; i < n_entry; i++)
	{
		if(strstr(entries[i], "ms0:/PSP/GAME/"))
		{
			/* Open EBOOT.PBP */
			char path[256];
			sprintf(path, "%s/EBOOT.PBP", entries[i]);

			SceUID fd = sceIoOpen(path, PSP_O_RDONLY, 0);
			if(fd >= 0)
			{
				/* Read PBP header */
				PBPHeader pbp_header;
				sceIoRead(fd, &pbp_header, sizeof(PBPHeader));

				/* Get SFO infos */
				void *sfo_param;
				if(ReadPBPFile(fd, &sfo_param, &pbp_header.param_offset) > 0)
				{
					cache[j].path = malloc(strlen(path) + 1);
					strcpy(cache[j].path, path);

					cache[j].title = GetSfoStringByName(sfo_param, "TITLE", 52);
					cache[j].category = GetSfoStringByName(sfo_param, "CATEGORY", 3);
					free(sfo_param);

					/* Read PIC1.PNG */
					if(config.show_pic1)
					{
						int pic1_size = ReadPBPFile(fd, &pic1, &pbp_header.pic1_offset);
						if(pic1_size)
						{
							oslSetTempFileData(pic1, pic1_size, &VF_MEMORY);
							cache[j].pic1_image = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
							free(pic1);
						}
					}

					/* Read ICON0.PNG */
					cache[j].icon0_image = default_image;
					int icon0_size = ReadPBPFile(fd, &icon0, &pbp_header.icon0_offset);
					if(icon0_size)
					{
						oslSetTempFileData(icon0, icon0_size, &VF_MEMORY);
						OSL_IMAGE *img = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
						free(icon0);

						if(img)
						{
							cache[j].icon0_image = img;
							oslSetImageRotCenter(cache[j].icon0_image);
						}
					}

					cache[j].sizeX = cache[j].icon0_image->sizeX;
					cache[j].sizeY = cache[j].icon0_image->sizeY;

					j++;
				}

				sceIoClose(fd);
			}
		}
		else if(strstr(entries[i], ".iso") || strstr(entries[i], ".ISO") ||
				strstr(entries[i], ".cso") || strstr(entries[i], ".CSO"))
		{
			/* Get SFO infos */
			void *sfo_param;
			if(ReadISOFile(entries[i], &sfo_param, "PSP_GAME/PARAM.SFO") > 0)
			{
				cache[j].path = malloc(strlen(entries[i]) + 1);
				strcpy(cache[j].path, entries[i]);

				cache[j].title = GetSfoStringByName(sfo_param, "TITLE", 52);
				cache[j].category = GetSfoStringByName(sfo_param, "CATEGORY", 3);
				free(sfo_param);

				/* Read PIC1.PNG */
				if(config.show_pic1)
				{
					int pic1_size = ReadISOFile(cache[j].path, &pic1, "PSP_GAME/PIC1.PNG");
					if(pic1_size)
					{
						oslSetTempFileData(pic1, pic1_size, &VF_MEMORY);
						cache[j].pic1_image = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
						free(pic1);
					}
				}

				/* Read ICON0.PNG */
				cache[j].icon0_image = default_image;
				int icon0_size = ReadISOFile(cache[j].path, &icon0, "PSP_GAME/ICON0.PNG");
				if(icon0_size)
				{
					oslSetTempFileData(icon0, icon0_size, &VF_MEMORY);
					OSL_IMAGE *img = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
					free(icon0);

					if(img)
					{
						cache[j].icon0_image = img;
						oslSetImageRotCenter(cache[j].icon0_image);
					}
				}

				cache[j].sizeX = cache[j].icon0_image->sizeX;
				cache[j].sizeY = cache[j].icon0_image->sizeY;

				j++;
			}
        }
		else if(strstr(entries[i], ".zip") || strstr(entries[i], ".ZIP"))
		{
			static char *temp = "ms0:/__TEMP__";
			ExtractEBOOT(entries[i], temp);

			/* Open EBOOT.PBP */
			SceUID fd = sceIoOpen(temp, PSP_O_RDONLY, 0);
			if(fd >= 0)
			{
				/* Read PBP header */
				PBPHeader pbp_header;
				sceIoRead(fd, &pbp_header, sizeof(PBPHeader));

				/* Get SFO infos */
				void *sfo_param;
				if(ReadPBPFile(fd, &sfo_param, &pbp_header.param_offset) > 0)
				{
					cache[j].path = malloc(strlen(entries[i]) + 1);
					strcpy(cache[j].path, entries[i]);

					cache[j].category = malloc(strlen("ZIP") + 1);
					strcpy(cache[j].category, "ZIP");

					cache[j].title = GetSfoStringByName(sfo_param, "TITLE", 52);
					free(sfo_param);

					/* Read PIC1.PNG */
					if(config.show_pic1)
					{
						int pic1_size = ReadPBPFile(fd, &pic1, &pbp_header.pic1_offset);
						if(pic1_size)
						{
							oslSetTempFileData(pic1, pic1_size, &VF_MEMORY);
							cache[j].pic1_image = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
							free(pic1);
						}
					}

					/* Read ICON0.PNG */
					cache[j].icon0_image = default_image;
					int icon0_size = ReadPBPFile(fd, &icon0, &pbp_header.icon0_offset);
					if(icon0_size)
					{
						oslSetTempFileData(icon0, icon0_size, &VF_MEMORY);
						OSL_IMAGE *img = oslLoadImageFilePNG(oslGetTempFileName(), OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
						free(icon0);

						if(img)
						{
							cache[j].icon0_image = img;
							oslSetImageRotCenter(cache[j].icon0_image);
						}
					}

					cache[j].sizeX = cache[j].icon0_image->sizeX;
					cache[j].sizeY = cache[j].icon0_image->sizeY;

					j++;
				}

				sceIoClose(fd);

				sceIoRemove(temp);
			}
		}
	}

	*n_entrycache = j;

	return cache;
}

void FreeEntryCache(EntryCache *cache, int n_entrycache)
{
	int i;
	for(i = 0; i < n_entrycache; i++)
	{
		free(cache[i].path);
		free(cache[i].title);
		free(cache[i].category);
		if(cache[i].pic1_image) oslDeleteImage(cache[i].pic1_image);
		if(cache[i].icon0_image) oslDeleteImage(cache[i].icon0_image);
	}

	free(cache);
}

int CreateCache(EntryCache **cache)
{
	char **entries = NULL; //must be NULL
	int n_entry = 0;
	GetEntries(savedata_path, &entries, &n_entry);
	GetEntries("ms0:/ISO/", &entries, &n_entry);
	GetEntries("ms0:/PSP/GAME/", &entries, &n_entry);

	int n_entrycache = 0;
	*cache = CreateEntryCache(entries, n_entry, &n_entrycache);

	FreeEntries(entries, n_entry);

	return n_entrycache;
}