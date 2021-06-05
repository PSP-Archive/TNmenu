#ifndef __OPTION_MENU_H__
#define __OPTION_MENU_H__

typedef struct
{
	char *entry;
	char *comment;
	char **options;
	int n_options;
	int *value; //can be used as function
} EntryOptions;

void OptionMenuDisplay(EntryOptions *entries, int n_entries, int *item, int *sel, int show_max);
void OptionMenuCtrl(EntryOptions *entries, int n_entries, int *item, int *sel, int scroll_max);

#endif