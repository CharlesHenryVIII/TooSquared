#include "Rendering.h"
#include "Misc.h"
#include "STB/stb_image.h"
#include "Block.h"
#include "WinInterop.h"

Renderer g_renderer;
Window g_window;
Camera g_camera;
Light g_light;

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


TextureArray::TextureArray(const char* fileLocation)
{
	uint8* data = stbi_load(fileLocation, &size.x, &size.y, NULL, STBI_rgb_alpha);
	Defer{
		stbi_image_free(data);
	};
	assert(data);

	glGenTextures(1, &gl_handle);
	Bind();
	glBindTexture(GL_TEXTURE_2D_ARRAY, gl_handle);
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
	glBindTexture(GL_TEXTURE_2D_ARRAY, gl_handle);
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

Vertex cubeVertices[] = {
    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, {  1.0f, 0.0f, 0.0f } }, // +x

    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // top right
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // Top Left
    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // bot right
    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // -x bottom Left

    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } }, // +y
    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f,  1.0f, 0.0f } },

    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // -y
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },

    { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } }, // z
    { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
    { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },
    { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f,  1.0f } },

    { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // -z
    { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
    { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
    { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
};
uint32 cubeIndices[36] = {};

void RenderBlock(Block* block)
{
    for (uint32 faceIndex = 0; faceIndex < +Face::Count; faceIndex++)
    {
        uint32 tileIndex = block->spriteLocation[faceIndex];
        if (tileIndex == 254)
            tileIndex = block->defaultSpriteLocation;
        Rect s = GetRectFromSprite(tileIndex);

        const uint32 size = blocksPerRow * pixelsPerBlock;
        float iblx = Clamp(s.botLeft.x  / size, 0.0f, 1.0f);
        float ibly = Clamp(s.botLeft.y  / size, 0.0f, 1.0f);
        float itrx = Clamp(s.topRight.x / size, 0.0f, 1.0f);
        float itry = Clamp(s.topRight.y / size, 0.0f, 1.0f);

        cubeVertices[faceIndex * 4 + 0].uv = { iblx, itry }; //Bot Left
        cubeVertices[faceIndex * 4 + 1].uv = { iblx, ibly }; //Top Left
        cubeVertices[faceIndex * 4 + 2].uv = { itrx, itry }; //Bot Right
        cubeVertices[faceIndex * 4 + 3].uv = { itrx, ibly }; //Top Right
    }
    block->vertexBuffer.Upload(cubeVertices, arrsize(cubeVertices));
    block->indexBuffer.Upload(cubeIndices,  arrsize(cubeIndices));
    g_renderer.textures[Texture::Minecraft]->Bind();
    g_renderer.programs[+Shader::Simple3D]->UseShader();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
    glEnableVertexArrayAttrib(g_renderer.vao, 2);

    Mat4 perspective;
    gb_mat4_perspective(&perspective, 3.14f / 2, float(g_window.size.x) / g_window.size.y, 0.65f, 1000.0f);
    Mat4 transform;
    gb_mat4_translate(&transform, block->p);

    ShaderProgram* sp = g_renderer.programs[+Shader::Simple3D];
    sp->UpdateUniformMat4( "u_perspective", 1, false, perspective.e);
    sp->UpdateUniformMat4( "u_view",        1, false, g_camera.view.e);
    sp->UpdateUniformMat4( "u_model",       1, false, transform.e);

	sp->UpdateUniformVec3( "u_lightColor",	    1,  g_light.c.e);
	sp->UpdateUniformVec3( "u_lightP",          1,  g_light.p.e);
	sp->UpdateUniformVec3( "u_cameraP",         1,  g_camera.p.e);

	sp->UpdateUniformVec3( "material.ambient",  1,  block->material.ambient.e);
	sp->UpdateUniformVec3( "material.diffuse",  1,  block->material.diffuse.e);
	sp->UpdateUniformVec3( "material.specular", 1,  block->material.specular.e);
	sp->UpdateUniformFloat("material.shininess",    block->material.shininess);


    glDrawElements(GL_TRIANGLES, arrsize(cubeIndices), GL_UNSIGNED_INT, 0);
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

void UpdateFrameBuffer(Vec2Int size)
{
	if (g_renderer.backBuffer == nullptr)
	{
		g_renderer.backBuffer = new FrameBuffer();
		glGenFramebuffers(1, &g_renderer.backBuffer->handle);
		glBindFramebuffer(GL_FRAMEBUFFER, g_renderer.backBuffer->handle);
	}
	FrameBuffer* fb = g_renderer.backBuffer;
	if (fb->size.x == size.x && fb->size.y == size.y)
		return;
		
	glBindFramebuffer(GL_FRAMEBUFFER, g_renderer.backBuffer->handle);
	if (fb->colorHandle)
		glDeleteTextures(1, &fb->colorHandle);
	glGenTextures(1, &fb->colorHandle);
	glBindTexture(GL_TEXTURE_2D, fb->colorHandle);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	if (fb->depthHandle)
		glDeleteTextures(1, &fb->depthHandle);
	glGenTextures(1, &fb->depthHandle);
	glBindTexture(GL_TEXTURE_2D, fb->depthHandle);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->colorHandle, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->depthHandle, 0);

	GLint err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (err != GL_FRAMEBUFFER_COMPLETE)
	{
		fprintf(stderr, "Error: Frame buffer error");
	}

	Vertex verticees[] = 
	{
		{-1.0,  1.0, 0, 0.0f, 1.0f },
		{-1.0, -1.0, 0, 0.0f, 0.0f },
		{ 1.0,  1.0, 0, 1.0f, 1.0f },
		{ 1.0, -1.0, 0, 1.0f, 0.0f },
	};

	fb->vertexBuffer.Upload(verticees, arrsize(verticees));
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

	static_assert(arrsize(cubeVertices) == 24, "");

	glGenVertexArrays(1, &g_renderer.vao);
	glBindVertexArray(g_renderer.vao);

#if 0
	g_renderer.textures[Texture::Minecraft] = new Texture("Assets/TestSpriteSheet.png");
#else
	g_renderer.textures[Texture::Minecraft] = new Texture("Assets/MinecraftSpriteSheet20120215.png");
	g_renderer.spriteTextArray = new TextureArray("Assets/MinecraftSpriteSheet20120215.png");
#endif
	g_renderer.programs[+Shader::Simple3D] = new ShaderProgram("Source/Shaders/3D.vert", "Source/Shaders/3D.frag");
	g_renderer.programs[+Shader::BufferCopy] = new ShaderProgram("Source/Shaders/BufferCopy.vert", "Source/Shaders/BufferCopy.frag");

	for (int face = 0; face < 6; ++face)
	{
		int base_index = face * 4;
		//NOTE: This is to fix culling issues where winding order
		//is reversed on some of these vertices
		if (face == 1 || face == 3 || face == 4)
		{
			cubeIndices[face * 6 + 0] = base_index + 0;
			cubeIndices[face * 6 + 1] = base_index + 1;
			cubeIndices[face * 6 + 2] = base_index + 2;
			cubeIndices[face * 6 + 3] = base_index + 1;
			cubeIndices[face * 6 + 4] = base_index + 3;
			cubeIndices[face * 6 + 5] = base_index + 2;
		}
		else
		{
			cubeIndices[face * 6 + 0] = base_index + 0;
			cubeIndices[face * 6 + 1] = base_index + 2;
			cubeIndices[face * 6 + 2] = base_index + 1;
			cubeIndices[face * 6 + 3] = base_index + 1;
			cubeIndices[face * 6 + 4] = base_index + 2;
			cubeIndices[face * 6 + 5] = base_index + 3;
		}
	}
	g_light.c = {  1.0f,  1.0f,  1.0f };
	g_light.p = { 25.0f, 270.0f, 25.0f };

	g_renderer.chunkIB = new IndexBuffer();
	FillIndexBuffer(g_renderer.chunkIB);

}
