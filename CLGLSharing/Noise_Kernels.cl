constant sampler_t imageSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

float4 rand(uint2 *state)
{
    const float4 invMaxInt = (float4) (1.0f/4294967296.0f, 1.0f/4294967296.0f, 1.0f/4294967296.0f, 0);
    uint x = (*state).x * 17 + (*state).y * 13123;
    (*state).x = (x<<13) ^ x;
    (*state).y ^= (x<<7);

    uint4 tmp = (uint4)
    ( (x * (x * x * 15731 + 74323) + 871483),
      (x * (x * x * 13734 + 37828) + 234234),
      (x * (x * x * 11687 + 26461) + 137589), 0 );

    return convert_float4(tmp) * invMaxInt;
}

kernel void main(read_only image2d_t srcImage, write_only image2d_t outImage, uint seed, float f)
{
    int2 coord = (int2)(get_global_id(0), get_global_id(1));

    uint2 states = (uint2)( (coord.x + coord.y * get_global_size(1)) * seed);
    float4 r = rand(&states);

    float4 pixel;
    if ((r.x + r.y + r.z)/3 < f)
        pixel = r;
    else
        pixel = read_imagef(srcImage, imageSampler, coord);

    write_imagef(outImage, coord, pixel);
}
