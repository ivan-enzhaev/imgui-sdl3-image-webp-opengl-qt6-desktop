#include "mainwindow.h"

// SDL3 and SDL3_image headers
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

// Helper to translate Qt Key to ImGuiKey
static ImGuiKey QtKeyToImGuiKey(int key)
{
    switch (key)
    {
        case Qt::Key_Tab:
            return ImGuiKey_Tab;
        case Qt::Key_Left:
            return ImGuiKey_LeftArrow;
        case Qt::Key_Right:
            return ImGuiKey_RightArrow;
        case Qt::Key_Up:
            return ImGuiKey_UpArrow;
        case Qt::Key_Down:
            return ImGuiKey_DownArrow;
        case Qt::Key_PageUp:
            return ImGuiKey_PageUp;
        case Qt::Key_PageDown:
            return ImGuiKey_PageDown;
        case Qt::Key_Home:
            return ImGuiKey_Home;
        case Qt::Key_End:
            return ImGuiKey_End;
        case Qt::Key_Insert:
            return ImGuiKey_Insert;
        case Qt::Key_Delete:
            return ImGuiKey_Delete;
        case Qt::Key_Backspace:
            return ImGuiKey_Backspace;
        case Qt::Key_Space:
            return ImGuiKey_Space;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            return ImGuiKey_Enter;
        case Qt::Key_Escape:
            return ImGuiKey_Escape;
        case Qt::Key_Control:
            return ImGuiKey_LeftCtrl;
        case Qt::Key_Shift:
            return ImGuiKey_LeftShift;
        case Qt::Key_Alt:
            return ImGuiKey_LeftAlt;
        default:
            if (key >= Qt::Key_A && key <= Qt::Key_Z)
                return (ImGuiKey)(ImGuiKey_A + (key - Qt::Key_A));
            if (key >= Qt::Key_0 && key <= Qt::Key_9)
                return (ImGuiKey)(ImGuiKey_0 + (key - Qt::Key_0));
            return ImGuiKey_None;
    }
}

// Textured Quad Shaders
static const char *vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char *fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
)";

// ==========================================
// GLWidget Implementation
// ==========================================
GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    if (m_texture)
        glDeleteTextures(1, &m_texture);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo)
        glDeleteBuffers(1, &m_vbo);
    if (m_shaderProgram)
        glDeleteProgram(m_shaderProgram);
    doneCurrent();
}

