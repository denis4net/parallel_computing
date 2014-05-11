constant sampler_t imageSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

kernel void main(read_only image2d_t srcImage, write_only image2d_t outImage)
{
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    float4 pixel = read_imagef(srcImage, imageSampler, coord);
    write_imagef(outImage, coord, pixel);
}
