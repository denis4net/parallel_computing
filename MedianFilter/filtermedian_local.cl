#define lwidth 1920
#define lheight 1

#define getPixel(a, x, y, width, height) ((x >= 0 && x < width && y >= 0 && y < height) ? a[x+width*y] : 0)
#define setPixel(a, val, x, y, width, height) a[x+1][y+1] = val

kernel void medianFilter(const global ulong* pSrc, global ulong* pDst)
{
    //global related parameters
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    if (y == 0 || y == get_global_size(1)) //if woring item is processing first or last pixel string, quit from program
    return;

    const int width = get_global_size(0);
    const int height = get_global_size(1);
    const int size = width * height;
    //group/local related parameters
    const int lx = get_local_id(0);
    const int ly = get_local_id(1);

    const int current_pixel_ind = x*get_local_size(0)+get_local_id(0) + y*get_local_size(1);

    //allocate working group local memory
    __local ulong lRGBA[lwidth][lheight];
    
    lRGBA[lx][0] = pSrc[x + (y-1)*width];
    lRGBA[lx][1] = pSrc[x + (y)*width];
    lRGBA[lx][2] = pSrc[x + (y+1)*width];

    barrier(CLK_LOCAL_MEM_FENCE);	

    if( lx == 0 || lx == get_local_size(0) - 1 ) //if working item is processing first or last pixel in string, quit from program
    return;


        ulong uiResult = 0;
        ulong uiMask = 0xFFFFFF;

	for(int ch = 0; ch < 3; ch++)
        {
            //extract next color channel
            ulong r0,r1,r2,r3,r4,r5,r6,r7,r8;
            r0 = lRGBA[lx][ly] & uiMask;
            r1 = lRGBA[lx+1][ly] & uiMask;
            r2 = lRGBA[lx+1][ly+1] & uiMask;
            r3 = lRGBA[lx][ly+1] & uiMask;
            r4 = lRGBA[lx][ly-1] & uiMask;
            r5 = lRGBA[lx-1][ly-1]& uiMask;
            r6 = lRGBA[lx-1][ly] & uiMask;
            r7 = lRGBA[lx-1][ly+1] & uiMask;
            r8 = lRGBA[lx+1][ly-1] & uiMask;

            //perform partial bitonic sort to find current channel median
            ulong uiMin = min(r0, r1);
            ulong uiMax = max(r0, r1);
            r0 = uiMin;
            r1 = uiMax;

            uiMin = min(r3, r2);
            uiMax = max(r3, r2);
            r3 = uiMin;
            r2 = uiMax;

            uiMin = min(r2, r0);
            uiMax = max(r2, r0);
            r2 = uiMin;
            r0 = uiMax;

            uiMin = min(r3, r1);
            uiMax = max(r3, r1);
            r3 = uiMin;
            r1 = uiMax;

            uiMin = min(r1, r0);
            uiMax = max(r1, r0);
            r1 = uiMin;
            r0 = uiMax;

            uiMin = min(r3, r2);
            uiMax = max(r3, r2);
            r3 = uiMin;
            r2 = uiMax;

            uiMin = min(r5, r4);
            uiMax = max(r5, r4);
            r5 = uiMin;
            r4 = uiMax;

            uiMin = min(r7, r8);
            uiMax = max(r7, r8);
            r7 = uiMin;
            r8 = uiMax;

            uiMin = min(r6, r8);
            uiMax = max(r6, r8);
            r6 = uiMin;
            r8 = uiMax;

            uiMin = min(r6, r7);
            uiMax = max(r6, r7);
            r6 = uiMin;
            r7 = uiMax;

            uiMin = min(r4, r8);
            uiMax = max(r4, r8);
            r4 = uiMin;
            r8 = uiMax;

            uiMin = min(r4, r6);
            uiMax = max(r4, r6);
            r4 = uiMin;
            r6 = uiMax;

            uiMin = min(r5, r7);
            uiMax = max(r5, r7);
            r5 = uiMin;
            r7 = uiMax;

            uiMin = min(r4, r5);
            uiMax = max(r4, r5);
            r4 = uiMin;
            r5 = uiMax;

            uiMin = min(r6, r7);
            uiMax = max(r6, r7);
            r6 = uiMin;
            r7 = uiMax;

            uiMin = min(r0, r8);
            uiMax = max(r0, r8);
            r0 = uiMin;
            r8 = uiMax;

            r4 = max(r0, r4);
            r5 = max(r1, r5);

            r6 = max(r2, r6);
            r7 = max(r3, r7);

            r4 = min(r4, r6);
            r5 = min(r5, r7);

            //store found median into result
            uiResult |= min(r4, r5);

            //update channel mask
            uiMask <<= 8;
        }

    pDst[x+y*width] = lRGBA[lx][ly];
}
