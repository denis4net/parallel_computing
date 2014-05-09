#include "pngimage.h"
#include <memory>
#include <iostream>

extern "C" {
#include <png.h>
}

const void abort_(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}



PngImage::PngImage(const char *filename) {
    read(filename);
}

PngImage::PngImage(const std::string filename) {
    read(filename.c_str());
}

PngImage::PngImage(const PngImage &image)
{
    m_width = image.m_width;
    m_height = image.m_height;
    m_bitDepth = image.m_bitDepth;
    m_colorType = image.m_colorType;
    info_ptr = image.info_ptr;
    png_ptr = image.png_ptr;

    m_data = (pixel_t**) malloc(sizeof(pixel_t*) * m_height);
    for (y=0; y<m_height; y++) {
        m_data[y] = (pixel_t*) malloc(sizeof(pixel_t) * getWidth());
        memcpy(m_data[y], image.m_data[y], sizeof(sizeof(pixel_t) * getWidth()));
    }
}

PngImage::PngImage() {
    m_data = nullptr;
}

PngImage::~PngImage()
{
#if 0
    if (image_bytes != nullptr)
        for (int i=0; i<getHeight(); i++)
            free(image_bytes[i]);

    image_bytes = nullptr;
#endif
}

pixel_t PngImage::pixel(int x, int y)
{
    return m_data[y][x];
}

void PngImage::setPixel(int x, int y, pixel_t p)
{
    m_data[y][x] = p;
}

const pixel_t **PngImage::getRaw() {
    return const_cast<const pixel_t**> (m_data);
}

std::shared_ptr<cl_uint> PngImage::getData()
{
    cl_uint* data = (cl_uint*) malloc(getWidth()*getHeight()*sizeof(cl_uint));

    for (int y=0; y<getHeight(); y++) {
        for (int x=0; x<getWidth(); x++) {
            data[x+y*getWidth()] = pixel(x,y);
        }
    }
    return std::shared_ptr<cl_uint>(data);
}

int PngImage::setData(cl_uint *data)
{


    for (int y=0; y<getHeight(); y++) {
        for (int x=0; x<getWidth(); x++) {
            cl_uint uc = data[x+y*getWidth()];
            setPixel(x, y, (pixel_t) uc);
        }
    }
}



void PngImage::read(const char *file_name)
{
    // 8 is the maximum size that can be checked
    png_const_bytep header = (png_const_bytep) malloc(8 * sizeof(png_byte));
    // open file and test for it being a png
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
        abort_("[read_png_file] File %s could not be opened for reading", file_name);

    std::fread((void*) header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
        abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        abort_("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr)
        abort_("[read_png_file] png_create_info_struct failed");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    m_width = png_get_image_width(png_ptr, info_ptr);
    m_height = png_get_image_height(png_ptr, info_ptr);
    m_colorType = png_get_color_type(png_ptr, info_ptr);
    m_bitDepth = png_get_bit_depth(png_ptr, info_ptr);

    std::cout << m_width << "x" << m_height << " bit depth:" << (int) m_bitDepth << std::endl;
    std::cout << "Color type: " << (int) m_colorType << std::endl;
    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    const int bytes_per_line = png_get_rowbytes(png_ptr,info_ptr);
    std::cout << "Bytes per pixel: " << bytes_per_line/getWidth() << std::endl;
    m_data = (pixel_t**) malloc(sizeof(pixel_t*) * m_height);
    for (y=0; y<m_height; y++) {
        m_data[y] = (pixel_t*) malloc(bytes_per_line);
        memset(m_data[y], 0xFF, bytes_per_line);
    }

    png_read_image(png_ptr, (png_bytepp) m_data);

    fclose(fp);
    free((void*)header);
}

void PngImage::write(const char *file_name)
{
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp)
        abort_("[write_png_file] File %s could not be opened for writing", file_name);


    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    info_ptr = png_create_info_struct(png_ptr);

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, m_width, m_height,
                 m_bitDepth, m_colorType, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, (png_bytepp) m_data);
    png_write_end(png_ptr, NULL);

    fclose(fp);
}

void PngImage::print() {
    std::cout << (int) m_bitDepth << ' ' << (int) m_colorType << std::endl;

    for (int i=0; i<getHeight(); i++) {
        for (int j=0; j<getWidth(); j++) {
            std::cout << std::hex << (int) pixel(j, i) << " ";
        }
        std::cout << std::endl;
    }
}
