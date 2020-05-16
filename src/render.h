/*   Graphene renderer

Vertices for quads are specified in STRIP order:

  0      1
  ┌──────┐
  │      │
  └──────┘
  2      3

*/

struct image {
    u32 Texture;

    u32 Width;
    u32 Height;
    u32 N;

    u8 *Data;
};

enum shader_ {
    Shader_Plain,
    Shader_Textured,
    Shader_Glyph,
    Shader_Count,
};

enum draw_mode_ {
    DrawMode_Quad,
    DrawMode_Triangle,
    DrawMode_Strip,
};

struct command_data {
    shader_ Shader;
    GLuint Texture;
    v2i QuadDim;
};

struct render_command {
    draw_mode_ DrawMode;
    u32 Offset;
    u32 ElementCount;
    command_data Data;
};

struct vertex_xyzrgba {
    v3 P;
    v4 Color;
};

struct vertex_xyzrgbauv {
    v3 P;
    v4 Color;
    v2 UV;
};

struct shader {
    ls_static_string<32> Name;
    GLuint Id;
};

struct renderer {
    char *ShaderError;
    v2 Screen;

    GLuint VertexArrayPlain;
    GLuint VertexArrayTextured;
    GLuint VertexBufferPlain;
    GLuint VertexBufferTextured;
    GLuint ViewUniformBuffer;

    shader Shaders[Shader_Count];
    GLuint QuadDimUniform;

    m4x4 ProjectionMatrix;
    shader_ Shader;
    u32 Texture;

    vertex_xyzrgba *PlainVertices;
    u32 PlainVertexCount;

    vertex_xyzrgbauv *TexturedVertices;
    u32 TexturedVertexCount;

    render_command Commands[10000];
    u32 CommandCount;

    void SetMatrix(v2 CamP, b32 YIsUp, b32 Centered, r32 Scale);
    void Flush();
};

static renderer Renderer = {};

#define VERTEX_BUFFER_SIZE 10000
#define COMMAND_BUFFER_SIZE 100

void renderer::
Flush()
{
    u32 ScreenWidth = this->Screen.x;
    u32 ScreenHeight = this->Screen.y;

    glBindBuffer(GL_UNIFORM_BUFFER, Renderer.ViewUniformBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(m4x4), (u8 *)&Renderer.ProjectionMatrix);

    if (Renderer.Shader == Shader_Plain) {
        glBindVertexArray(Renderer.VertexArrayPlain);
        glBindBuffer(GL_ARRAY_BUFFER, Renderer.VertexBufferPlain);
    } else if (Renderer.Shader == Shader_Textured) {
        // glBindVertexArray(Renderer.VertexArrayTextured);
        // glBindBuffer(GL_ARRAY_BUFFER, Renderer.VertexBufferTextured);
        // glBindTexture(GL_TEXTURE_2D, Command.Data.Texture);

        // EM_ASM(console.log($0), Command.Data.Texture);
    } else if (Renderer.Shader == Shader_Glyph) {
        glBindVertexArray(Renderer.VertexArrayTextured);
        glBindBuffer(GL_ARRAY_BUFFER, Renderer.VertexBufferTextured);
        glBindTexture(GL_TEXTURE_2D, Renderer.Texture);

        // EM_ASM(console.log($0), Command.Data.Texture);
    }

    if (Renderer.PlainVertexCount) {
        glBindBuffer(GL_ARRAY_BUFFER, Renderer.VertexBufferPlain);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer.PlainVertexCount * sizeof(vertex_xyzrgba), (void *)Renderer.PlainVertices);
    } else if (Renderer.TexturedVertexCount) {
        glBindBuffer(GL_ARRAY_BUFFER, Renderer.VertexBufferTextured);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Renderer.TexturedVertexCount * sizeof(vertex_xyzrgbauv), (void *)Renderer.TexturedVertices);
    }

    glUseProgram(Renderer.Shaders[Renderer.Shader].Id);

    for (u32 i=0; i<Renderer.CommandCount; ++i) {
        render_command C = Renderer.Commands[i];

        if (C.DrawMode == DrawMode_Quad) {
            glDrawArrays(GL_TRIANGLES, C.Offset, 6 * C.ElementCount);
        } else if (C.DrawMode == DrawMode_Triangle) {
            glDrawArrays(GL_TRIANGLES, C.Offset, 3 * C.ElementCount);
        } else if (C.DrawMode == DrawMode_Strip) {
            glDrawArrays(GL_TRIANGLE_STRIP, C.Offset, C.ElementCount);
        }
    }

    this->TexturedVertexCount = 0;
    this->PlainVertexCount = 0;
    this->CommandCount = 0;
}

