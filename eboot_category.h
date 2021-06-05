typedef struct
{
	char *category;
	char *key;
	int apitype;
	char *argp;
} EbootCategory;

EbootCategory eboot_category[] =
{
	{  "EG", "umdemu", PSP_INIT_APITYPE_UMDEMU_MS1, EBOOT_BIN },
	{  "MG", "game", PSP_INIT_APITYPE_MS2, NULL },
	{  "ME", "pops", PSP_INIT_APITYPE_MS5, NULL },
};

#define EBOOT_CATEGORY_N (sizeof(eboot_category) / sizeof(EbootCategory))