#include "Rendering.h"
#include "Misc.h"
#include "STB/stb_image.h"
#include "Block.h"
#include "WinInterop.h"

Renderer g_renderer;
Window g_window;
Camera g_camera;
#if DIRECTIONALLIGHT == 1
Light_Direction g_light;
#else
Light_Point g_light;
#endif

extern "C" {
#ifdef _MSC_VER
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#else
    __attribute__((dllexport)) uint32_t NvOptimusEnablement = 0x00000001;
    __attribute__((dllexport)) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif
}

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

Texture::Texture(TextureParams tp)
{
	glGenTextures(1, &m_handle);
	Bind();

	if (tp.minFilter)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tp.minFilter);
	if (tp.magFilter)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tp.magFilter);
	if (tp.wrapS)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tp.wrapS);
	if (tp.wrapT)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tp.wrapT);

	glTexImage2D(GL_TEXTURE_2D, 0, tp.internalFormat, tp.size.x, tp.size.y, 0, tp.internalFormat, tp.type, tp.data);
	m_size = tp.size;
#ifdef _DEBUGPRINT
	DebugPrint("Texture Created\n");
#endif
}

Texture::Texture(const char* fileLocation)
{
	m_data = stbi_load(fileLocation, &m_size.x, &m_size.y, &m_bytesPerPixel, STBI_rgb_alpha);

	glGenTextures(1, &m_handle);
	Bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.x, m_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
#ifdef _DEBUGPRINT
	DebugPrint("Texture Created\n");
#endif
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_handle);
	stbi_image_free(m_data);
}

inline void Texture::Bind()
{
	glBindTexture(GL_TEXTURE_2D, m_handle);
#ifdef _DEBUGPRINT
	DebugPrint("Texture Bound\n");
#endif
}


TextureArray::TextureArray(const char* fileLocation)
{
	uint8* data = stbi_load(fileLocation, &m_size.x, &m_size.y, NULL, STBI_rgb_alpha);
	Defer{
		stbi_image_free(data);
	};
	assert(data);

	glGenTextures(1, &m_handle);
	Bind();
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    uint32 mipMapLevels = 5;
	uint32 height = 16;
    uint32 width = 16;
	uint32 depth = 256;

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipMapLevels, GL_RGBA8, width, height, depth);

	//TODO: Fix
	uint32 colors[16 * 16] = {};
	uint32 arrayIndex = 0;
	for (uint32 y = height; y--;)
	{
		for (uint32 x = 0; x < width; x++)
		{
			for (uint32 xp= 0; xp < 16; xp++)  //Total
			{
				for (uint32 yp = 0; yp < 16; yp++)  //Total
				{
					uint32 sourceIndex = (y * 16 + yp) * 256 + x * 16 + xp;
					uint32 destIndex = yp * 16 + xp;
					colors[destIndex] = reinterpret_cast<uint32*>(data)[sourceIndex];
				}
			}
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, arrayIndex, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, colors);
			arrayIndex++;
		}
	}
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
#ifdef _DEBUGPRINT
	DebugPrint("Texture Created\n");
#endif
}

inline void TextureArray::Bind()
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
#ifdef _DEBUGPRINT
	DebugPrint("Texture Bound\n");
#endif
}


bool ShaderProgram::CompileShader(GLuint handle, const char* name, std::string text)
{
	const char* strings[] = { text.c_str() };
	glShaderSource(handle, 1, strings, NULL);
	glCompileShader(handle);

	GLint status;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint log_length;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
		std::string infoString;
		infoString.resize(log_length, 0);
		//GLchar info[4096] = {};
		glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)infoString.c_str());
		DebugPrint("Vertex Shader compilation error: %s\n", infoString.c_str());

		SDL_MessageBoxButtonData buttons[] = {
			//{ /* .flags, .buttonid, .text */        0, 0, "Continue" },
			{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
			{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Stop" },
		};

		int32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), ts_MessageBox::Error, name, infoString.c_str());
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

