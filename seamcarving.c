#include <stdio.h>
#include <math.h>
#include <string.h>
#include "seamcarving.h"
#define min(a, b) ((a) < (b) ? (a) : (b))

void calc_energy(struct rgb_img *im, struct rgb_img **grad){
    // Create image to store the dual-gradient energy function values
    create_img(grad, im->height, im->width);
    // Allocates memory for the image

    // Error handling for malloc
    if (*grad == NULL){
        exit(1);
    }

    for (int i = 0; i < im->height; i++){
        for(int j = 0; j < im->width; j++){
            // Horizontal pixels
            int r_left = get_pixel(im, i, (j - 1 + im->width) % im->width, 0);
            int g_left = get_pixel(im, i, (j - 1 + im->width) % im->width, 1);
            int b_left = get_pixel(im, i, (j - 1 + im->width) % im->width, 2);

            int r_right = get_pixel(im, i, (j + 1) % im->width, 0);
            int g_right = get_pixel(im, i, (j + 1) % im->width, 1);
            int b_right = get_pixel(im, i, (j + 1) % im->width, 2);

            // Difference across x-axis
            int diff_rx = r_right - r_left;
            int diff_gx = g_right - g_left;
            int diff_bx = b_right - b_left;

            // Vertical pixels
            int r_up = get_pixel(im, (i - 1 + im->height) % im->height, j, 0);
            int g_up = get_pixel(im, (i - 1 + im->height) % im->height, j, 1);
            int b_up = get_pixel(im, (i - 1 + im->height) % im->height, j, 2);

            int r_down = get_pixel(im, (i + 1) % im->height, j, 0);
            int g_down = get_pixel(im, (i + 1) % im->height, j, 1);
            int b_down = get_pixel(im, (i + 1) % im->height, j, 2);

            // Difference across y-axis
            int diff_ry = r_up - r_down;
            int diff_gy = g_up - g_down;
            int diff_by = b_up - b_down;

            // Energy calculation 
            int energy_x = pow(diff_rx, 2) + pow(diff_gx, 2) + pow(diff_bx, 2);
            int energy_y = pow(diff_ry, 2) + pow(diff_gy, 2) + pow(diff_by, 2);
            
            int tot_energy = sqrt(energy_x + energy_y);

            uint8_t final_energy = (uint8_t)(tot_energy / 10);

            // Store the energy in another image
            set_pixel(*grad, i, j, final_energy, final_energy, final_energy);

            // Grad array should be of size: 3*im->height*im->width
        }
    }

}


void dynamic_seam(struct rgb_img *grad, double **best_arr) {
    // Allocate memory for the dynamic array
    *best_arr = (double *)malloc(sizeof(double) * grad->height * grad->width);

    // Error handling for malloc
    if (*best_arr == NULL){
        exit(1);
    }

    // Initialize best_arr first row with values from the gradient image
    for (int k = 0; k < grad->width; k++) {
        (*best_arr)[k] = get_pixel(grad, 0, k, 0); 
    }

    for (int i = 1; i < grad->height; i++) {
        for (int j = 0; j < grad->width; j++) {
            double min_cost = (*best_arr)[(i - 1) * grad->width + j]; 
            if (j > 0) {
                min_cost = min(min_cost, (*best_arr)[(i - 1) * grad->width + j - 1]); 
            }
            if (j < grad->width - 1) {
                min_cost = min(min_cost, (*best_arr)[(i - 1) * grad->width + j + 1]); 
            }

            // Add the current pixel's energy
            min_cost += get_pixel(grad, i, j, 0);

            (*best_arr)[i * grad->width + j] = min_cost; 
        
        }
    }
}


void recover_path(double *best, int height, int width, int **path){
    // Allocate memory for array
    (*path) = (int *)malloc(sizeof(int) * height);

    // Error handling for malloc
    if (*path == NULL){
        exit(1);
    }

    for (int i = height - 1; i >= 0; i--){
        double cur_best = best[i * width];
        int cur_index = 0;
        for (int j = 1; j < width; j++){
            if (best[i * width + j] < cur_best){
                cur_best = best[i * width + j];
                cur_index = j;
            }
        }
        (*path)[i] = cur_index;
    }
}

void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path){
    // The function creates the destination image, and writes to it the source image, with the seam removed.

    // Need to allocate for dest and create destination image
    create_img(dest, src->height, src->width - 1);

    // Error handling for malloc
    if (*dest == NULL){
        exit(1);
    }

    // Use memmove to remove seam by replacing pixels
    // on the path with pixels in front
    for (int i = 0; i < src->height; i++) {

        if (path[i] > 0) {
            memmove(&((*dest)->raster[3 * (i * ((*dest)->width))]), 
                    &(src->raster[3 * (i * (src->width))]),          
                    3 * (path[i]) * sizeof(uint8_t));              
        }

        if (path[i] < src->width - 1) {
            memmove(&((*dest)->raster[3 * (i * ((*dest)->width) + path[i])]), 
                    &(src->raster[3 * (i * (src->width) + path[i] + 1)]),    
                    3 * (src->width - path[i] - 1) * sizeof(uint8_t));     
        }
    }
}

