#ifndef __MAIN_H__
#define __MAIN_H__

#include <oslib/oslib.h>

#define log(...) { char msg[256]; sprintf(msg,__VA_ARGS__); logmsg(msg); }

extern OSL_FONT *ltn0;
extern OSL_IMAGE *default_image;
extern char savedata_path[64];

extern float option_menu_scroll_y;
extern float comment_scroll;

enum scroll_modes
{
	SCROLL_DONE = 0,
	SCROLL_BACKWARD,
	SCROLL_FORWARD,
};

enum colors
{
	WHITE =	0xFFFFFFFF,
	BLACK = 0xFF000000,
};

typedef struct
{
	char nickname[32];
	int button_assign;
	int cpu_speed;
	int iso_driver;
	int hide_cfw_folders;
	int exit_button_1;
	int exit_button_2;
	int exit_hold_duration;
	int show_pic1;
} TNConfig;

extern TNConfig config;

typedef struct
{
	u32 header;
	u32 version;
	u32 param_offset;
	u32 icon0_offset;
	u32 icon1_offset;
	u32 pic0_offset;
	u32 pic1_offset;
	u32 snd0_offset;
	u32 elf_offset;
	u32 PSAR_offset;
} PBPHeader;

typedef struct
{
	u32 magic;
	u32 version;
	u32 keyofs;
	u32 valofs;
	u32 count;
} __attribute__((packed)) SFOHeader;

typedef struct
{
	u16 nameofs;
	u8 alignment;
	u8 type;
	u32 valsize;
	u32 totalsize;
	u32 dataofs;
} __attribute__((packed)) SFOEntry;

#define EBOOT_BIN "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN"

int sctrlSEGetConfig(TNConfig *config);
int sctrlSESetConfig(TNConfig *config);
int sctrlSEMountUmdFromFile(char *file, int noumd, int isofs);

void DrawMenu();

#endif