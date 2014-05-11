#include <typeinfo>
#include <ctime>

#include "pngimage.h"
#include "GUI.hpp"
#include "OpenCLEngine.hpp"


OpenCLEngine clEngine;

void keyHandler(unsigned char kcode, int x, int y)
{
    static float factor;

    switch (kcode) {
    case 'a':
        clEngine.run();
        GUI::update();
        break;
    case 'n':
        clEngine.runCLNoise(std::time(NULL), factor);
        clEngine.runCLCopy();
        GUI::update();
        break;
    case '-':
        factor -= 0.1;
        clEngine.runCLNoise(std::time(NULL), factor);
        clEngine.runCLCopy();
        GUI::update();
        break;
    case '=':
        factor += 0.1;
        clEngine.runCLNoise(std::time(NULL), factor);
        clEngine.runCLCopy();
        GUI::update();
        break;
    default:
        std::cerr << "Invalid hotkey " << kcode << std::endl;
        break;
    }
}

int main(int argc, char * argv[])
{
    PngImage image("./input.png");

    GUI::init(argc, argv);

    clEngine.setup();

    GUI::setImage(image);

    GLuint id[] = {GUI::getTextureId(0), GUI::getTextureId(1), GUI::getTextureId(2), GUI::getTextureId(3), ~((GLuint)0)};
    int count = clEngine.addTextures(id, image.getWidth(), image.getHeight());
    std::cout << "Added " << count << " textures to OpenCL engine" << std::endl;

    glutKeyboardFunc(keyHandler);

    return GUI::loop();
}



