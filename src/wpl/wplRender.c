void wInitShader(wShader* shader, i32 stride)
{
	shader->vert = 0;
	shader->frag = 0;
	shader->program = 0;
	shader->targetVersion = 33;
	shader->defaultDivisor = 0;
	shader->stride = stride;
	shader->attribCount = 0;
	shader->uniformCount = 0;
}

i32 wAddAttribToShader(wShader* shader, wShaderComponent* attrib)
{
	if(shader->attribCount < 0) shader->attribCount = 0;
	if(shader->attribCount >= Shader_MaxAttribs) {
		return 0;
	}

	shader->attribs[shader->attribCount++] = *attrib;
	return 1;
}

wShaderComponent* wCreateAttrib(wShader* shader, 
		string name, i32 type, i32 count, usize ptr)
{
	if(shader->attribCount < 0) shader->attribCount = 0;
	if(shader->attribCount >= Shader_MaxAttribs) {
		return NULL;
	}

	wShaderComponent* c = shader->attribs + shader->attribCount;
	c->name = name;
	c->loc = shader->attribCount;
	c->divisor = shader->defaultDivisor;
	c->type = type;
	c->count = count;
	c->ptr = ptr;
	shader->attribCount++;
	return c;
}

i32 wAddUniformToShader(wShader* shader, wShaderComponent* uniform)
{
	if(shader->uniformCount < 0) shader->uniformCount = 0;
	if(shader->uniformCount >= Shader_MaxUniforms) {
		return 0;
	}

	shader->uniforms[shader->uniformCount++] = *uniform;
	return 0;
}

wShaderComponent* wCreateUniform(wShader* shader,
		string name, i32 type, i32 count, usize ptr)
{
	if(shader->uniformCount < 0) shader->uniformCount = 0;
	if(shader->uniformCount >= Shader_MaxUniforms) {
		return NULL;
	}

	wShaderComponent* c = shader->uniforms + shader->uniformCount;
	c->name = name;
	c->loc = shader->attribCount;
	c->divisor = shader->defaultDivisor;
	c->type = type;
	c->count = count;
	c->ptr = ptr;

	shader->uniformCount++;
	return c;
}

i32 wFinalizeShader(wShader* shader)
{
	if(shader->program != 0) {
		wLogError(0, "Error: attempting to re-compile shader\n");
		wLogError(0, "Delete old program first before trying\n");
		return 0;
	}
	
	shader->program = glCreateProgram();

	glAttachShader(shader->program, shader->vert);
	if(shader->targetVersion < 33)
	for(isize i = 0; i < shader->attribCount; ++i) {
		shader->attribs[i].loc = i;
		glBindAttribLocation(shader->program, i, shader->attribs[i].name);
	}
	glAttachShader(shader->program, shader->frag);
	glLinkProgram(shader->program);

	i32 success = 1;
	glGetProgramiv(shader->program, GL_LINK_STATUS, &success);
	if(!success) {
		char log[4096];
		i32 logSize = 0;
		glGetProgramInfoLog(shader->program, 4096, &logSize, log);
		wLogError(0, "\n=====Shader Program Link Log=====\n%s\n\n", log);
		return 0;
	}

	glUseProgram(shader->program);

	for(isize i = 0; i < shader->uniformCount; ++i) {
		shader->uniforms[i].loc = glGetUniformLocation(
				shader->program, shader->uniforms[i].name);
	}

	glUseProgram(0);
	return 1;
}

void wDeleteShaderProgram(wShader* shader)
{
	wRemoveSourceFromShader(shader, wShader_Vertex);
	wRemoveSourceFromShader(shader, wShader_Frag);
	glDeleteProgram(shader->program);
	shader->program = 0;
}

void wRemoveSourceFromShader(wShader* shader, i32 kind)
{
	if(kind == wShader_Vertex) {
		glDetachShader(shader->program, shader->vert);
		glDeleteShader(shader->vert);
		shader->vert = 0;
	} else if(kind == wShader_Frag) {
		glDetachShader(shader->program, shader->frag);
		glDeleteShader(shader->frag);
		shader->frag = 0;
	}
}


