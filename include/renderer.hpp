#ifndef ALIVE_RENDERER_HPP
#define ALIVE_RENDERER_HPP

#include <vector>
#include <GL/glew.h>
#include "SDL_opengl.h"

// Vertex array object. Contains vertex and index buffers
// Copy-pasted from revolc engine
typedef struct Vao {
    GLuint vao_id;
    GLuint vbo_id;
    GLuint ibo_id;

    int v_count;
    int v_capacity;
    int v_size;
    int i_count;
    int i_capacity;
} Vao;
typedef uint32_t MeshIndexType;
#define MESH_INDEX_GL_TYPE GL_UNSIGNED_INT

Vao create_vao(int max_v_count, int max_i_count);
void destroy_vao(Vao *vao);
void bind_vao(const Vao *vao);
void unbind_vao();
void add_vertices_to_vao(Vao *vao, void *vertices, int count);
void add_indices_to_vao(Vao *vao, MeshIndexType *indices, int count);
void reset_vao_mesh(Vao *vao);
void draw_vao(const Vao *vao);


// These values should match nanovg
enum TextAlign {
	TEXT_ALIGN_LEFT 	= 1<<0,	// Default, align text horizontally to left.
	TEXT_ALIGN_CENTER 	= 1<<1,	// Align text horizontally to center.
	TEXT_ALIGN_RIGHT 	= 1<<2,	// Align text horizontally to right.
	TEXT_ALIGN_TOP 		= 1<<3,	// Align text vertically to top.
	TEXT_ALIGN_MIDDLE	= 1<<4,	// Align text vertically to middle.
	TEXT_ALIGN_BOTTOM	= 1<<5,	// Align text vertically to bottom. 
	TEXT_ALIGN_BASELINE	= 1<<6, // Default, align text vertically to baseline. 
};

struct Color {
    float r, g, b, a;

    static Color white();
};

// For NanoVG wrap
struct RenderPaint {
    float xform[6];
    float extent[2];
    float radius;
    float feather;
    Color innerColor;
    Color outerColor;
    int image;
};

struct BlendMode {
    GLenum srcFactor;
    GLenum dstFactor;
    GLenum equation;
    float colorMul;

    // Some usual blend modes
    static BlendMode normal();
    static BlendMode additive();
    static BlendMode subtractive();
    static BlendMode opaque();
    static BlendMode B100F100();
};

// Internal to Renderer
enum DrawCmdType {
    DrawCmdType_quad,
    DrawCmdType_fillColor,
    DrawCmdType_strokeColor,
    DrawCmdType_strokeWidth,
    DrawCmdType_fontSize,
    DrawCmdType_fontBlur,
    DrawCmdType_textAlign,
    DrawCmdType_text,
    DrawCmdType_resetTransform,
    DrawCmdType_beginPath,
    DrawCmdType_moveTo,
    DrawCmdType_lineTo,
    DrawCmdType_closePath,
    DrawCmdType_fill,
    DrawCmdType_stroke,
    DrawCmdType_roundedRect,
    DrawCmdType_rect,
    DrawCmdType_circle,
    DrawCmdType_solidPathWinding,
    DrawCmdType_fillPaint,
    DrawCmdType_scissor,
    DrawCmdType_resetScissor,
};
struct DrawCmd {
    DrawCmdType type;
    int layer;
    union { // This union is to make the struct smaller, as RenderPaint is quite large
        struct {
            int integer;
            float f[5];
            BlendMode blendMode; // Used only for quads, for now.
            Color color;
            char str[128]; // TODO: Allocate dynamically from cheap frame allocator
        } s;
        RenderPaint paint;
    };
};

// GLES2 renderer
class Renderer {
public:
    
    Renderer(const char *fontPath);
    ~Renderer();

    void beginFrame(int w, int h);
    void endFrame();

    // Layers with larger depth are drawn on top
    void beginLayer(int depth);
    void endLayer();

    int createTexture(GLenum internalFormat, int width, int height, GLenum inputFormat, GLenum colorDataType, const void *pixels, bool interpolation);
    void destroyTexture(int handle);

    // Drawing commands, which will be buffered and issued at the end of the frame.
    // All coordinates are given in pixels.
    // All color components are given in 0..1 floating point.

    // Use negative w or h to flip uv coordinates
    void drawQuad(int texHandle, float x, float y, float w, float h, Color color = Color::white(), BlendMode blendMode = BlendMode::normal());

    // NanoVG wrap
    void fillColor(Color c);
    void strokeColor(Color c);
    void strokeWidth(float size);
    void fontSize(float s);
    void fontBlur(float s);
    void textAlign(int align); // TextAlign bitfield
    void text(float x, float y, const char *msg);
    void resetTransform();
    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void closePath();
    void fill();
    void stroke();
    void roundedRect(float x, float y, float w, float h, float r);
    void rect(float x, float y, float w, float h);
    void circle(float x, float y, float r);
    void solidPathWinding(bool b); // If false, then holes are created
    void fillPaint(RenderPaint p);
    void scissor(float x, float y, float w, float h);
    void resetScissor();

    // Not drawing commands
    RenderPaint linearGradient(float sx, float sy, float ex, float ey, Color sc, Color ec);
    RenderPaint boxGradient(float x, float y, float w, float h,
                            float r, float f, Color icol, Color ocol);
    RenderPaint radialGradient(float cx, float cy, float inr, float outr, Color icol, Color ocol);
    // TODO: Add fontsize param to make independent of "current state"
    void textBounds(int x, int y, const char *msg, float bounds[4]);

private:
    void pushCmd(DrawCmd cmd);

    // Vector rendering
    struct NVGLUframebuffer* mNanoVgFrameBuffer = nullptr;
    struct NVGcontext* mNanoVg = nullptr;
    int mW = 0;
    int mH = 0;

    // Textured quad rendering
    GLuint mVs;
    GLuint mFs;
    GLuint mProgram;
    Vao mQuadVao;

    enum Mode
    {
        Mode_none,
        Mode_vector,
        Mode_quads,
    };

    std::vector<int> mLayerStack;
    std::vector<DrawCmd> mDrawCmds;
    std::vector<int> mDestroyTextureList;
};

#endif