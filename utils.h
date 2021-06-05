#ifndef _UTILS__H_
#define _UTILS__H_

u32 tnUtilsGetRemoved();
u32 tnUtilsGetCopied();
int tnUtilsRemove(const char *path, int reset);
int tnUtilsCopy(const char *src, const char *dest, int reset);
int tnUtilsGetInfo(const char *path, u32 *size, u32 *folders, u32 *files);

SceUID GetEntries(char *directory, char ***entries, int *n_entry);
void FreeEntries(char **entries, int n_entry);

char *GetSfoStringByName(void *sfo_param, char *name, int n);

int ReadPBPFile(SceUID fd, void **data, u32 *offset);
int ReadISOFile(char *path, void **data, char *file);

u64 ms_free_space();

#endif