#include <pspkernel.h>
#include <oslib/oslib.h>

#include "main.h"

int ShowMessageDialog(char *message, int enableYesno)
{
	int res = 0;

	oslInitMessageDialog(message, enableYesno);

	while(!osl_quit)
	{
		oslStartDrawing();

		DrawMenu();
		oslDrawDialog();

		oslEndDrawing();

		oslEndFrame();
		oslSyncFrame();

		if(oslGetDialogStatus() == PSP_UTILITY_DIALOG_NONE)
		{
			if(enableYesno)
			{
				if(oslGetDialogButtonPressed() == PSP_UTILITY_MSGDIALOG_RESULT_YES) res = 1;
			}

			oslEndDialog();
			break;
		}
	}
	
	return res;
}

int ShowOskDialog(char *outtext, char *descStr, char *initialStr, int textLimit, int linesNumber)
{
	int res = 0;

	oslInitOsk(descStr, initialStr, textLimit, linesNumber, -1);

	while(!osl_quit)
	{
		oslStartDrawing();

		DrawMenu();
		oslDrawOsk();

		oslEndDrawing();

		oslEndFrame();
		oslSyncFrame();

		if(oslGetOskStatus() == PSP_UTILITY_DIALOG_NONE)
		{
			if(oslOskGetResult() != OSL_OSK_CANCEL)
			{
				oslOskGetText(outtext);
				res = 1;
			}

			oslEndOsk();
			break;
		}
	}
	
	return res;
}