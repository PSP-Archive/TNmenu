#ifndef _UNZIP_H_
#define _UNZIP_H_

void ResetZipRead();
u32 GetZipRead();

u32 GetZipUncompressedSize(const char *zipfile);
int ExtractEBOOT(const char *zipfile, char *temp);
int ExtractZip(const char *zipfile, const char *dir);

#endif