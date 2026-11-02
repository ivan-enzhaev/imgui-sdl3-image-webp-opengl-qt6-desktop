#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// glad.h MUST be included before ANY Qt OpenGL headers!
#include <glad/glad.h>

#include <QElapsedTimer>
#include <QMainWindow>
#include <QOpenGLContext>
#include <QOpenGLWidget>

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // Input events forwarded to ImGui
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    GLuint createTextureWithSDL3(const char *path);

    GLuint m_shaderProgram = 0;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_texture = 0;
    QElapsedTimer m_timer;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    GLWidget *m_glWidget = nullptr;
};

#endif // MAINWINDOW_H
