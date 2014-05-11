#ifndef GUI_HPP
#define GUI_HPP

#include "pngimage.h"

#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <cmath>
#include <fstream>

class GUI {
public:
    static int init(int argc=0, char** argv=NULL) {
        std::cout << "GLUT initialization" << std::endl;
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE);
        glutCreateWindow("OpenCL/GL shared texture buffer demo");
        initGraphics();
        glutDisplayFunc(&(GUI::onDisplay));
        glutReshapeFunc(&(GUI::onResize));

    }

    static int loop() {
        glutMainLoop();
        return 0;
    }

    static void setWindowsSize(int height, int width)
    {
        glutInitWindowSize(width, height);
    }

    static void setImage(const PngImage& image)
    {
        PngImage::DataPtr ptr = image.getData();

        glGenTextures(4, textureID);

        glBindTexture(GL_TEXTURE_2D, textureID[0]);
        setTextureParameters();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getWidth(), image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

        glBindTexture(GL_TEXTURE_2D, textureID[1]);
        setTextureParameters();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getWidth(), image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

        glBindTexture(GL_TEXTURE_2D, textureID[2]);
        setTextureParameters();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getWidth(), image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());

        glBindTexture(GL_TEXTURE_2D, textureID[3]);
        setTextureParameters();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getWidth(), image.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr.get());
    }

    static void update()
    {
        onDisplay();
    }

    static GLuint getTextureId(int i) {
        return textureID[i];
    }

private:

    static GLuint textureID[4];

    static void initGraphics()
    {
        glClearColor(0, 0, 0, 0);
        glEnable(GL_TEXTURE_2D);
    }

    static void onResize(int w, int h)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glViewport(0, 0, w, h);
        gluPerspective(40, (float) w / h, 1, 100);
        glMatrixMode(GL_MODELVIEW);
    }

    static void setTextureParameters()
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    static void onDisplay()
    {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        gluLookAt(0.0, 0.0, 2.0,
                  0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0);

        glBindTexture(GL_TEXTURE_2D, getTextureId(0));
        setTextureParameters();
        glRect(-1, 0, 1, 1);

        glBindTexture(GL_TEXTURE_2D, getTextureId(1));
        setTextureParameters();
        glRect(0, 0, 1, 1);


        glBindTexture(GL_TEXTURE_2D, getTextureId(2));
        setTextureParameters();
        glRect(-1, -1, 1, 1);

        glBindTexture(GL_TEXTURE_2D, getTextureId(3));
        setTextureParameters();
        glRect(0, -1, 1, 1);

        glutSwapBuffers();
        glFinish();
    }

    static void glRect(int x, int y, int w, int h) {

        glBegin(GL_POLYGON);
        glTexCoord2f(0, 1); glVertex2f(x, y);
        glTexCoord2f(1, 1); glVertex2f(x+w, y);
        glTexCoord2f(1, 0); glVertex2f(x+w, y+h);
        glTexCoord2f(0, 0); glVertex2f(x, y+h);
        glEnd();

    }
};

GLuint GUI::textureID[] = {0, 1, 2, 3};


#endif // GUI_HPP
