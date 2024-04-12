#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

#include "stb_image.h"
#include "stb_image_write.h"

void check_min_max(const char* name, float* arr, int count, float* min_res, float* max_res) {
  float min = FLT_MAX;
  float max = 0.0;
  for (int i = 0; i < count; ++i) {
    if (arr[i] < min) min = arr[i];
    if (arr[i] > max) max = arr[i];
  }

  printf("[SEAM-CARVER] %s: min=%f, max=%f\n", name, min, max);
  
  if (min_res != NULL) *min_res = min;
  if (max_res != NULL) *max_res = max;
}

float rgb_to_luminance(uint32_t pixel) {
  float r = ((pixel >> (8*0)) & 0xFF) / 255.0;
  float g = ((pixel >> (8*1)) & 0xFF) / 255.0;
  float b = ((pixel >> (8*2)) & 0xFF) / 255.0;

  return 0.299*r + 0.587*g + 0.114*b;
}

float* calculate_lum_image(uint32_t* pixels, int width, int height) {
  float* lum = malloc(sizeof(double) * width * height);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      lum[y*width + x] = rgb_to_luminance(pixels[y*width + x]);
    }
  }

  return lum;
}

float* calculate_grad_image(float* lum, int width, int height) {
  static double kernelx[3][3] = {
    { 1.0, 0.0, -1.0 },
    { 2.0, 0.0, -2.0 },
    { 1.0, 0.0, -1.0 },
  };

  static double kernely[3][3] = {
    { 1.0, 2.0, 1.0 },
    { 0.0, 0.0, 0.0 },
    { -1.0, -2.0, -1.0 },
  };

  float* grad = malloc(sizeof(float) * width * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float gx = 0.0;
      float gy = 0.0;
      for (int dy = -1; dy <= 1; ++dy) {
	for (int dx = -1; dx <= 1; ++dx) {
	  if (y+dy >= 0 && y+dy < height && x+dx >= 0 && x+dx < width) {
	    gx += lum[(y + dy)*width + (x + dx)] * kernelx[dy+1][dx+1];
	    gy += lum[(y + dy)*width + (x + dx)] * kernely[dy+1][dx+1];
	  }
	}
      }
      grad[y*width + x] = sqrt(gx*gx + gy*gy);
    }
  }

  return grad;
}

int dump_image(const char* out_path, float* values, uint32_t* pixels, int width, int height) {
  for (int i = 0; i < width * height; ++i) {
    uint32_t value = 255 * values[i];
    pixels[i] = 0xFF000000|(value<<(8*2))|(value<<(8*1))|(value<<(8*0));
  }

  if(!stbi_write_png(out_path, width, height, 4, pixels, width*sizeof(*pixels))) {
    printf("[SEAM-CARVER] Error: failed to save luminance image\n");
    return 1;
  }

  printf("[SEAM-CARVER] Succesfully outputted to %s\n", out_path);
  printf("\n");
  return 0;
}

int main() {
  const char* file_path = "images/Broadway_tower_edit.jpg";

  int width, height;
  uint32_t* pixels = (uint32_t*)stbi_load(file_path, &width, &height, NULL, 4);

  if (pixels == NULL) {
    printf("[SEAM-CARVER] Error: couldn't load image");
    return 1;
  }
  
  printf("[SEAM-CARVER] Loaded image succesfully\n");
  printf("[SEAM-CARVER] width=%d, height=%d\n", width, height);
  printf("\n");
  
  float* lum = calculate_lum_image(pixels, width, height);
  check_min_max("Luminance", lum, width*height, NULL, NULL);
  if (dump_image("lum.png", lum, pixels, width, height)) return 1;
  
  float* grad = calculate_grad_image(lum, width, height);
  float max_grad;
  check_min_max("Gradient", grad, width*height, NULL, &max_grad);
  for (int i = 0; i < width*height; ++i) grad[i] /= max_grad;
  if (dump_image("grad.png", grad, pixels, width, height)) return 1;

  return 0;
}