void ShaderProgram::CheckForUpdate()
{

	uint64 vertexFileTime = 0;
	if (!GetFileTime(&vertexFileTime, m_vertexFile))
	{
		assert(false);//todo
		return;
	}

	uint64 pixelFileTime = 0;
	if(!GetFileTime(&pixelFileTime, m_pixelFile))
	{
		assert(false);//todo
		return;
	}

	if (m_vertexLastWriteTime < vertexFileTime ||
		m_pixelLastWriteTime  < pixelFileTime)
	{
		//Compile shaders and link to program
		GLuint vhandle = glCreateShader(GL_VERTEX_SHADER);
		GLuint phandle = glCreateShader(GL_FRAGMENT_SHADER);

		std::string vertexText;
		std::string pixelText;
		GetFileText(vertexText, m_vertexFile);
		GetFileText(pixelText, m_pixelFile);

		if (!CompileShader(vhandle, "Vertex Shader", vertexText) ||
			!CompileShader(phandle, "Pixel Shader", pixelText))
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
			glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
			GLchar info[4096] = {};
			assert(log_length > 0);
			glGetProgramInfoLog(handle, log_length, NULL, info);
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
			m_vertexLastWriteTime = vertexFileTime;
			m_pixelLastWriteTime = pixelFileTime;
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

void ShaderProgram::UpdateUniformVec4(const char* name, GLsizei count, const GLfloat* value)
{
	GLint loc = glGetUniformLocation(m_handle, name);
	glUniform4fv(loc, count, value);
#ifdef _DEBUGPRINT
	DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformVec3(const char* name, GLsizei count, const GLfloat* value)
{
	GLint loc = glGetUniformLocation(m_handle, name);
	glUniform3fv(loc, count, value);
#ifdef _DEBUGPRINT
	DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformFloat(const char* name, GLfloat value)
{
	GLint loc = glGetUniformLocation(m_handle, name);
	glUniform1f(loc, value);
#ifdef _DEBUGPRINT
	DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformUint8(const char* name, GLuint value)
{
	GLint loc = glGetUniformLocation(m_handle, name);
	glUniform1ui(loc, value);
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
void VertexBuffer::Upload(Vertex_Chunk* vertices, size_t count)
{
	UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
	DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
}

double s_lastShaderUpdateTime = 0;
double s_incrimentalTime = 0;
void RenderUpdate(float deltaTime)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (s_lastShaderUpdateTime + 0.1f <= s_incrimentalTime)
	{
		for (ShaderProgram* s : g_renderer.programs)
		{
			if (s)
				s->CheckForUpdate();
		}
		s_lastShaderUpdateTime = s_incrimentalTime;
	}
    s_incrimentalTime += deltaTime;


}

Rect GetRectFromSprite(uint32 i)
{
    assert(i <= blocksPerRow * blocksPerRow);
    i = Clamp<uint32>(i, 0, blocksPerRow * blocksPerRow);
    uint32 x = i % blocksPerRow;
    uint32 y = i / blocksPerRow;

    Rect result = {};
    result.botLeft.x  = static_cast<float>(x * pixelsPerBlock);
    result.botLeft.y  = static_cast<float>(pixelsPerBlock * (blocksPerRow - y - 1));
    result.topRight.x = result.botLeft.x + pixelsPerBlock;
    result.topRight.y = result.botLeft.y + pixelsPerBlock;
    return result;
}

void OpenGLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                         GLsizei length, const GLchar *message, const void *userParam)
{
    std::string _severity;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        _severity = "HIGH";
        break;
        case GL_DEBUG_SEVERITY_MEDIUM:
        _severity = "MEDIUM";
        break;
        case GL_DEBUG_SEVERITY_LOW:
        _severity = "LOW";
        break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        _severity = "NOTIFICATION";
        return;
        //break;
        default:
        _severity = "UNKNOWN";
        break;
    }

    DebugPrint("%s severity: %s\n",
            _severity.c_str(), message);
    //DebugPrint("%d: %s of %s severity, raised from %s: \n%s\n",
    //        id, _type.c_str(), _severity.c_str(), _source.c_str(), message);
    //DebugPrint("vs \n");
    //DebugPrint("%s\n\n", message);

    AssertOnce(severity != GL_DEBUG_SEVERITY_HIGH);
}

void FillIndexBuffer(IndexBuffer* ib)
{
	std::vector<uint32> arr;

	size_t amount = CHUNK_X * CHUNK_Y * CHUNK_Z * 6 * 6;
	arr.reserve(amount);
	int32 baseIndex = 0;
	for (int32 i = 0; i < amount; i += 6)
	{
		arr.push_back(baseIndex + 0);
		arr.push_back(baseIndex + 1);
		arr.push_back(baseIndex + 2);
		arr.push_back(baseIndex + 1);
		arr.push_back(baseIndex + 3);
		arr.push_back(baseIndex + 2);

		baseIndex += 4; //Amount of vertices
	}
	
	ib->Upload(arr.data(), amount);
}

//void SSOAUpdate()
//{
//	Texture::TextureParams tp = {};
//	tp.minFilter = GL_NEAREST;
//	tp.magFilter = GL_NEAREST;
//	tp.wrapS = GL_CLAMP_TO_EDGE;
//	tp.wrapT = GL_CLAMP_TO_EDGE;
//	tp.internalFormat = GL_RGBA16F;
//	Texture* t = new Texture(tp);
//
//	//std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
//	//std::default_random_engine generator;
//	std::vector<Vec3> ssaoKernel;
//	for (unsigned int i = 0; i < 64; ++i)
//	{
//		Vec3 sample = {
//			RandomFloat(0.0f, 1.0f) * 2.0f - 1.0f,
//			RandomFloat(0.0f, 1.0f) * 2.0f - 1.0f,
//			RandomFloat(0.0f, 1.0f) * 2.0f - 1.0f
//		};
//		sample = Normalize(sample);
//		sample *= RandomFloat(0.0f, 1.0f);
//		//ssaoKernel.push_back(sample);
//		float scale = (float)i / 64.0;
//		scale = Lerp(0.1f, 1.0f, scale * scale);
//		sample *= scale;
//		ssaoKernel.push_back(sample);
//	}
//
//	std::vector<Vec3> ssaoNoise;
//	for (unsigned int i = 0; i < 16; i++)
//	{
//		Vec3 noise = {
//			RandomFloat(0.0f, 1.0f) * 2.0 - 1.0,
//			RandomFloat(0.0f, 1.0f) * 2.0 - 1.0,
//			0.0f};
//		ssaoNoise.push_back(noise);
//	}
//	Texture::TextureParams tp2 = {
//		.size = { 4, 4 },
//		.minFilter = GL_NEAREST,
//		.magFilter = GL_NEAREST,
//		.internalFormat = GL_RGBA16F,
//		.format = GL_RGB,
//		.type = GL_FLOAT,
//		.data = &ssaoNoise[0],
//	};
//	Texture* noiseTexture = new Texture(tp2);
//
//	FrameBuffer ssaoFBO = FrameBuffer();
//	Texture::TextureParams tp3 = {
//		.minFilter = GL_NEAREST,
//		.magFilter = GL_NEAREST,
//		.wrapS = 0,
//		.wrapT = 0,
//		.internalFormat = GL_RED,
//		.format = GL_RED,
//		.type = GL_FLOAT,
//	};
//	ssaoFBO.m_color = new Texture(tp3);
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoFBO.m_color->m_handle, 0);
//
//	
//}

inline void FrameBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

FrameBuffer::FrameBuffer()
{
	glGenFramebuffers(1, &m_handle);
	Bind();
}

void UpdateFrameBuffer(Vec2Int size)
{
	if (g_renderer.backBuffer == nullptr)
	{
		g_renderer.backBuffer = new FrameBuffer();
	}
	FrameBuffer* fb = g_renderer.backBuffer;
	if (fb->m_size.x == size.x && fb->m_size.y == size.y)
		return;

	fb->Bind();
	if (fb->m_color && fb->m_color->m_handle)
		delete fb->m_color;
	if (fb->m_depth && fb->m_depth->m_handle)
		delete fb->m_depth;
	Texture::TextureParams tp = {};
	tp.size = size;
	fb->m_color = new Texture(tp);
	tp.internalFormat = GL_DEPTH_COMPONENT;
	tp.format = GL_DEPTH_COMPONENT;
	fb->m_depth = new Texture(tp);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->m_color->m_handle, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->m_depth->m_handle, 0);

	GLint err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (err != GL_FRAMEBUFFER_COMPLETE)
	{
		DebugPrint("Error: Frame buffer error: %s", stderr);
		assert(false);
	}

	Vertex verticees[] = 
	{
		{-1.0,  1.0, 0, 0.0f, 1.0f },
		{-1.0, -1.0, 0, 0.0f, 0.0f },
		{ 1.0,  1.0, 0, 1.0f, 1.0f },
		{ 1.0, -1.0, 0, 1.0f, 0.0f },
	};

	fb->m_vertexBuffer.Upload(verticees, arrsize(verticees));
}


void InitializeVideo()
{
    SDL_Init(SDL_INIT_VIDEO);
    {
        SDL_Rect screenSize = {};
        SDL_GetDisplayBounds(0, &screenSize);
        float displayRatio = 16 / 9.0f;
        g_window.size.x = screenSize.w / 2;
        g_window.size.y = Clamp<int>(int(g_window.size.x / displayRatio), 50, screenSize.h);
        g_window.pos.x = g_window.size.x / 2;
        g_window.pos.y = g_window.size.y / 2;
    }

    uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    g_window.SDL_Context = SDL_CreateWindow("TooSquared", g_window.pos.x, g_window.pos.y, g_window.size.x, g_window.size.y, windowFlags);
    /* Create an OpenGL context associated with the window. */

    {
        const int32 majorVersionRequest = 4;//3;
        const int32 minorVersionRequest = 3;//2;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersionRequest);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersionRequest);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        g_renderer.GL_Context = SDL_GL_CreateContext(g_window.SDL_Context);
        SDL_GL_MakeCurrent(g_window.SDL_Context, g_renderer.GL_Context);
        GLint majorVersionActual = {};
        GLint minorVersionActual = {};
        glGetIntegerv(GL_MAJOR_VERSION, &majorVersionActual);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersionActual);

        if (majorVersionRequest != majorVersionActual && minorVersionRequest != minorVersionActual)
        {
            DebugPrint("OpenGL could not set recommended version: %i.%i to %i.%i\n",
                    majorVersionRequest, minorVersionRequest,
                    majorVersionActual,  minorVersionActual);
            FAIL;
        }
    }

    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
    SDL_GL_SetSwapInterval(1);
    glewExperimental = true;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        DebugPrint("Error: %s\n", glewGetErrorString(err));
    }
    DebugPrint("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	stbi_set_flip_vertically_on_load(true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClearColor(0.263f, 0.706f, 0.965f, 0.0f);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLErrorCallback, NULL);
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_FALSE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glGenVertexArrays(1, &g_renderer.vao);
	glBindVertexArray(g_renderer.vao);

#if 0
	g_renderer.textures[Texture::Minecraft] = new Texture("Assets/TestSpriteSheet.png");
	g_renderer.spriteTextArray = new TextureArray("Assets/TestSpriteSheet.png");
#else
	g_renderer.textures[Texture::Minecraft] = new Texture("Assets/MinecraftSpriteSheet20120215.png");
	g_renderer.spriteTextArray = new TextureArray("Assets/MinecraftSpriteSheet20120215.png");
#endif
	g_renderer.programs[+Shader::Simple3D] = new ShaderProgram("Source/Shaders/3D.vert", "Source/Shaders/3D.frag");
	g_renderer.programs[+Shader::BufferCopy] = new ShaderProgram("Source/Shaders/BufferCopy.vert", "Source/Shaders/BufferCopy.frag");

#if DIRECTIONALLIGHT == 1
	g_light.d = {  1.0f, -1.0f,  1.0f };
	g_light.c = {  1.0f,  1.0f,  1.0f };
#else
	g_light.p = { 25.0f, 270.0f, 25.0f };
	g_light.c = {  1.0f,  1.0f,  1.0f };
#endif

	g_renderer.chunkIB = new IndexBuffer();
	FillIndexBuffer(g_renderer.chunkIB);

}
