#include "pngimage.h"
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

    image_bytes = (pixel_t**) malloc(sizeof(pixel_t*) * m_height);
    for (y=0; y<m_height; y++) {
        image_bytes[y] = (pixel_t*) malloc(sizeof(pixel_t) * getWidth());
        memcpy(image_bytes[y], image.image_bytes[y], sizeof(sizeof(pixel_t) * getWidth()));
    }
}

PngImage::PngImage() {
   image_bytes = nullptr;
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
    return image_bytes[y][x];
}

void PngImage::setPixel(int x, int y, pixel_t p)
{
    image_bytes[y][x] = p;
}

const pixel_t **PngImage::getRaw() {
    return const_cast<const pixel_t**> (image_bytes);
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

    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    m_width = png_get_image_width(png_ptr, info_ptr);
    m_height = png_get_image_height(png_ptr, info_ptr);
    m_colorType = png_get_color_type(png_ptr, info_ptr);
    m_bitDepth = png_get_bit_depth(png_ptr, info_ptr);

    std::cout << m_width << "x" << m_height << " bit depth:" << (int) m_bitDepth << std::endl;

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[read_png_file] Error during read_image");

    const int bytes_per_line = png_get_rowbytes(png_ptr,info_ptr);

    image_bytes = (pixel_t**) malloc(sizeof(pixel_t*) * m_height);
    for (y=0; y<m_height; y++)
        image_bytes[y] = (pixel_t*) malloc(bytes_per_line);


    png_read_image(png_ptr, (png_bytepp) image_bytes);

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

    if (!png_ptr)
        abort_("[write_png_file] png_create_write_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        abort_("[write_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during init_io");

    png_init_io(png_ptr, fp);


    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during writing header");

    png_set_IHDR(png_ptr, info_ptr, m_width, m_height,
                 m_bitDepth, m_colorType, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during writing bytes");

    png_write_image(png_ptr, (png_bytepp) image_bytes);


    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);

   fclose(fp);
}

void PngImage::print() {
    for (int i=0; i<getHeight(); i++) {
        for (int j=0; j<getWidth(); j++) {
            std::cout << std::hex << (int) pixel(j, i) << " ";
        }
        std::cout << std::endl;
    }
}