void renderer::
SetMatrix(v2 CamP, b32 YIsUp, b32 Centered, r32 Scale)
{
    m4x4 Result = {};

    r32 a = SafeDivide1(2.0f, this->Screen.x * Scale);
    r32 b = -1.0f * SafeDivide1(2.0f, this->Screen.y * Scale);
    r32 c = SafeDivide1(1.0f, 1000.0f - 0.0f);
    r32 d = 1.0f;

    v2 T = v2{ (2.0f * CamP.x) / this->Screen.x, (CamP.y * 2.0f) / this->Screen.y };

    // centering offset
    v2 C = {};

    if (Centered) {
        C.x = -1;
        C.y = 1;
    }

    if (YIsUp) {
        d = -1.0f;
        b *= -1.0f;

        if (Centered) {
            C.y = -1;
        }
    }

    T.x += C.x;
    T.y += C.y;

    this->ProjectionMatrix = m4x4{
        a,  0,  0, 0,
        0,  b,  0, 0,
        0,  0, -c, 0,
        -1 - T.x, d - T.y, 0, 1
    };
}

void
DrawQuad(v2 A, v2 B, v2 C, v2 D, v4 Color, r32 Z)
{
    if (Renderer.CommandCount && Renderer.Shader != Shader_Plain) {
        Renderer.Flush();
    }

    Renderer.Shader = Shader_Plain;

    vertex_xyzrgba *Vertices = Renderer.PlainVertices + Renderer.PlainVertexCount;

    Assert(Renderer.PlainVertexCount + 6 <= VERTEX_BUFFER_SIZE);

    Vertices[0].P = v3{A.x, A.y, Z};
    Vertices[1].P = v3{B.x, B.y, Z};
    Vertices[2].P = v3{C.x, C.y, Z};
    Vertices[3].P = v3{D.x, D.y, Z};

    Vertices[0].Color = Color / 255.0f;
    Vertices[1].Color = Color / 255.0f;
    Vertices[2].Color = Color / 255.0f;
    Vertices[3].Color = Color / 255.0f;

    render_command *Command = Renderer.Commands + Renderer.CommandCount;
    Command->DrawMode = DrawMode_Strip;
    Command->Offset = Renderer.PlainVertexCount;
    Command->ElementCount = 4;

    Renderer.PlainVertexCount += 4;
    ++Renderer.CommandCount;
}

void
DrawRect(v4 Color, v2 P, v2 Dim, r32 Z)
{
    if (Renderer.CommandCount && Renderer.Shader != Shader_Plain) {
        Renderer.Flush();
    }

    Renderer.Shader = Shader_Plain;

    vertex_xyzrgba *Vertices = Renderer.PlainVertices + Renderer.PlainVertexCount;

    Assert(Renderer.PlainVertexCount + 6 <= VERTEX_BUFFER_SIZE);

    Vertices[0].P = v3{P.x, P.y, Z};
    Vertices[1].P = v3{P.x + Dim.x, P.y, Z};
    Vertices[2].P = v3{P.x, P.y + Dim.y, Z};
    Vertices[3].P = v3{P.x + Dim.x, P.y + Dim.y, Z};

    Vertices[0].Color = Color / 255.0f;
    Vertices[1].Color = Color / 255.0f;
    Vertices[2].Color = Color / 255.0f;
    Vertices[3].Color = Color / 255.0f;

    render_command *C = Renderer.Commands + Renderer.CommandCount;
    C->DrawMode = DrawMode_Strip;
    C->Offset = Renderer.PlainVertexCount;
    C->ElementCount = 4;

    Renderer.PlainVertexCount += 4;
    ++Renderer.CommandCount;
}

