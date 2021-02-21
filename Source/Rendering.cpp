#include "Rendering.h"
#include "Misc.h"
#include "STB/stb_image.h"


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

Texture::Texture(const char* fileLocation)
{
	data = stbi_load(fileLocation, &size.x, &size.y, &n, STBI_rgb_alpha);

	glGenTextures(1, &gl_handle);
	Bind();
	glBindTexture(GL_TEXTURE_2D, gl_handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#ifdef _DEBUGPRINT
	DebugPrint("Texture Created\n");
#endif

}

inline void Texture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, gl_handle);
#ifdef _DEBUGPRINT
	DebugPrint("Texture Bound\n");
#endif
}
