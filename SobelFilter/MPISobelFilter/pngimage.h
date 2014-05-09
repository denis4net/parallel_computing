#ifndef _PNG_IMAGE
#define _PNG_IMAGE

#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdarg>
#include <iostream>
#include <memory>
#include <CL/cl.hpp>

#define PNG_DEBUG 3
extern "C" {
    #include <png.h>
}

typedef u_int32_t pixel_t;

class PngImage 
{

private:
	int x, y;

    int m_width, m_height;
    png_byte m_colorType;
    png_byte m_bitDepth;

	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
    pixel_t**  m_data;

public:
    PngImage(const char* filename);

    PngImage(const std::string filename);

    PngImage(const PngImage& image);

    PngImage();

    ~PngImage();

    virtual int getWidth() {
        return m_width;
	}

    virtual int getHeight() {
        return m_height;
	}

    virtual pixel_t pixel(int x, int y);

    virtual void setPixel(int x, int y, pixel_t p);

    const pixel_t** getRaw();

    std::shared_ptr<cl_uint> getData();
    int setData(cl_uint* data);

    virtual void read(const char* file_name);

    virtual void write(const char* file_name);

    virtual void print();
};

#endif
