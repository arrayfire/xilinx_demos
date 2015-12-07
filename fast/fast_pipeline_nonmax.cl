/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#define FAST_THREADS_X 1
#define FAST_THREADS_Y 1
#define FAST_THREADS_NONMAX_X 32
#define FAST_THREADS_NONMAX_Y 8

#define ARC_LENGTH 9
#define NONMAX 1
#define EDGE 3
#define LOCAL_LINES 17
#define NONMAX_LINES 16
#define WIDTH 640

#define MAX_VAL(A,B) (A<B) ? (B) : (A)

inline int idx_y(const int i)
{
    int j = i - 4;
    int k = min(j, 8 - j);
    return clamp(k, -3, 3);
}

inline int idx_x(const int i)
{
    return idx_y((i + 4) & 15);
}

inline int idx(const int x, const int y)
{
    return y * WIDTH + x;
}

// test_greater()
// Tests if a pixel x > p + thr
inline int test_greater(const int x, const int p, const int thr)
{
    return (x >= p + thr);
}

// test_smaller()
// Tests if a pixel x < p - thr
inline int test_smaller(const int x, const int p, const int thr)
{
    return (x <= p - thr);
}

// test_pixel()
// Returns -1 when x < p - thr
// Returns  0 when x >= p - thr && x <= p + thr
// Returns  1 when x > p + thr
inline int test_pixel(__local int* local_image, const int p, const int thr, const int x, const int y)
{
    return -test_smaller(local_image[idx(x,y)], p, thr) | test_greater(local_image[idx(x,y)], p, thr);
}