void
DrawCircle(v4 Color, v2 Center, r32 R, r32 Z)
{
    if (Renderer.CommandCount && Renderer.Shader != Shader_Plain) {
        Renderer.Flush();
    }

    Renderer.Shader = Shader_Plain;

    vertex_xyzrgba *Vertices = Renderer.PlainVertices + Renderer.PlainVertexCount;

    u32 StepCount = 8 * Max_r32(1, R - 7);
    Assert(Renderer.PlainVertexCount + (StepCount * 3) <= VERTEX_BUFFER_SIZE);

    r32 StepRad = (2.0f * PI) / StepCount;
    u32 Steps = (2.0f * PI) / StepRad;
    r32 Rad = 0;

    Vertices[0].P = v3{1.0f, 0.0f, Z};
    Vertices[0].Color = Color / 255.0f;

    u32 VIndex = 1;

    for (u32 i=1; i<StepCount; ++i) {
        Rad += StepRad;

        Vertices[VIndex + 0].P = v3{(r32)cos(Rad), (r32)sin(Rad), Z};
        Vertices[VIndex + 0].Color = Color / 255.0f;

        Vertices[VIndex + 1].P = v3{0.0, 0.0f, Z};
        Vertices[VIndex + 1].Color = Color / 255.0f;

        Vertices[VIndex + 2].P = v3{Vertices[VIndex - 1].P.x, Vertices[VIndex - 1].P.y, Z};
        Vertices[VIndex + 2].Color = Color / 255.0f;

        VIndex += 3;
    }

    for (u32 i=0; i<StepCount*3; ++i) {
        Vertices[i].P.x *= R;
        Vertices[i].P.y *= R;
        Vertices[i].P.xy += Center;
    }

    render_command *C = Renderer.Commands + Renderer.CommandCount;
    C->DrawMode = DrawMode_Triangle;
    C->Offset = Renderer.PlainVertexCount;
    C->ElementCount = StepCount;

    Renderer.PlainVertexCount += StepCount * 3;
    ++Renderer.CommandCount;
}

void
DrawLine(v2 Start, v2 End, r32 Thickness, v4 Color, r32 Z)
{
    v2 P = Normalize(Perp(End-Start));

    v2 A = Start + P * Thickness;
    v2 B = End + P * Thickness;
    v2 C = Start - P * Thickness;
    v2 D = End - P * Thickness;

    DrawQuad(A, B, C, D, Color, Z);
}

#if 0
void
DrawImage(v2 P, image Image, r32 Scale, u32 Z=0)
{
    vertex_xyzrgbauv *Vertices = Renderer.TexturedVertices + Renderer.TexturedVertexCount;

    Assert(Renderer.TexturedVertexCount + 4 <= VERTEX_BUFFER_SIZE);

    Vertices[0].P = v3{P.x, P.y, (r32)Z};
    Vertices[1].P = v3{P.x + Image.Width, P.y, (r32)Z};
    Vertices[2].P = v3{P.x, P.y + Image.Height, (r32)Z};
    Vertices[3].P = v3{P.x + Image.Width, P.y + Image.Height, (r32)Z};

    v4 Color = RGBA(255,0,255,255);
    Vertices[0].Color = Color / 255.0f;
    Vertices[1].Color = Color / 255.0f;
    Vertices[2].Color = Color / 255.0f;
    Vertices[3].Color = Color / 255.0f;

    Vertices[0].UV = v2{0.0f, 0.0f};
    Vertices[1].UV = v2{1.0f, 0.0f};
    Vertices[2].UV = v2{0.0f, 1.0f};
    Vertices[3].UV = v2{1.0f, 1.0f};

    command_data Data = {};
    Data.Shader = Shader_Textured;
    Data.Texture = Image.Texture;

    // AddRendesrCommand(DrawMode_Strip, Renderer.TexturedVertexCount, 4, Data);
    Renderer.TexturedVertexCount += 4;
}

// void
// DrawTexturedRect(render *Render, v2 P, v2 Dim, v4 Color, u32 Texture, u32 Z=0)
// {
//     vertex_xyzrgbauv *Vertices = Renderer.TexturedVertices + Renderer.TexturedVertexCount;

//     Assert(Renderer.TexturedVertexCount + 4 <= VERTEX_BUFFER_SIZE);

//     Vertices[0].P = v3{P.x, P.y, (r32)Z};
//     Vertices[1].P = v3{P.x + Dim.x, P.y, (r32)Z};
//     Vertices[2].P = v3{P.x, P.y + Dim.y, (r32)Z};
//     Vertices[3].P = v3{P.x + Dim.x, P.y + Dim.y, (r32)Z};

//     Vertices[0].Color = Color / 255.0f;
//     Vertices[1].Color = Color / 255.0f;
//     Vertices[2].Color = Color / 255.0f;
//     Vertices[3].Color = Color / 255.0f;

//     Vertices[0].UV = v2{0.0f, 0.0f};
//     Vertices[1].UV = v2{1.0f, 0.0f};
//     Vertices[2].UV = v2{0.0f, 1.0f};
//     Vertices[3].UV = v2{1.0f, 1.0f};

//     command_data Data = {};
//     Data.Shader = Shader_Textured;

//     assert(!"texture is wrong! fix me!");
//     // Data.Texture = Renderer.TestTexture;
//     AddRenderCommand(Render, DrawMode_Strip, Renderer.TexturedVertexCount, 4, Data);
//     Renderer.TexturedVertexCount += 4;
// }
#endif