i32 wAddSourceToShader(wShader* shader, string src, i32 kind)
{
	i32 glkind = -1;
	if(kind == wShader_Vertex) {
		glkind = GL_VERTEX_SHADER;
		if(shader->vert != 0) {
			wLogError(0, "Error: re-adding vertex source\n");
			return 0;
		}
	} else if(kind == wShader_Frag) {
		glkind = GL_FRAGMENT_SHADER;
		if(shader->frag != 0) {
			wLogError(0, "Error: re-adding fragment source\n");
			return 0;
		}
	} else {
		//TODO(will): error logging
		return 0;
	}

	u32 obj = glCreateShader(glkind);
	glShaderSource(obj, 1, &src, NULL);
	glCompileShader(obj);
	
	i32 success = 1;
	glGetShaderiv(obj, GL_COMPILE_STATUS, &success);
	if(!success) {
		char log[4096];
		i32 logSize = 0;
		glGetShaderInfoLog(obj, 4096, &logSize, log);
		wLogError(0, "\n=====%s Shader Compile Log=====\n%s\n\n", 
				kind == wShader_Vertex ? "Vertex" : "Frag", log);
		wLogError(0, "\n=====Source====\n%s\n=====End=====\n\n", src);
		return 0;
	}

	if(kind == wShader_Vertex) {
		shader->vert = obj;
	} else if(kind == wShader_Frag) {
		shader->frag = obj;
	}
	return 1;
}

void wInitBatch(wRenderBatch* batch,
		wTexture* texture, wShader* shader,
		i32 renderCall, i32 primitiveMode, 
		isize elementSize, isize instanceSize,
		void* data, u32* indices)
{
	memset(batch, 0, sizeof(wRenderBatch));
	batch->texture = texture;
	batch->shader = shader;

	batch->renderCall = renderCall;
	batch->primitiveMode = primitiveMode;
	batch->elementSize = elementSize;
	batch->instanceSize = instanceSize;
	batch->data = data;
	batch->indices = indices;
}

static
u32 transformOpenGLTypes(u32 in)
{
	switch(in) {
		case wShader_NormalizedInt:
			return GL_INT;
		case wShader_NormalizedShort:
			return GL_SHORT;
		case wShader_NormalizedByte:
			return GL_UNSIGNED_BYTE;
		case wShader_Float:
			return GL_FLOAT;
		case wShader_Double:
			return GL_DOUBLE;
		case wShader_FloatInt:
			return GL_INT;
		case wShader_FloatShort:
			return GL_SHORT;
		case wShader_FloatByte:
			return GL_UNSIGNED_BYTE;
		case wShader_Int:
			return GL_INT;
		case wShader_Short:
			return GL_SHORT;
		case wShader_Byte:
			return GL_UNSIGNED_BYTE;
		default:
			return GL_FLOAT;
	}
}

