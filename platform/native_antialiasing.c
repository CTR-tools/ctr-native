// FXAA (Timothy Lottes' FxaaPixelShader, NVIDIA's public-domain Fxaa3_11.h),
// "simple" 5-sample variant: luma range over the four diagonal neighbours,
// edge direction from the diagonal gradients, then two blended taps along it.
// It samples point-for-point like the reference implementation, so the source
// texture must stay GL_NEAREST. Alpha carries PSX STP/mask state on both source
// formats, so it passes the centre pixel through untouched.
global_variable const char *ctr_fxaa_shader = "#ifdef VERTEX\n"
                                              "attribute vec2 a_position;\n"
                                              "varying vec2 v_uv;\n"
                                              "void main() {\n"
                                              "\tv_uv = a_position * 0.5 + 0.5;\n"
                                              "\tgl_Position = vec4(a_position, 0.0, 1.0);\n"
                                              "}\n"
                                              "#endif\n"
                                              "#ifdef FRAGMENT\n"
                                              "varying vec2 v_uv;\n"
                                              "uniform sampler2D s_texture;\n"
                                              "uniform vec4 sourceRect;\n"
                                              "uniform vec2 textureSize;\n"
                                              "uniform float flipY;\n"
                                              "uniform float packedSource;\n"
                                              "const vec3 FXAA_LUMA = vec3(0.299, 0.587, 0.114);\n"
                                              "vec4 fxaaSource(vec2 uv) {\n"
                                              "\tvec4 texel = texture2D(s_texture, uv);\n"
                                              "\tif (packedSource < 0.5) {\n"
                                              "\t\treturn texel;\n"
                                              "\t}\n"
                                              "\tivec2 packedBytes = ivec2(texel.rg * 255.0 + 0.5);\n"
                                              "\tint pixel = packedBytes.r | (packedBytes.g << 8);\n"
                                              "\tivec3 color5 = ivec3(pixel & 31, (pixel >> 5) & 31, (pixel >> 10) & 31);\n"
                                              "\tivec3 color8 = (color5 << 3) | (color5 >> 2);\n"
                                              "\treturn vec4(vec3(color8) / 255.0, float((pixel >> 15) & 1));\n"
                                              "}\n"
                                              "void main() {\n"
                                              "\tvec2 pixel = sourceRect.xy + vec2(v_uv.x, mix(v_uv.y, 1.0 - v_uv.y, flipY)) * sourceRect.zw;\n"
                                              "\tvec2 uv = pixel / textureSize;\n"
                                              "\tvec2 texel = 1.0 / textureSize;\n"
                                              "\tvec4 rgbM = fxaaSource(uv);\n"
                                              "\tfloat lumaNW = dot(fxaaSource(uv + vec2(-1.0, -1.0) * texel).rgb, FXAA_LUMA);\n"
                                              "\tfloat lumaNE = dot(fxaaSource(uv + vec2( 1.0, -1.0) * texel).rgb, FXAA_LUMA);\n"
                                              "\tfloat lumaSW = dot(fxaaSource(uv + vec2(-1.0,  1.0) * texel).rgb, FXAA_LUMA);\n"
                                              "\tfloat lumaSE = dot(fxaaSource(uv + vec2( 1.0,  1.0) * texel).rgb, FXAA_LUMA);\n"
                                              "\tfloat lumaM = dot(rgbM.rgb, FXAA_LUMA);\n"
                                              "\tfloat lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
                                              "\tfloat lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"
                                              "\tif (lumaMax - lumaMin < max(0.0312, lumaMax * 0.125)) {\n"
                                              "\t\tfragColor = rgbM;\n"
                                              "\t\treturn;\n"
                                              "\t}\n"
                                              "\tvec2 dir;\n"
                                              "\tdir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
                                              "\tdir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
                                              "\tfloat dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.03125, 0.0078125);\n"
                                              "\tfloat rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);\n"
                                              "\tdir = clamp(dir * rcpDirMin, vec2(-8.0), vec2(8.0)) * texel;\n"
                                              "\tvec4 rgbA = 0.5 * (fxaaSource(uv - dir * (1.0 / 3.0 - 0.5)) + fxaaSource(uv + dir * (2.0 / 3.0 - 0.5)));\n"
                                              "\tvec4 rgbB = rgbA * 0.5 + 0.25 * (fxaaSource(uv - dir * 0.5) + fxaaSource(uv + dir * 0.5));\n"
                                              "\tfloat lumaB = dot(rgbB.rgb, FXAA_LUMA);\n"
                                              "\tvec4 rgbOut = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;\n"
                                              "\tfragColor = vec4(rgbOut.rgb, rgbM.a);\n"
                                              "}\n"
                                              "#endif\n";

// What one pass resolves over, and how its result reaches the viewport.
typedef struct
{
	TextureID texture;
	int rectX, rectY;         // source region, in texels
	int rectW, rectH;
	int textureW, textureH;   // whole source texture, in texels
	int packedSource;         // 15:15:1 VRAM decode and its top-down row order
	GLenum filter;            // filter for the final blit into the viewport
} AntialiasingSource;

global_variable struct NativeRenderTarget s_aaRenderTarget;
global_variable GLuint s_fxaaShader = 0;
global_variable GLint s_fxaaSourceRectLoc = -1;
global_variable GLint s_fxaaTextureSizeLoc = -1;
global_variable GLint s_fxaaFlipYLoc = -1;
global_variable GLint s_fxaaPackedLoc = -1;

int NativeAntialiasing_Enabled(void)
{
	return g_config.fxaa;
}

