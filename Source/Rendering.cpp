#include "Rendering.h"
#include "Misc.h"


const SDL_MessageBoxColorScheme colorScheme = {
	/* .colors (.r, .g, .b) */
	   /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
	{{ 200, 200, 200 },
	/* [SDL_MESSAGEBOX_COLOR_TEXT] */
	{   0,   0,   0 },
	/* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
	{ 100, 100, 100 },
	/* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
	{ 220, 220, 220 },
	/* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
	{ 240, 240, 240 }}
};

int32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, int32 numOfButtons, ts_MessageBox type, const char*  title, const char* message)
{
	SDL_MessageBoxData messageBoxData = {
		.flags = uint32(type),
		.window = NULL,
		.title = title, //an UTF-8 title
		.message = message, //an UTF-8 message text
		.numbuttons = numOfButtons, //the number of buttons
		.buttons = buttons, //an array of SDL_MessageBoxButtonData with length of numbuttons
		.colorScheme = &colorScheme
	};

	int32 buttonID = 0;

	if (SDL_ShowMessageBox(&messageBoxData, &buttonID))
	{
		FAIL;
	}
	if (buttonID == -1)
	{
		FAIL;
	}
	return buttonID;
}