void wConstructBatchGraphicsState(wRenderBatch* batch)
{
	wShader* shader = batch->shader;
	glUseProgram(shader->program);
	if(shader->targetVersion > 21) {
		glGenVertexArrays(1, &batch->vao);
		glBindVertexArray(batch->vao);
	}

	glGenBuffers(1, &batch->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);

	i32 attribTypes[] = {
		GL_FLOAT, GL_DOUBLE, 
		GL_INT, GL_SHORT, GL_UNSIGNED_BYTE,
		GL_INT, GL_SHORT, GL_UNSIGNED_BYTE,
		GL_INT, GL_SHORT, GL_UNSIGNED_BYTE,
		0, 0, 0
	};

	for(isize i = 0; i < shader->attribCount; ++i) {
		wShaderComponent* c = shader->attribs + i;
		i32 isNormalized = 0;
		u32 type = attribTypes[c->type - wShader_Float];
		glEnableVertexAttribArray(c->loc);
		glVertexAttribDivisor(c->loc, c->divisor);
		switch(c->type) {
			case wShader_NormalizedInt:
			case wShader_NormalizedShort:
			case wShader_NormalizedByte:
				isNormalized = 1;
			case wShader_Float:
			case wShader_Double:
			case wShader_FloatInt:
			case wShader_FloatShort:
			case wShader_FloatByte:
				//printf("%s %d -> %x\n", c->name, c->type - wShader_Float, type);
				glVertexAttribPointer(
						c->loc,
						c->count,
						type,
						isNormalized,
						shader->stride,
						(void*)c->ptr);
				break;
			case wShader_Int:
			case wShader_Short:
			case wShader_Byte:
				glVertexAttribIPointer(
						c->loc,
						c->count,
						type,
						shader->stride,
						(void*)c->ptr);
				break;
			default:
				break;
		}
	}

	if(batch->shader->targetVersion > 21) {
		glBindVertexArray(0);
	}
}


void wSetBatchScissorRect(wRenderBatch* batch, i32 enabled, f32 x, f32 y, f32 w, f32 h)
{
	batch->scissor = enabled;
	batch->scissorRect[0] = x;
	batch->scissorRect[1] = y;
	batch->scissorRect[2] = w;
	batch->scissorRect[3] = h;
}


