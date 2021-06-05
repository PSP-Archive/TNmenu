#include <pspkernel.h>
#include <oslib/oslib.h>

#include "main.h"
#include "option_menu.h"

void OptionMenuDisplay(EntryOptions *entries, int n_entries, int *item, int *sel, int show_max)
{
	/* Draw entries */
	int i;
	for(i = 0; i < show_max && (*item + i) < n_entries; i++)
	{
		oslIntraFontSetStyle(ltn0, 0.9f, *sel == i ? BLACK : WHITE, 0, INTRAFONT_ALIGN_LEFT);
		oslSetFont(ltn0);
		oslDrawStringf(50, option_menu_scroll_y + 40 + i * 20, "%s", entries[*item + i].entry);

		if(entries[*item + i].n_options)
		{
			oslIntraFontSetStyle(ltn0, 0.9f, *sel == i ? BLACK : WHITE, 0, INTRAFONT_ALIGN_RIGHT);
			oslSetFont(ltn0);
			oslDrawString(480-50, option_menu_scroll_y + 40 + i * 20, entries[*item + i].options[entries[*item + i].n_options > 1 ? (*entries[*item + i].value) : 0]);
		}
	}

	/* Draw comment */
	if(entries[*item + *sel].comment)
	{
		oslIntraFontSetStyle(ltn0, 0.6f, WHITE, 0, INTRAFONT_SCROLL_LEFT);
		oslSetFont(ltn0);
		comment_scroll = oslIntraFontPrintColumn(ltn0, comment_scroll, option_menu_scroll_y + 190, 460, 0, entries[*item + *sel].comment);
	}
}

void OptionMenuCtrl(EntryOptions *entries, int n_entries, int *item, int *sel, int scroll_max)
{
	if(osl_keys->pressed.up)
	{
		if(*sel > 0)
		{
			(*sel)--;
		}
		else
		{
			if(*item > 0)
			{
				(*item)--;
			}
		}
	}
	else if(osl_keys->pressed.down)
	{
		if(*sel < n_entries - 1) //lower entries than scroll_max
		{
			if(*sel < scroll_max - 1)
			{
				(*sel)++;
			}
			else
			{
				if((*item + *sel) < n_entries - 1)
				{
					(*item)++;
				}
			}
		}
	}
	else if(osl_keys->pressed.left)
	{
		if(entries[*item + *sel].n_options > 1)
		{
			if((*entries[*item + *sel].value) > 0) (*entries[*item + *sel].value)--;
			else (*entries[*item + *sel].value) = entries[*item + *sel].n_options - 1;
		}
	}
	else if(osl_keys->pressed.right || osl_keys->pressed.cross)
	{
		if(entries[*item + *sel].n_options > 1)
		{
			if((*entries[*item + *sel].value) < (entries[*item + *sel].n_options - 1)) (*entries[*item + *sel].value)++;
			else (*entries[*item + *sel].value) = 0;
		}
		else //is function to call
		{
			int (* func)() = (void *)entries[*item + *sel].value;
			func(entries, *item, *sel);
		}
	}
}