GLuint GLWidget::createTextureWithSDL3(const char *path)
{
    SDL_IOStream *io = SDL_IOFromFile(path, "rb");
    if (!io)
    {
        qWarning("Failed to open file stream for %s: %s", path, SDL_GetError());
        return 0;
    }

    // 1 (true) will automatically close and free the SDL_IOStream
    SDL_Surface *surface = IMG_Load_IO(io, 1);
    if (!surface)
    {
        qWarning("Image loading failed for %s: %s", path, SDL_GetError());
        return 0;
    }

    // Convert to RGBA ABGR8888 for OpenGL
    SDL_Surface *optimizedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surface);

    if (!optimizedSurface)
    {
        qWarning("Surface conversion failed: %s", SDL_GetError());
        return 0;
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Check anisotropic filtering support using Qt's QOpenGLContext
    bool hasAnisotropy = false;
    if (auto *ctx = QOpenGLContext::currentContext())
    {
        hasAnisotropy = ctx->hasExtension(QByteArrayLiteral("GL_EXT_texture_filter_anisotropic"));
    }

    if (hasAnisotropy)
    {
        float maxAnisotropy = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
        qInfo("Anisotropic filtering enabled. Max: %.1f", maxAnisotropy);
    }
    else
    {
        qInfo("Anisotropic filtering not supported.");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, optimizedSurface->w, optimizedSurface->h,
        0, GL_RGBA, GL_UNSIGNED_BYTE, optimizedSurface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_DestroySurface(optimizedSurface);
    return textureID;
}

void GLWidget::initializeGL()
{
    // Initialize GLAD with Qt's OpenGL proc address loader
    bool ok = gladLoadGLLoader([](const char *name) -> void * {
        return reinterpret_cast<void *>(QOpenGLContext::currentContext()->getProcAddress(name));
    });

    if (!ok)
    {
        qFatal("Failed to initialize GLAD using Qt OpenGL context!");
    }

    // Shaders
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fs);

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vs);
    glAttachShader(m_shaderProgram, fs);
    glLinkProgram(m_shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Geometry: Textured Quad
    float vertices[] = {
        // Positions   // TexCoords
        -0.5f, 0.5f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 1.0f, 1.0f,

        -0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 1.0f, 1.0f,
        0.5f, 0.5f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Load WebP texture using SDL3_image
    m_texture = createTextureWithSDL3("assets/textures/clear_01.webp");

    // Dear ImGui Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplOpenGL3_Init("#version 330");

    m_timer.start();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw textured quad in background
    if (m_texture)
    {
        glUseProgram(m_shaderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    // Render ImGui UI on top
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width(), (float)height());
    float dpr = (float)devicePixelRatio();
    io.DisplayFramebufferScale = ImVec2(dpr, dpr);

    float dt = (float)m_timer.restart() / 1000.0f;
    io.DeltaTime = dt > 0.0f ? dt : (1.0f / 60.0f);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // ImGui Preview Window for the WebP texture
    ImGui::Begin("SDL3_image WebP Texture Preview");
    if (m_texture)
    {
        ImGui::Text("Loaded OpenGL Texture ID: %u", m_texture);
        ImGui::Image((ImTextureID)(intptr_t)m_texture, ImVec2(200, 200));
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Failed to load assets/textures/clear_01.webp");
    }
    ImGui::End();

    ImGui::ShowDemoWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Input handling
void GLWidget::mousePressEvent(QMouseEvent *event)
{
    int button = -1;
    if (event->button() == Qt::LeftButton)
        button = 0;
    if (event->button() == Qt::RightButton)
        button = 1;
    if (event->button() == Qt::MiddleButton)
        button = 2;

    if (button != -1)
        ImGui::GetIO().AddMouseButtonEvent(button, true);

    update();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    int button = -1;
    if (event->button() == Qt::LeftButton)
        button = 0;
    if (event->button() == Qt::RightButton)
        button = 1;
    if (event->button() == Qt::MiddleButton)
        button = 2;

    if (button != -1)
        ImGui::GetIO().AddMouseButtonEvent(button, false);

    update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    ImGui::GetIO().AddMousePosEvent((float)event->position().x(), (float)event->position().y());
    update();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;

    if (!numPixels.isNull())
        ImGui::GetIO().AddMouseWheelEvent((float)numPixels.x() * 0.1f, (float)numPixels.y() * 0.1f);
    else if (!numDegrees.isNull())
        ImGui::GetIO().AddMouseWheelEvent((float)numDegrees.x() / 15.0f, (float)numDegrees.y() / 15.0f);

    update();
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    ImGuiKey imguiKey = QtKeyToImGuiKey(event->key());
    if (imguiKey != ImGuiKey_None)
        ImGui::GetIO().AddKeyEvent(imguiKey, true);

    if (!event->text().isEmpty())
        ImGui::GetIO().AddInputCharactersUTF8(event->text().toUtf8().constData());

    update();
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{
    ImGuiKey imguiKey = QtKeyToImGuiKey(event->key());
    if (imguiKey != ImGuiKey_None)
        ImGui::GetIO().AddKeyEvent(imguiKey, false);

    update();
}

// ==========================================
// MainWindow Implementation
// ==========================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Qt 6 + ImGui 1.93 + SDL3_image (WebP)");
    resize(1280, 720);

    m_glWidget = new GLWidget(this);
    setCentralWidget(m_glWidget);
}
