/* *
 * Copyright (c) 2014, James S. Plank and Kevin Greenan
 * All rights reserved.
 *
 * Jerasure - A C/C++ Library for a Variety of Reed-Solomon and RAID-6 Erasure
 * Coding Techniques
 *
 * Revision 2.0: Galois Field backend now links to GF-Complete
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 *  - Neither the name of the University of Tennessee nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Jerasure's authors:

   Revision 2.x - 2014: James S. Plank and Kevin M. Greenan
   Revision 1.2 - 2008: James S. Plank, Scott Simmerman and Catherine D. Schuman.
   Revision 1.0 - 2007: James S. Plank
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gf_complete.h>
#include "galois.h"
#include "jerasure.h"
#include "reed_sol.h"

#define talloc(type, num) (type *) malloc(sizeof(type)*(num))

inline int *reed_sol_r6_coding_matrix_setup(int k, int w, int *matrix) {
    int i, tmp;

    for (i = 0; i < k; i++) matrix[i] = 1;
    matrix[k] = 1;
    tmp = 1;
    for (i = 1; i < k; i++) {
        tmp = galois_single_multiply(tmp, 2, w);
        matrix[k+i] = tmp;
    }
    return matrix;
}

int *reed_sol_r6_coding_matrix(int k, int w)
{
    int *matrix;

    if (w != 8 && w != 16 && w != 32) return NULL;

    matrix = talloc(int, 2*k);
    if (matrix == NULL) return NULL;

    return reed_sol_r6_coding_matrix_setup(k, w, matrix);
}

int *reed_sol_r6_coding_matrix_noalloc(int k, int w, int *matrix)
{
    if (w != 8 && w != 16 && w != 32) return NULL;

    return reed_sol_r6_coding_matrix_setup(k, w, matrix);
}

inline int *reed_sol_vandermonde_coding_matrix_setup(int k, int m, int w, int *dist, int *vdm) 
{
    int i, j;
    i = k*k;
    for (j = 0; j < m*k; j++) {
        dist[j] = vdm[i];
        i++;
    }
    return dist;
}

int *reed_sol_vandermonde_coding_matrix(int k, int m, int w)
{
    int *vdm, *dist;

    vdm = reed_sol_big_vandermonde_distribution_matrix(k+m, k, w);
    if (vdm == NULL) return NULL;
    dist = talloc(int, m*k);
    if (dist == NULL) {
        free(vdm);
        return NULL;
    }
    reed_sol_vandermonde_coding_matrix_setup(k, m, w, dist, vdm);
    free(vdm);
    return dist;
}

int *reed_sol_vandermonde_coding_matrix_noalloc(int k, int m, int w, int* matrix) 
{
    int vdm[(k+m) * k];
    int *ret = reed_sol_big_vandermonde_distribution_matrix_noalloc(k+m, k, w, vdm);
    if (ret == NULL) return NULL;
    reed_sol_vandermonde_coding_matrix_setup(k, m, w, matrix, vdm);
    return matrix;
}

static int prim08 = -1;
static gf_t GF08;

void reed_sol_galois_w08_region_multby_2(char *region, int nbytes)
{
    if (prim08 == -1) {
        prim08 = galois_single_multiply((1 << 7), 2, 8);
        if (!gf_init_hard(&GF08, 8, GF_MULT_BYTWO_b, GF_REGION_DEFAULT, GF_DIVIDE_DEFAULT,
                    prim08, 0, 0, NULL, NULL)) {
            fprintf(stderr, "Error: Can't initialize the GF for reed_sol_galois_w08_region_multby_2\n");
            assert(0);
        }
    }
    GF08.multiply_region.w32(&GF08, region, region, 2, nbytes, 0);
}

static int prim16 = -1;
static gf_t GF16;

void reed_sol_galois_w16_region_multby_2(char *region, int nbytes)
{
    if (prim16 == -1) {
        prim16 = galois_single_multiply((1 << 15), 2, 16);
        if (!gf_init_hard(&GF16, 16, GF_MULT_BYTWO_b, GF_REGION_DEFAULT, GF_DIVIDE_DEFAULT,
                    prim16, 0, 0, NULL, NULL)) {
            fprintf(stderr, "Error: Can't initialize the GF for reed_sol_galois_w16_region_multby_2\n");
            assert(0);
        }
    }
    GF16.multiply_region.w32(&GF16, region, region, 2, nbytes, 0);
}

static int prim32 = -1;
static gf_t GF32;

void reed_sol_galois_w32_region_multby_2(char *region, int nbytes)
{
    if (prim32 == -1) {
        prim32 = galois_single_multiply((1 << 31), 2, 32);
        if (!gf_init_hard(&GF32, 32, GF_MULT_BYTWO_b, GF_REGION_DEFAULT, GF_DIVIDE_DEFAULT,
                    prim32, 0, 0, NULL, NULL)) {
            fprintf(stderr, "Error: Can't initialize the GF for reed_sol_galois_w32_region_multby_2\n");
            assert(0);
        }
    }
    GF32.multiply_region.w32(&GF32, region, region, 2, nbytes, 0);
}

int reed_sol_r6_encode(int k, int w, char **data_ptrs, char **coding_ptrs, int size)
{
    int i;

    /* First, put the XOR into coding region 0 */

    memcpy(coding_ptrs[0], data_ptrs[0], size);

    for (i = 1; i < k; i++) galois_region_xor(data_ptrs[i], coding_ptrs[0], size);

    /* Next, put the sum of (2^j)*Dj into coding region 1 */

    memcpy(coding_ptrs[1], data_ptrs[k-1], size);

    for (i = k-2; i >= 0; i--) {
        switch (w) {
            case 8:  reed_sol_galois_w08_region_multby_2(coding_ptrs[1], size); break;
            case 16: reed_sol_galois_w16_region_multby_2(coding_ptrs[1], size); break;
            case 32: reed_sol_galois_w32_region_multby_2(coding_ptrs[1], size); break;
            default: return 0;
        }

        galois_region_xor(data_ptrs[i], coding_ptrs[1], size);
    }
    return 1;
}

