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

bool ShaderProgram::CompileShader(GLuint handle, const char* text)
{
	glShaderSource(handle, 1, &text, NULL);
	glCompileShader(handle);

	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint log_length;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
		GLchar info[4096];
		glGetShaderInfoLog(handle, log_length, NULL, info);
		DebugPrint("Vertex Shader compilation error: %s\n", info);

		SDL_MessageBoxButtonData buttons[] = {
			//{ /* .flags, .buttonid, .text */        0, 0, "Continue" },
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Retry" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Stop" },
		};

		int32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), ts_MessageBox::Error, "Shader Compilation Error", reinterpret_cast<char*>(info));
		if (buttons[buttonID].buttonid == 2)//NOTE: Stop button
		{
			DebugPrint("stop hit");
			g_running = false;
			return false;
		}
		else
		{
			if (buttons[buttonID].buttonid == 0)//NOTE: Continue button
			{

				return false;
			}
			else if (buttons[buttonID].buttonid == 1)//NOTE: Retry button
			{
				CheckForUpdate();
			}
		}
	}
#ifdef _DEBUGPRINT
	DebugPrint("Shader Vertex/Fragment Created\n");
#endif
	return true;
}

ShaderProgram::ShaderProgram(const std::string& vertexFileLocation, const std::string& pixelFileLocation)
{
	m_vertexFile = vertexFileLocation;
	m_pixelFile = pixelFileLocation;

	CheckForUpdate();
}

ShaderProgram::~ShaderProgram()
{
	//TODO: Delete shaders as well
	glDeleteProgram(m_handle);
#ifdef _DEBUGPRINT
	DebugPrint("Shader Deleted\n");
#endif
}

FILETIME ShaderProgram::GetShaderTimeAndText(uint8* buffer, const size_t bufferLength, const std::string& fileLoc)
{
	HANDLE bufferHandle = CreateFileA(fileLoc.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	{
		if (bufferHandle == INVALID_HANDLE_VALUE)
			FAIL;
	}//HANDLE should be valid

	uint32 bytesRead;
	static_assert(sizeof(DWORD) == sizeof(uint32));
	static_assert(sizeof(LPVOID) == sizeof(void*));
	if (!ReadFile(bufferHandle, buffer, (DWORD)bufferLength, reinterpret_cast<LPDWORD>(&bytesRead), NULL))
	{
		DebugPrint("Read File failed");
		DWORD error = GetLastError();
		if (error == ERROR_INSUFFICIENT_BUFFER)
			FAIL;
	}

	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	if (!GetFileTime(bufferHandle, &creationTime, &lastAccessTime, &lastWriteTime))
	{
		DebugPrint("GetFileTime failed with %d\n", GetLastError());
	}
	CloseHandle(bufferHandle);
	return lastWriteTime;
}

void ShaderProgram::CheckForUpdate()
{
	const size_t bufferLength = 1024;
	uint8 vertexBufferText[bufferLength] = {};
	FILETIME vertexLastWriteFiletime = GetShaderTimeAndText(vertexBufferText, bufferLength, m_vertexFile);

	uint8 pixelBufferText[bufferLength] = {};
	FILETIME pixelLastWriteFiletime = GetShaderTimeAndText(pixelBufferText, bufferLength, m_pixelFile);


	if (m_vertexLastWriteTime.dwLowDateTime < vertexLastWriteFiletime.dwLowDateTime ||
		m_vertexLastWriteTime.dwHighDateTime < vertexLastWriteFiletime.dwHighDateTime ||
		m_pixelLastWriteTime.dwLowDateTime < pixelLastWriteFiletime.dwLowDateTime ||
		m_pixelLastWriteTime.dwHighDateTime < pixelLastWriteFiletime.dwHighDateTime)
	{
		GLuint vhandle = glCreateShader(GL_VERTEX_SHADER);
		GLuint phandle = glCreateShader(GL_FRAGMENT_SHADER);
		//Compile shaders and link to program
		if (!CompileShader(vhandle, reinterpret_cast<char*>(vertexBufferText)) ||
			!CompileShader(phandle, reinterpret_cast<char*>(pixelBufferText)))
			return;


		GLuint handle = glCreateProgram();

		glAttachShader(handle, vhandle);
		glAttachShader(handle, phandle);
		glLinkProgram(handle);

		GLint status;
		glGetProgramiv(handle, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLint log_length;
			glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &log_length);
			GLchar info[4096];
			glGetProgramInfoLog(m_handle, log_length, NULL, info);
			DebugPrint("Shader linking error: %s\n", info);

			SDL_MessageBoxButtonData buttons[] = {
				{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
				{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Stop" },
			};
			int32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), ts_MessageBox::Error, "Shader Compilation Error", reinterpret_cast<char*>(info));
			if (buttonID = 0)
			{
				CheckForUpdate();
			}
			else if (buttonID = 1)
			{
				g_running = false;
				return;
			}
			else
			{
				FAIL;
			}

		}
		else
		{
			m_handle = handle;
#ifdef _DEBUGPRINT
			DebugPrint("Shader Created\n");
#endif
			m_vertexLastWriteTime = vertexLastWriteFiletime;
			m_pixelLastWriteTime = pixelLastWriteFiletime;
			glDeleteShader(vhandle);
			glDeleteShader(phandle);
		}
	}
}

void ShaderProgram::UseShader()
{
	glUseProgram(m_handle);
#ifdef _DEBUGPRINT
	DebugPrint("Shader Used\n");
#endif
}

void ShaderProgram::UpdateUniformMat4(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	GLint loc = glGetUniformLocation(m_handle, name);
	glUniformMatrix4fv(loc, count, transpose, value);
#ifdef _DEBUGPRINT
	DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void GpuBuffer::UploadData(void* data, size_t size)
{
	Bind();
	size_t required_size = size;
	if (m_allocated_size < required_size)
	{
		glBufferData(m_target, required_size, nullptr, GL_STATIC_DRAW);
		m_allocated_size = required_size;
	}
	glBufferSubData(m_target, 0, required_size, data);
}

GpuBuffer::~GpuBuffer()
{
	glDeleteBuffers(1, &m_handle);
#ifdef _DEBUGPRINT
	DebugPrint("GPU Buffer deleted %i, %i\n", m_target, m_handle);
#endif
}

void GpuBuffer::Bind()
{
	glBindBuffer(m_target, m_handle);
#ifdef _DEBUGPRINT
	DebugPrint("GPU Buffer Bound %i, %i\n", m_target, m_handle);
#endif
}

GLuint GpuBuffer::GetGLHandle()
{
	return m_handle;
}

void IndexBuffer::Upload(uint32* indices, size_t count)
{
	UploadData(indices, sizeof(indices[0]) * count);
#ifdef _DEBUGPRINT
	DebugPrint("Index Buffer Upload,size %i\n", count);
#endif
}

void VertexBuffer::Upload(Vertex* vertices, size_t count)
{
	UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
	DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
}