__kernel __attribute__ ((reqd_work_group_size(FAST_THREADS_X, FAST_THREADS_Y, 1)))
void locate_features(
    __global int *in,
    const int d0,
    const int d1,
    __global int *score,
    const int thr,
    const unsigned edge)
{
#ifdef __xilinx__
    __attribute__((xcl_pipeline_workitems)) {
#endif
    __local int local_image[((LOCAL_LINES + EDGE*2) * WIDTH)];
    __local int local_score[((NONMAX_LINES + EDGE*2) * WIDTH)];

    for (int i = EDGE; i < d1 - EDGE; i += LOCAL_LINES) {
        size_t gidx = ((i-3) * WIDTH);
        size_t copy_size = ((i-3 + LOCAL_LINES+EDGE*2) > d1)
                           ? (d1-i+EDGE)*WIDTH
                           : (LOCAL_LINES+EDGE*2)*WIDTH;
        event_t ev;
        ev = async_work_group_copy(local_image, in + gidx, copy_size, 0);
        wait_group_events(1, &ev);

        for (int j = 0; j < ((NONMAX_LINES + EDGE*2) * WIDTH); j++) {
            local_score[j] = 0;
        }

        for (int ii = 0; ii < LOCAL_LINES; ii++) {
            for (int j = EDGE; j < d0 - EDGE; j++) {
                int x = j;
                int y = i + ii;
                int lx = x;
                int ly = ii + 3;

                int p = local_image[idx(lx, ly)];

                // Start by testing opposite pixels of the circle that will result in
                // a non-kepoint
                int d = test_pixel(local_image, p, thr, lx-3, ly+0) | test_pixel(local_image, p, thr, lx+3, ly+0);
                if (d == 0)
                    continue;

                d &= test_pixel(local_image, p, thr, lx-2, ly+2) | test_pixel(local_image, p, thr, lx+2, ly-2);
                d &= test_pixel(local_image, p, thr, lx+0, ly+3) | test_pixel(local_image, p, thr, lx+0, ly-3);
                d &= test_pixel(local_image, p, thr, lx+2, ly+2) | test_pixel(local_image, p, thr, lx-2, ly-2);
                if (d == 0)
                    continue;

                d &= test_pixel(local_image, p, thr, lx-3, ly+1) | test_pixel(local_image, p, thr, lx+3, ly-1);
                d &= test_pixel(local_image, p, thr, lx-1, ly+3) | test_pixel(local_image, p, thr, lx+1, ly-3);
                d &= test_pixel(local_image, p, thr, lx+1, ly+3) | test_pixel(local_image, p, thr, lx-1, ly-3);
                d &= test_pixel(local_image, p, thr, lx+3, ly+1) | test_pixel(local_image, p, thr, lx-3, ly-1);
                if (d == 0)
                    continue;

                int sum = 0;

                // Sum responses [-1, 0 or 1] of first ARC_LENGTH pixels
                #ifdef __xilinx__
                __attribute__((opencl_unroll_hint))
                #endif
                for (int i = 0; i < ARC_LENGTH; i++)
                    sum += test_pixel(local_image, p, thr, lx+idx_x(i), ly+idx_y(i));

                // Test maximum and mininmum responses of first segment of ARC_LENGTH
                // pixels
                int max_sum = 0, min_sum = 0;
                max_sum = max(max_sum, sum);
                min_sum = min(min_sum, sum);

                // Sum responses and test the remaining 16-ARC_LENGTH pixels of the circle
                #ifdef __xilinx__
                __attribute__((opencl_unroll_hint))
                #endif
                for (int i = ARC_LENGTH; i < 16; i++) {
                    sum -= test_pixel(local_image, p, thr, lx+idx_x(i-ARC_LENGTH), ly+idx_y(i-ARC_LENGTH));
                    sum += test_pixel(local_image, p, thr, lx+idx_x(i), ly+idx_y(i));
                    max_sum = max(max_sum, sum);
                    min_sum = min(min_sum, sum);
                }

                // To completely test all possible segments, it's necessary to test
                // segments that include the top junction of the circle
                #ifdef __xilinx__
                __attribute__((opencl_unroll_hint))
                #endif
                for (int i = 0; i < ARC_LENGTH-1; i++) {
                    sum -= test_pixel(local_image, p, thr, lx+idx_x(16-ARC_LENGTH+i), ly+idx_y(16-ARC_LENGTH+i));
                    sum += test_pixel(local_image, p, thr, lx+idx_x(i), ly+idx_y(i));
                    max_sum = max(max_sum, sum);
                    min_sum = min(min_sum, sum);
                }

                // If sum at some point was equal to (+-)ARC_LENGTH, there is a segment
                // for which all pixels are much brighter or much darker than central
                // pixel p.
                if (max_sum == ARC_LENGTH || min_sum == -ARC_LENGTH) {
                    // Compute scores for brighter and darker pixels
                    int s_bright = 0, s_dark = 0;

                    #ifdef __xilinx__
                    __attribute__((opencl_unroll_hint))
                    #endif
                    for (int i = 0; i < 16; i++) {
                        int p_x    = local_image[lx+idx(idx_x(i), ly+idx_y(i))];
                        int weight = abs((int)p_x - (int)p) - thr;
                        s_bright += test_greater(p_x, p, thr) * weight;
                        s_dark   += test_smaller(p_x, p, thr) * weight;
                    }

                    //score[x + d0 * y] = MAX_VAL(s_bright, s_dark);
                    local_score[idx(lx, ly)] = MAX_VAL(s_bright, s_dark);
                }
            }
        }

        for (int ii = 0; ii < NONMAX_LINES; ii++) {
            for (int j = EDGE; j < d0 - EDGE; j++) {
                int x = j;
                int y = i + ii;
                int lx = x;
                int ly = ii + 3;

                int v = local_score[idx(lx, ly)];

                if (v != 0) {
                    int max_v = local_score[idx(lx-1, ly-1)];
                    max_v = MAX_VAL(max_v, local_score[idx(lx-1, ly)]);
                    max_v = MAX_VAL(max_v, local_score[idx(lx-1, ly+1)]);
                    max_v = MAX_VAL(max_v, local_score[idx(lx,   ly-1)]);
                    max_v = MAX_VAL(max_v, local_score[idx(lx,   ly+1)]);
                    max_v = MAX_VAL(max_v, local_score[idx(lx+1, ly+1)]);
                    max_v = MAX_VAL(max_v, local_score[idx(lx+1, ly)]);
                    max_v = MAX_VAL(max_v, local_score[idx(lx+1, ly-1)]);
                    if (v > max_v)
                        score[idx(x, y)] = v;
                }
            }
        }
    }
#ifdef __xilinx__
    }
#endif
}
