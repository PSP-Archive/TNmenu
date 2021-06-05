#include <pspkernel.h>
#include <psprtc.h>
#include <psploadexec_kernel.h>
#include <systemctrl.h>
#include <oslib/oslib.h>

#include "main.h"
#include "option_menu.h"
#include "cache.h"
#include "utils.h"
#include "utility.h"
#include "unzip.h"

#include "eboot_category.h"

#include "options.h"

PSP_MODULE_INFO("TNMenu", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_MAX();

char savedata_path[64];

OSL_FONT *ltn0;
OSL_IMAGE *background_image, *bar_image, *option_image, *default_image;
OSL_IMAGE *pb_skin_image, *pb_bar_image;
OSL_IMAGE *bat_skin_image, *bat_bar_image;

u8 pic1_alpha = 0;

//menu vars
int menu_move = SCROLL_DONE, menu_sel = 0, old_menu_sel = -1;

//y-scroll vars
float menu_scroll_y = 0, old_menu_scroll_y = 0;
float option_menu_scroll_y = 250;

//text scroll vars
float title_scroll = 10;
float comment_scroll = 10;

EntryCache *cache = NULL;
int n_entrycache = 0;

char free_space[64];

TNConfig config;

#define COLS 3

#define OPTION_MENU_BEGIN 26
#define OPTION_MENU_END 250

#define OPTION_MENU_INTERVAL 16
#define MENU_INTERVAL 20
#define PAGE_INTERVAL 15
#define PAGE_DISTANCE 100

#define SHOW_MAX_OPTIONS 7
#define SCROLL_MAX_OPTIONS 6

int exit_callback(int arg1, int arg2, void *common)
{
	osl_quit = 1;
	return 0;
}

int CallbackThread(SceSize args, void *argp)
{
    SceUID cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int SetupCallbacks()
{
	SceUID thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
	if(thid >= 0) sceKernelStartThread(thid, 0, 0);
	return thid;
}

int initOSLib()
{
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslSetKeyAutorepeatInit(10);
    oslSetKeyAutorepeatInterval(5);
    oslIntraFontInit(INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8);

	SetupCallbacks();

	/* Load font */
    ltn0 = oslLoadFontFile("flash0:/font/jpn0.pgf");

	/* Load images */
	sceIoChdir(savedata_path);
	ExtractZip("DATA.TN", "ms0:/");

	sceIoChdir("ms0:/temp/");

	pb_skin_image = oslLoadImageFilePNG("progressbar_skin.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	pb_bar_image = oslLoadImageFilePNG("progressbar_bar.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	bat_skin_image = oslLoadImageFilePNG("battery_skin.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	bat_bar_image = oslLoadImageFilePNG("battery_bar.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

	background_image = oslLoadImageFilePNG("background.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	bar_image = oslLoadImageFilePNG("bar.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	option_image = oslLoadImageFilePNG("option.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
	default_image = oslLoadImageFilePNG("default.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

	oslSetImageRotCenter(default_image);

	tnUtilsRemove("ms0:/temp", 1);

    return 0;
}

void ResetMenu()
{
	/* Recreate cache */
	FreeEntryCache(cache, n_entrycache);
	n_entrycache = CreateCache(&cache);

	/* reset variables */
	menu_sel = 0;
	old_menu_sel = -1;
	menu_scroll_y = 0;
	old_menu_scroll_y = 0;

	/* get ms size */
	u64 size = ms_free_space();

	u64 sz, szd;
	if(size > 1024 * 1024 * 1024)
	{
		sz = size / (u64)(1024 * 1024 * 1024);
		szd = (size / (u64)(1024 * 1024) - sz * 1024) % 1024 / (u64)(10);
		sprintf(free_space, "%u.%02u GB", (u32)sz, (u32)szd);
	}
	else if(size > 1024 * 1024)
	{
		sz = size / (u64)(1024 * 1024);
		szd = (size / (u64)(1024) - sz * 1024) % 1024 / (u64)(10);
		sprintf(free_space, "%u.%02u MB", (u32)sz, (u32)szd);
	}
	else if(size > 1024)
	{
		sz = size / (u64)(1024);
		szd = (size - sz * 1024) % 1024 / (u64)(10);
		sprintf(free_space, "%u.%02u KB", (u32)sz, (u32)szd);
	}
	else
	{
		sprintf(free_space, "%u Bytes", (u32)size);
	}
}

void DrawBar()
{
	/* Draw title */
	oslSetAlpha(OSL_FX_ALPHA, 250);
	oslDrawImage(bar_image);
	oslSetAlpha(OSL_FX_DEFAULT, 0);

	oslIntraFontSetStyle(ltn0, 0.9f, WHITE, 0, INTRAFONT_SCROLL_LEFT);
	oslSetFont(ltn0);
	title_scroll = oslIntraFontPrintColumn(ltn0, title_scroll, 8, 440, 0, cache[menu_sel].title);

	/* Draw battery */
	float progress = (float)(((float)bat_bar_image->sizeX * (float)scePowerGetBatteryLifePercent()) / (float)(100));
	bat_bar_image->stretchX = (u32)progress;

	oslDrawImageXY(bat_skin_image, 480 - 10 - bat_skin_image->sizeX, (bar_image->sizeY - bat_skin_image->sizeY) / 2);
	oslDrawImageXY(bat_bar_image, 480 - 10 - 2 - bat_bar_image->sizeX, (bar_image->sizeY - bat_bar_image->sizeY) / 2);

	/* Draw bar */
	oslSetAlpha(OSL_FX_ALPHA, 250);
	oslDrawImageXY(option_image, 0, option_menu_scroll_y);
	oslSetAlpha(OSL_FX_DEFAULT, 0);

	/* Draw TN Menu */
	oslIntraFontSetStyle(ltn0, 0.6f, WHITE, 0, INTRAFONT_ALIGN_LEFT);
	oslSetFont(ltn0);
	oslDrawString(10, option_menu_scroll_y + 4, "TN Menu 1.0");

	/* Draw time */
	pspTime time;
	sceRtcGetCurrentClockLocalTime(&time);

	oslIntraFontSetStyle(ltn0, 0.8f, WHITE, 0, INTRAFONT_ALIGN_CENTER);
	oslSetFont(ltn0);
	oslDrawStringf(240, option_menu_scroll_y + 6, "%02d:%02d", time.hour, time.minutes);

	/* Draw free space */
	oslIntraFontSetStyle(ltn0, 0.6f, WHITE, 0, INTRAFONT_ALIGN_RIGHT);
	oslSetFont(ltn0);
	oslDrawString(480-10, option_menu_scroll_y + 4, free_space);
}

void DrawMenu()
{
	oslDrawImage(background_image);

	if(old_menu_sel == menu_sel)
	{
		if(config.show_pic1)
		{
			if(cache[menu_sel].pic1_image)
			{
				oslSetAlpha(OSL_FX_ALPHA, pic1_alpha);
				oslDrawImage(cache[menu_sel].pic1_image);
				oslSetAlpha(OSL_FX_DEFAULT, 0);

				if(pic1_alpha < 0xFF) pic1_alpha += 5;
			}
		}
	}
	else
	{
		old_menu_sel = menu_sel;
		pic1_alpha = 0;

		title_scroll = 10;
	}

	switch(menu_move)
	{
		case SCROLL_BACKWARD:
			if(menu_scroll_y < old_menu_scroll_y + 80)
			{
				menu_scroll_y += MENU_INTERVAL;
			}
			else
			{
				menu_sel -= COLS;
				menu_move = SCROLL_DONE;
				old_menu_scroll_y = menu_scroll_y;
			}
			break;

		case SCROLL_FORWARD:
			if(menu_scroll_y > old_menu_scroll_y - 80)
			{
				menu_scroll_y -= MENU_INTERVAL;
			}
			else
			{
				menu_sel += COLS;
				menu_move = SCROLL_DONE;
				old_menu_scroll_y = menu_scroll_y;
			}
			break;
	}

	int i;
	for(i = 0; i < n_entrycache; i++)
	{
		int col = (i % COLS);
		int row = (i - col) / COLS;

		int x = 96 + col * 144;
		int y = 100 + row * 80;

		if(cache[i].icon0_image)
		{
			if(i == menu_sel)
			{
				if(cache[i].sizeX < (float)cache[i].icon0_image->sizeX)
				{
					cache[i].sizeX += (float)cache[i].icon0_image->sizeX * 0.08f;
				}
				if(cache[i].sizeY < (float)cache[i].icon0_image->sizeY)
				{
					cache[i].sizeY += (float)cache[i].icon0_image->sizeY * 0.08f;
				}
			}
			else
			{
				if(cache[i].sizeX > (float)cache[i].icon0_image->sizeX * 0.8f)
				{
					cache[i].sizeX -= (float)cache[i].icon0_image->sizeX * 0.08f;
				}
				if(cache[i].sizeY > (float)cache[i].icon0_image->sizeY * 0.8f)
				{
					cache[i].sizeY -= (float)cache[i].icon0_image->sizeY * 0.08f;
				}
			}

			cache[i].icon0_image->stretchX = (int)cache[i].sizeX;
			cache[i].icon0_image->stretchY = (int)cache[i].sizeY;
			oslDrawImageXY(cache[i].icon0_image, x, y + menu_scroll_y);
		}
	}

	DrawBar();
}

char *pages[] = { "Options", "Extras", "Information" };

void change_nickname(EntryOptions *entries, int item, int sel)
{
	ShowOskDialog(*entries[item + sel].options, entries[item + sel].entry, *entries[item + sel].options, 32, 1);
}

void DrawOptionMenu()
{
	TNConfig old_config;
	memcpy(&old_config, &config, sizeof(TNConfig));

	int page_move = SCROLL_DONE;
	int page_sel = 0;
	float page_scroll = 0;

	int sel = 0, item = 0;
	int old_itemsel = -1;
	int option_menu_mode = 0;

	EntryOptions option_entries[] =
	{
		{ "Nickname", nickname_comment, nickname_option, sizeof(nickname_option) / sizeof(char **), (int *)&change_nickname }, //function
		{ "Button Assign", buttonassign_comment, button_assign, sizeof(button_assign) / sizeof(char **), &config.button_assign },
		{ "CPU Speed", cpuspeed_comment, cpuspeed, sizeof(cpuspeed) / sizeof(char **), &config.cpu_speed },
		{ "ISO Driver", isodriver_comment, isodriver, sizeof(isodriver) / sizeof(char **), &config.iso_driver },
		{ "Hide CFW Folders", hidecfwfolders_comment, offon, sizeof(offon) / sizeof(char **), &config.hide_cfw_folders },
		{ "Exit Button 1", exitbutton_comment, buttons, sizeof(buttons) / sizeof(char **), &config.exit_button_1 },
		{ "Exit Button 2", exitbutton2_comment, buttons, sizeof(buttons) / sizeof(char **), &config.exit_button_2 },
		{ "Exit Hold Duration", exitholdduration_comment, exitholdduration, sizeof(exitholdduration) / sizeof(char **), &config.exit_hold_duration },
		{ "Show PIC1", showpic1_comment, offon, sizeof(offon) / sizeof(char **), &config.show_pic1 },
	};

    while(!osl_quit)
	{
		oslStartDrawing();

		if(old_itemsel == item + sel)
		{
		}
		else
		{
			old_itemsel = item + sel;
			comment_scroll = 10;
		}

		/* Scroll */
		if(option_menu_mode == 0)
		{
			if(option_menu_scroll_y > OPTION_MENU_BEGIN)
			{
				option_menu_scroll_y -= OPTION_MENU_INTERVAL;
			}
		}
		else if(option_menu_mode == 1)
		{
			if(option_menu_scroll_y < OPTION_MENU_END)
			{
				option_menu_scroll_y += OPTION_MENU_INTERVAL;
			}
			else
			{
				break;
			}
		}

		DrawMenu();

		/* Display */
		OptionMenuDisplay(option_entries, sizeof(option_entries) / sizeof(EntryOptions), &item, &sel, SHOW_MAX_OPTIONS);
/*
		switch(page_move)
		{
			case SCROLL_BACKWARD:
				if(page_scroll < PAGE_DISTANCE)
				{
					page_scroll += PAGE_INTERVAL;
				}
				else
				{
					page_sel--;
					page_move = SCROLL_DONE;
					page_scroll = 0;
				}
				break;
				
			case SCROLL_FORWARD:
				if(page_scroll > -PAGE_DISTANCE)
				{
					page_scroll -= PAGE_INTERVAL;
				}
				else
				{
					page_sel++;
					page_move = SCROLL_DONE;
					page_scroll = 0;
				}
				break;
		}

		int i;
		for(i = -1; i < 2; i++)
		{
			oslIntraFontSetStyle(ltn0, i == 0 ? 1.0f : 0.7f, WHITE, 0, INTRAFONT_ALIGN_CENTER);
			oslSetFont(ltn0);

			int item = page_sel + i;
			if(item >= 0 && item < (sizeof(pages) / sizeof(char **)))
			{
				oslDrawString(240 + (i * PAGE_DISTANCE) + page_scroll, option_menu_scroll_y + 224, pages[item]);
			}
		}
*/
		oslEndDrawing();

        oslEndFrame();
        oslSyncFrame();

        oslReadKeys();

		/* Ctrl */
		OptionMenuCtrl(option_entries, sizeof(option_entries) / sizeof(EntryOptions), &item, &sel, SCROLL_MAX_OPTIONS);
/*
		if(osl_keys->pressed.L)
		{
			if(page_sel > 0) page_move = SCROLL_BACKWARD;
		}
		else if(osl_keys->pressed.R)
		{
			if(page_sel < (sizeof(pages) / sizeof(char **)) - 1) page_move = SCROLL_FORWARD;
		}
*/
		if(osl_keys->pressed.square || osl_keys->pressed.circle)
		{
			if(memcmp(&config, &old_config, sizeof(TNConfig)) != 0)
			{
				sctrlSESetConfig(&config);
			}

			option_menu_mode = 1;
		}
	}
}

int MoveHandler()
{
	char *p = strrchr(cache[menu_sel].path, '/');

	char path[256];
	sprintf(path, "ms0:/ISO/%s", p + 1);

	sceIoMkdir("ms0:/ISO/", 0777);

	tnUtilsCopy(cache[menu_sel].path, path, 1);
	tnUtilsRemove(cache[menu_sel].path, 1);

	return sceKernelExitDeleteThread(0);
}

int ExtractHandler()
{
	ExtractZip(cache[menu_sel].path, "ms0:/");

	sceIoRemove(cache[menu_sel].path);

	return sceKernelExitDeleteThread(0);
}

int RemoveHandler()
{
	tnUtilsRemove(cache[menu_sel].path, 1);

	return sceKernelExitDeleteThread(0);
}

void ProgressMenu(char *text, int (* handler)(), u32 (* value_handler)(), u32 max, int show_speed)
{
	SceUID thid = sceKernelCreateThread("ProgressThread", handler, 48, 0x4000, 0, NULL);
	if(thid >= 0) sceKernelStartThread(thid, 0, NULL);

	u32 time_start = sceKernelGetSystemTimeLow();

    while(!osl_quit)
	{
		oslStartDrawing();
		oslDrawImage(background_image);

		DrawBar();

		u32 value = value_handler();

		u32 kb = value / 1024;
		u32 s = ((sceKernelGetSystemTimeLow() - time_start) / (1000 * 1000));
		u32 speed = 0;
		if(kb && s) speed = kb / s;

		float percent = (float)(((float)(100) * (float)value) / (float)max);
		float progress = (float)(((float)pb_bar_image->sizeX * (float)value) / (float)max);

		pb_bar_image->stretchX = (u32)progress;

		oslDrawImageXY(pb_skin_image, 240 - (pb_skin_image->sizeX/2), 136 - (pb_skin_image->sizeY/2));
		oslDrawImageXY(pb_bar_image, 240 - (pb_bar_image->sizeX/2), 136 - (pb_bar_image->sizeY/2));

		oslIntraFontSetStyle(ltn0, 0.9f, WHITE, 0, INTRAFONT_ALIGN_CENTER);
		oslSetFont(ltn0);
		oslDrawString(240, 108, text);

		oslIntraFontSetStyle(ltn0, 0.8f, BLACK, 0, INTRAFONT_ALIGN_CENTER);
		oslSetFont(ltn0);

		char speed_msg[32];
		if(show_speed) sprintf(speed_msg, " %d KB/s", speed);
		oslDrawStringf(240, 130, "%d%%%s", (u32)percent, show_speed ? speed_msg : "");

		oslEndDrawing();

        oslEndFrame();
        oslSyncFrame();

		if(value >= max) break;
    }

	/* Wait thread end and reset menu */
	sceKernelWaitThreadEnd(thid, NULL);
	ResetMenu();
}

void MainMenu()
{
    while(!osl_quit)
	{
		oslStartDrawing();

		DrawMenu();

		oslEndDrawing();

        oslEndFrame();
        oslSyncFrame();

        oslReadKeys();

        if(osl_keys->pressed.up)
		{
			if(menu_sel - COLS >= 0) menu_move = SCROLL_BACKWARD;
		}
        else if(osl_keys->pressed.down)
		{
			if(menu_sel + COLS < n_entrycache) menu_move = SCROLL_FORWARD;
		}
        else if(osl_keys->pressed.left)
		{
			if((menu_sel % COLS) - 1 >= 0 && menu_sel - 1 >= 0) menu_sel--;
		}
        else if(osl_keys->pressed.right)
		{
			if((menu_sel % COLS) + 1 < COLS && menu_sel + 1 < n_entrycache) menu_sel++;
		}

		if(osl_keys->pressed.triangle)
		{
			if(ShowMessageDialog("Are you sure you want to delete it?", 1))
			{
				char *p = strrchr(cache[menu_sel].path, '/');
				if(strcmp(p + 1, "EBOOT.PBP") == 0) *p = '\0';

				u32 size = 0, folders = 0, files = 0;
				tnUtilsGetInfo(cache[menu_sel].path, &size, &folders, &files);

				ProgressMenu("Removing...", &RemoveHandler, tnUtilsGetRemoved, folders+files, 0);
			}
		}

        if(osl_keys->pressed.cross)
		{
			int j;
			for(j = 0; j < EBOOT_CATEGORY_N; j++)
			{
				if(strcmp(cache[menu_sel].category, eboot_category[j].category) == 0)
				{
					struct SceKernelLoadExecVSHParam param;

					memset(&param, 0, sizeof(param));
					param.size = sizeof(param);
					param.argp = eboot_category[j].argp ? eboot_category[j].argp : cache[menu_sel].path;
					param.args = strlen(param.argp) + 1;
					param.key = eboot_category[j].key;

					sctrlKernelLoadExecVSHWithApitype(eboot_category[j].apitype, cache[menu_sel].path, &param);
				}
			}

			if(strcmp(cache[menu_sel].category, "UG") == 0)
			{
				if(strncmp(cache[menu_sel].path, "ms0:/ISO/", 9) != 0) //not in iso folder
				{
					if(ShowMessageDialog("Do you want to move this backup to ms0:/ISO/?", 1))
					{
						u32 size = 0, folders = 0, files = 0;
						tnUtilsGetInfo(cache[menu_sel].path, &size, &folders, &files);

						if(size > (u32)ms_free_space())
						{
							if(!ShowMessageDialog("Not enough memory space.", 0))
							{
								continue;
							}
						}

						ProgressMenu("Moving...", &MoveHandler, tnUtilsGetCopied, size, 1);

						continue;
					}
					else
					{
						if(!ShowMessageDialog("So do you want to start the game?", 1))
						{
							continue;
						}
					}
				}

				sctrlSEMountUmdFromFile(cache[menu_sel].path, config.iso_driver + 1, 0);

				struct SceKernelLoadExecVSHParam param;

				memset(&param, 0, sizeof(param));
				param.size = sizeof(param);
				param.argp = EBOOT_BIN;
				param.args = strlen(param.argp) + 1;
				param.key = "game";

				sctrlKernelLoadExecVSHWithApitype(PSP_INIT_APITYPE_DISC, EBOOT_BIN, &param);
			}

			if(strcmp(cache[menu_sel].category, "ZIP") == 0)
			{
				if(ShowMessageDialog("Do you want to extract this archive?", 1))
				{
					ResetZipRead();
					u32 size = GetZipUncompressedSize(cache[menu_sel].path);

					if(size > (u32)ms_free_space())
					{
						if(!ShowMessageDialog("Not enough memory space.", 0))
						{
							continue;
						}
					}

					ProgressMenu("Extracting...", &ExtractHandler, GetZipRead, size, 1);
				}
			}
		}

        if(osl_keys->pressed.square)
		{
			DrawOptionMenu();
		}
    }
}

int main(int argc, char *argv[])
{
	strcpy(savedata_path, argv[0]);
	char *p = strrchr(savedata_path, '/');
	if(p) p[1] = '\0';

	sctrlSEGetConfig(&config);

    initOSLib();

	ResetMenu();

	MainMenu();

	FreeEntryCache(cache, n_entrycache);

    oslEndGfx();

    sceKernelExitGame();

    return 0;
}