void
DrawText(v2 P, r32 Z, r32 Scale, v4 Color, cached_font *Font, char *Text, r32 Len)
{
    if (Renderer.CommandCount && (Renderer.Shader != Shader_Glyph || Font->Atlas.Texture != Renderer.Texture)) {
        Renderer.Flush();
    }

    Renderer.Shader = Shader_Glyph;
    Renderer.Texture = Font->Atlas.Texture;

    v2 CurrentP = P;
    u32 PreviousCodePoint = 0;

    u32 IntegerLen = (u32)(Len);

    for (u32 i=0; i<Len; ++i) {
        v4 GlyphColor = Color;

        if (i == IntegerLen) {
            GlyphColor.a *= Len - IntegerLen;
        }

        vertex_xyzrgbauv *Vertices = Renderer.TexturedVertices + Renderer.TexturedVertexCount;

        Assert(Renderer.TexturedVertexCount + 6 <= VERTEX_BUFFER_SIZE);

        cached_glyph *Glyph = GetCachedGlyph(Font, Text[i]);

        v2 TextureOffsetPx = {1.0, 1.0f};

        r32 XKern = GetKerningForPair(Font, PreviousCodePoint, Glyph->CodePoint);
        r32 Left = XKern + Glyph->LeftBearing;
        r32 Right = Glyph->XAdvance - (Glyph->Width + Glyph->LeftBearing);
        r32 Width = (Glyph->Width + Left + Right) * Scale;
        v2 QuadDim = v2{(r32)Glyph->BitmapWidth, (r32)Glyph->BitmapHeight} * Scale;
        QuadDim.x += TextureOffsetPx.x;
        QuadDim.y += TextureOffsetPx.y;

        v2 GlyphP = CurrentP;
        GlyphP.x += Left * Scale - TextureOffsetPx.x;
        GlyphP.y += (Font->Baseline - Glyph->BitmapTop) * Scale - TextureOffsetPx.y;

        Vertices[0].P = v3{GlyphP.x, GlyphP.y, (r32)Z};
        Vertices[1].P = v3{GlyphP.x + QuadDim.x, GlyphP.y, (r32)Z};
        Vertices[2].P = v3{GlyphP.x, GlyphP.y + QuadDim.y, (r32)Z};

        Vertices[3].P = v3{GlyphP.x + QuadDim.x, GlyphP.y, (r32)Z};
        Vertices[4].P = v3{GlyphP.x, GlyphP.y + QuadDim.y, (r32)Z};
        Vertices[5].P = v3{GlyphP.x + QuadDim.x, GlyphP.y + QuadDim.y, (r32)Z};

        Vertices[0].Color = GlyphColor / 255.0f;
        Vertices[1].Color = GlyphColor / 255.0f;
        Vertices[2].Color = GlyphColor / 255.0f;
        Vertices[3].Color = GlyphColor / 255.0f;
        Vertices[4].Color = GlyphColor / 255.0f;
        Vertices[5].Color = GlyphColor / 255.0f;

        v2 TexelOffset = { TextureOffsetPx.x / (Font->Atlas.Width * Scale), TextureOffsetPx.y / (Font->Atlas.Height * Scale) };

        Vertices[0].UV = Glyph->UV.TopLeft - TexelOffset;
        Vertices[1].UV = Glyph->UV.TopRight + v2{TexelOffset.x, -TexelOffset.y};
        Vertices[2].UV = Glyph->UV.BottomLeft + v2{-TexelOffset.x, TexelOffset.y};
        Vertices[3].UV = Glyph->UV.TopRight + v2{TexelOffset.x, -TexelOffset.y};
        Vertices[4].UV = Glyph->UV.BottomLeft + v2{-TexelOffset.x, TexelOffset.y};
        Vertices[5].UV = Glyph->UV.BottomRight + TexelOffset;

        render_command *C = Renderer.Commands + Renderer.CommandCount;
        C->DrawMode = DrawMode_Quad;
        C->Offset = Renderer.TexturedVertexCount;
        C->ElementCount = 1;

        Renderer.TexturedVertexCount += 6;
        ++Renderer.CommandCount;

        CurrentP.x += Width;
        if (Layout.FontSpacing > 0.0f) {
            CurrentP.x += Width * Layout.FontSpacing;
        }
        PreviousCodePoint = Glyph->CodePoint;
    }
}

void
DrawText(v2 P, r32 Z, v4 Color, font_ FontId, r32 SizePx, char *Text)
{
    r32 Scale;
    cached_font *Font = FindMatchingFont(FontId, SizePx, &Scale);
    DrawText(P, Z, Scale, Color, Font, Text, strlen(Text));
}