void wDrawBatch(wState* state, wRenderBatch* batch, void* uniformData)
{
	//TODO(will) Add options for other common OpenGL things
	// ie: depthfunc, culling, stencil/scissor
	//
	// Problem: a lot of these are quite powerful and rely on several
	// calls. Either I lock this all down to a few combinations or just
	// expose the API to the user. Right now we only do blend, and only
	// very simple blending at that.
	
	wShader* shader = batch->shader;
	glUseProgram(shader->program);

	if(batch->scissor) {
		glEnable(GL_SCISSOR_TEST);
		f32* s = batch->scissorRect;
		glScissor(s[0], state->height - s[1] - s[3], s[2], s[3]);
	}
	

	if(batch->blend == wRenderBatch_BlendNormal) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else if(batch->blend == wRenderBatch_BlendPremultiplied) {
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	} else if(batch->blend == wRenderBatch_BlendNone) {
		glDisable(GL_BLEND);
	}

	if(uniformData) {
		i32 uniformType[] = {
			wShader_Float, 0,
			wShader_Int, 0, 0, 0, 0, 0, 0, 0, 0,
			wShader_Mat22, wShader_Mat33, wShader_Mat44
		};

		for(isize i = 0; i < shader->uniformCount; ++i) {
			wShaderComponent* c = shader->uniforms + i;
			u32 type = uniformType[c->type - wShader_Float];
			f32* uptrf = (f32*)((usize)uniformData + c->ptr);
			i32* uptri = (i32*)((usize)uniformData + c->ptr);
			switch(type) {
				case 0:
					break;
				case wShader_Float:
					switch(c->count) {
						case 1:
							glUniform1f(c->loc, uptrf[0]);
							break;
						case 2:
							glUniform2f(c->loc, uptrf[0], uptrf[1]);
							break;
						case 3:
							glUniform3f(c->loc, uptrf[0], uptrf[1], uptrf[2]);
							break;
						case 4:
							glUniform4f(c->loc, 
									uptrf[0], uptrf[1], uptrf[2], uptrf[3]);
							break;
						default:
							break;
					}
					break;
				case wShader_Int:
					switch(c->count) {
						case 1:
							glUniform1i(c->loc, uptri[0]);
							break;
						case 2:
							glUniform2i(c->loc, uptri[0], uptri[1]);
							break;
						case 3:
							glUniform3i(c->loc, uptri[0], uptri[1], uptri[2]);
							break;
						case 4:
							glUniform4i(c->loc, 
									uptri[0], uptri[1], uptri[2], uptri[3]);
							break;
						default:
							break;
					}
					break;
				case wShader_Mat44:
					glUniformMatrix4fv(c->loc, c->count, 0, uptrf);
					break;
				case wShader_Mat33:
					glUniformMatrix3fv(c->loc, c->count, 0, uptrf);
					break;
				case wShader_Mat22:
					glUniformMatrix2fv(c->loc, c->count, 0, uptrf);
					break;
				default: 
					break;

			}
		}
	}

	glBindVertexArray(batch->vao);

	u32 hint;
	if(batch->clearOnDraw) {
		hint = GL_STREAM_DRAW;
	} else {
		hint = GL_DYNAMIC_DRAW;
	}

	glBindBuffer(GL_ARRAY_BUFFER, batch->vbo);
	glBufferData(GL_ARRAY_BUFFER, 
			batch->elementSize * batch->elementCount,
			batch->data,
			hint);

	if( 	batch->renderCall == wRenderBatch_Elements || 
			batch->renderCall == wRenderBatch_ElementsInstanced) {
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				batch->indicesCount * sizeof(u32),
				batch->indices,
				hint);
	}
	glBindTexture(GL_TEXTURE_2D, batch->texture->glIndex);

	u32 primitive;
	switch(batch->primitiveMode) {
		case wRenderBatch_Triangles:
			primitive = GL_TRIANGLES;
			break;
		case wRenderBatch_TriangleStrip:
			primitive = GL_TRIANGLE_STRIP;
			break;
		case wRenderBatch_TriangleFan:
			primitive = GL_TRIANGLE_FAN;
			break;
		case wRenderBatch_Lines:
			primitive = GL_LINES;
			break;
		case wRenderBatch_LineStrip:
			primitive = GL_LINE_STRIP;
			break;
		case wRenderBatch_LineLoop:
			primitive = GL_LINE_LOOP;
			break;
		default:
			primitive = GL_TRIANGLES;
	}

	switch(batch->renderCall) {
		case wRenderBatch_Arrays:
			glDrawArrays(primitive, batch->startOffset, batch->elementCount);
			break;
		case wRenderBatch_Elements:
			glDrawElements(primitive, batch->elementCount, GL_UNSIGNED_INT, 
					(void*)(batch->startOffset * sizeof(u32)));
			break;
		case wRenderBatch_ArraysInstanced:
			glDrawArraysInstanced(
					primitive, 
					batch->startOffset,
					batch->instanceSize, 
					batch->elementCount);
			break;
		case wRenderBatch_ElementsInstanced:
			glDrawElementsInstanced(primitive,
					batch->elementCount,
					GL_UNSIGNED_INT,
					(void*)(batch->startOffset * sizeof(u32)),
					batch->instanceSize);
			break;
		default:
			break;
	}
	
	if(batch->clearOnDraw) {
		//TODO(will) this might not work with the ElementsInstanced path
		//	the docs aren't clear as to which part of the call is the 
		//	instance/element count imo
		batch->elementCount = 0;
	}

	if(batch->scissor) {
		glDisable(GL_SCISSOR_TEST);
	}
}

void wUploadTexture(wTexture* texture)
{
	glGenTextures(1, &texture->glIndex);
	glBindTexture(GL_TEXTURE_2D, texture->glIndex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
			texture->w, texture->h, 0, 
			GL_RGBA, GL_UNSIGNED_BYTE, 
			texture->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ASSERT(x)
#include "thirdparty/stb_image.h"

i32 wInitTexture(wTexture* texture, void* data, isize size)
{
	i32 w=0, h=0, bpp=0;
	u8* pixels = stbi_load_from_memory(data, size, &w, &h, &bpp, STBI_rgb_alpha);

	if(w == 0 || h == 0) {
		wLogError(0, "Error: Unable to parse image\n");
		return 0;
	}

	texture->w = w;
	texture->h = h;
	texture->pixels = pixels;
	texture->glIndex = -1;
	return 1;
}