inline int *reed_sol_extended_vandermonde_matrix_setup(int rows, int cols, int w, int *vdm) 
{
    int i, j, k;

    if (w < 30 && (1 << w) < rows) return NULL;
    if (w < 30 && (1 << w) < cols) return NULL;

    vdm[0] = 1;
    for (j = 1; j < cols; j++) vdm[j] = 0;
    if (rows == 1) return vdm;

    i=(rows-1)*cols;
    for (j = 0; j < cols-1; j++) vdm[i+j] = 0;
    vdm[i+j] = 1;
    if (rows == 2) return vdm;

    for (i = 1; i < rows-1; i++) {
        k = 1;
        for (j = 0; j < cols; j++) {
            vdm[i*cols+j] = k;
            k = galois_single_multiply(k, i, w);
        }
    }
    return vdm;
}

int *reed_sol_extended_vandermonde_matrix(int rows, int cols, int w)
{
    int *vdm;

    vdm = talloc(int, rows*cols);
    if (vdm == NULL) { return NULL; }
    int *ret = reed_sol_extended_vandermonde_matrix_setup(rows, cols, w, vdm);
    if (ret == NULL) { 
        free(vdm);
        return NULL;
    }
    return ret;
}

int *reed_sol_extended_vandermonde_matrix_noalloc(int rows, int cols, int w, int* vdm)
{
    return reed_sol_extended_vandermonde_matrix_setup(rows, cols, w, vdm);
}

inline int *reed_sol_big_vandermonde_distribution_matrix_setup(int rows, int cols, int w, int *dist)
{ 
    int i, j, k;
    int sindex, srindex, siindex, tmp;

    if (cols >= rows) return NULL;
    if (dist == NULL) return NULL;

    sindex = 0;
    for (i = 1; i < cols; i++) {
        sindex += cols;

        /* Find an appropriate row -- where i,i != 0 */
        srindex = sindex+i;
        for (j = i; j < rows && dist[srindex] == 0; j++) srindex += cols;
        if (j >= rows) {   /* This should never happen if rows/w are correct */
            fprintf(stderr, "reed_sol_big_vandermonde_distribution_matrix(%d,%d,%d) - couldn't make matrix\n", 
                    rows, cols, w);
            assert(0);
        }

        /* If necessary, swap rows */
        if (j != i) {
            srindex -= i;
            for (k = 0; k < cols; k++) {
                tmp = dist[srindex+k];
                dist[srindex+k] = dist[sindex+k];
                dist[sindex+k] = tmp;
            }
        }

        /* If Element i,i is not equal to 1, multiply the column by 1/i */

        if (dist[sindex+i] != 1) {
            tmp = galois_single_divide(1, dist[sindex+i], w);
            srindex = i;
            for (j = 0; j < rows; j++) {
                dist[srindex] = galois_single_multiply(tmp, dist[srindex], w);
                srindex += cols;
            }
        }

        /* Now, for each element in row i that is not in column 1, you need
           to make it zero.  Suppose that this is column j, and the element
           at i,j = e.  Then you want to replace all of column j with 
           (col-j + col-i*e).   Note, that in row i, col-i = 1 and col-j = e.
           So (e + 1e) = 0, which is indeed what we want. */

        for (j = 0; j < cols; j++) {
            tmp = dist[sindex+j];
            if (j != i && tmp != 0) {
                srindex = j;
                siindex = i;
                for (k = 0; k < rows; k++) {
                    dist[srindex] = dist[srindex] ^ galois_single_multiply(tmp, dist[siindex], w);
                    srindex += cols;
                    siindex += cols;
                }
            }
        }
    }
    /* We desire to have row k be all ones.  To do that, multiply
       the entire column j by 1/dist[k,j].  Then row j by 1/dist[j,j]. */

    sindex = cols*cols;
    for (j = 0; j < cols; j++) {
        tmp = dist[sindex];
        if (tmp != 1) { 
            tmp = galois_single_divide(1, tmp, w);
            srindex = sindex;
            for (i = cols; i < rows; i++) {
                dist[srindex] = galois_single_multiply(tmp, dist[srindex], w);
                srindex += cols;
            }
        }
        sindex++;
    }

    /* Finally, we'd like the first column of each row to be all ones.  To
       do that, we multiply the row by the inverse of the first element. */

    sindex = cols*(cols+1);
    for (i = cols+1; i < rows; i++) {
        tmp = dist[sindex];
        if (tmp != 1) { 
            tmp = galois_single_divide(1, tmp, w);
            for (j = 0; j < cols; j++) dist[sindex+j] = galois_single_multiply(dist[sindex+j], tmp, w);
        }
        sindex += cols;
    }

    return dist;
}

int *reed_sol_big_vandermonde_distribution_matrix_noalloc(int rows, int cols, int w, int* dist)
{
    int *ret = reed_sol_extended_vandermonde_matrix_noalloc(rows, cols, w, dist);
    if (ret == NULL) return NULL;
    return reed_sol_big_vandermonde_distribution_matrix_setup(rows, cols, w, dist);
}

int *reed_sol_big_vandermonde_distribution_matrix(int rows, int cols, int w)
{
    int *dist;
    dist = reed_sol_extended_vandermonde_matrix(rows, cols, w);
    if (dist == NULL) return NULL;
    return reed_sol_big_vandermonde_distribution_matrix_setup(rows, cols, w, dist);
}