void NativeAntialiasing_Init(void)
{
	NativeRenderer_InitRenderTarget(&s_aaRenderTarget);

	// Compiled unconditionally so toggling an option never compiles a shader
	// mid-frame. Reuses the fullscreen quad created by the VRAM pipelines.
	s_fxaaShader = NativeRenderer_Shader_Compile(ctr_fxaa_shader, false);
	s_fxaaSourceRectLoc = glGetUniformLocation(s_fxaaShader, "sourceRect");
	s_fxaaTextureSizeLoc = glGetUniformLocation(s_fxaaShader, "textureSize");
	s_fxaaFlipYLoc = glGetUniformLocation(s_fxaaShader, "flipY");
	s_fxaaPackedLoc = glGetUniformLocation(s_fxaaShader, "packedSource");
}

void NativeAntialiasing_Shutdown(void)
{
	NativeRenderer_DestroyRenderTarget(&s_aaRenderTarget);
	glDeleteProgram(s_fxaaShader);
	s_fxaaShader = 0;
}

void NativeAntialiasing_InvalidateTarget(void)
{
	s_aaRenderTarget.width = 0;
	s_aaRenderTarget.height = 0;
}

// Resolve a region of a presentation source and put the result on screen. The
// pass runs into an intermediate target the size of that region so the edge
// search works in source pixels, then the result is blitted to the letterboxed
// viewport. Returns 0 when antialiasing is off or the region is unusable.
static int NativeAntialiasing_PresentSource(const AntialiasingSource *source)
{
	if (!NativeAntialiasing_Enabled() || (source->rectW <= 0) || (source->rectH <= 0))
	{
		return 0;
	}

	const ShaderID previousShader = s_previousShader;
	const TextureID previousTexture = s_lastBoundTexture;
	const BlendMode previousBlendMode = s_previousBlendMode;
	const int previousScissorState = s_previousScissorState;
	const int previousStencilMode = s_previousStencilMode;

	NativeRenderer_EnsureRenderTarget(&s_aaRenderTarget, source->rectW, source->rectH);
	glBindFramebuffer(GL_FRAMEBUFFER, s_aaRenderTarget.framebuffer);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);
	NativeRenderer_SetScissorState(0);
	NativeRenderer_SetViewPort(0, 0, s_aaRenderTarget.width, s_aaRenderTarget.height);

	glUseProgram(s_fxaaShader);
	glUniform4f(s_fxaaSourceRectLoc, (float)source->rectX, (float)source->rectY, (float)source->rectW, (float)source->rectH);
	glUniform2f(s_fxaaTextureSizeLoc, (float)source->textureW, (float)source->textureH);
	glUniform1f(s_fxaaFlipYLoc, source->packedSource ? 1.0f : 0.0f);
	glUniform1f(s_fxaaPackedLoc, source->packedSource ? 1.0f : 0.0f);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, source->texture);
	glBindVertexArray(s_vramQuadVAO);
	NativeRenderer_DrawTriangles(0, 2);

	NativeRenderer_SetViewPort(s_presentViewport.x, s_presentViewport.y, s_presentViewport.w, s_presentViewport.h);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, s_aaRenderTarget.framebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, s_aaRenderTarget.width, s_aaRenderTarget.height,
	                  s_presentViewport.x, s_presentViewport.y,
	                  s_presentViewport.x + s_presentViewport.w, s_presentViewport.y + s_presentViewport.h,
	                  GL_COLOR_BUFFER_BIT, source->filter);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (s_boundVertexBuffer >= 0)
	{
		glBindVertexArray(s_glVertexArray[s_boundVertexBuffer]);
	}
	else
	{
		glBindVertexArray(0);
	}
	glUseProgram(previousShader == (ShaderID)-1 ? 0 : previousShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, previousTexture == (TextureID)-1 ? 0 : previousTexture);
	s_previousShader = previousShader;
	s_lastBoundTexture = previousTexture;
	s_previousBlendMode = BM_NONE;
	s_previousScissorState = 0;
	NativeRenderer_SetBlendMode(previousBlendMode);
	NativeRenderer_SetScissorState(previousScissorState);
	glEnable(GL_STENCIL_TEST);
	if (previousStencilMode != 0)
	{
		glStencilFunc(GL_ALWAYS, 1, 0x10);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	}
	else
	{
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
	}
	s_previousStencilMode = previousStencilMode;
	return 1;
}

int NativeAntialiasing_PresentVRAMRect(int x, int y, int w, int h)
{
	const AntialiasingSource source = {
	    .texture = s_vram.texture,
	    .rectX = x,
	    .rectY = y,
	    .rectW = w,
	    .rectH = h,
	    .textureW = VRAM_WIDTH,
	    .textureH = VRAM_HEIGHT,
	    .packedSource = 1,
	    // The VRAM texture is point-sampled, so the resolved image keeps that
	    // instead of picking up Smooth Scaling.
	    .filter = GL_NEAREST,
	};
	return NativeAntialiasing_PresentSource(&source);
}

int NativeAntialiasing_PresentMainTarget(void)
{
	const AntialiasingSource source = {
	    .texture = s_mainRenderTarget.texture,
	    .rectX = 0,
	    .rectY = 0,
	    .rectW = s_mainRenderTarget.width,
	    .rectH = s_mainRenderTarget.height,
	    .textureW = s_mainRenderTarget.width,
	    .textureH = s_mainRenderTarget.height,
	    .packedSource = 0,
	    // The main target already presents through Smooth Scaling, so the
	    // resolved image has to scale the same way.
	    .filter = g_config.smoothScaling ? GL_LINEAR : GL_NEAREST,
	};
	return NativeAntialiasing_PresentSource(&source);
}
