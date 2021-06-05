#ifndef __CACHE_H__
#define __CACHE_H__

typedef struct
{
	char *path;
	char *title;
	char *category;
	OSL_IMAGE *pic1_image;
	OSL_IMAGE *icon0_image;
	float sizeX;
	float sizeY;
} EntryCache;

EntryCache *CreateEntryCache(char **entries, int n_entry, int *n_entrycache);
void FreeEntryCache(EntryCache *cache, int n_entrycache);
int CreateCache(EntryCache **cache);

#endif