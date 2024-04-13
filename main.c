#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define AT(img, x, y) (img).pixels((y)*(img).width + (x)))

typedef struct _Magnitude {
  int width;
  int height;
  float* pixels;
} Magnitude;

typedef struct _Image {
  int width;
  int height;
  uint32_t* pixels;
} Image;

Image init_image(int width, int height, uint32_t* pixels) {
  return (Image){
    .width = width,
    .height = height,
    .pixels = pixels,
  };
}

Magnitude init_magnitude(int width, int height, float* pixels) {
  return (Magnitude){
    .width = width,
    .height = height,
    .pixels = pixels,
  };
}

void check_min_max(const char* name, Magnitude mag, float* min_res, float* max_res) {
  float min = FLT_MAX;
  float max = 0.0;
  for (int i = 0; i < mag.width * mag.height; ++i) {
    if (mag.pixels[i] < min) min = mag.pixels[i];
    if (mag.pixels[i] > max) max = mag.pixels[i];
  }

  printf("[SEAM-CARVER] %s: min=%f, max=%f\n", name, min, max);
  
  if (min_res != NULL) *min_res = min;
  if (max_res != NULL) *max_res = max;
}

int dump_image(const char* out_path, Image image) {
  if(!stbi_write_png(out_path, image.width, image.height, 4, image.pixels, image.width*sizeof(*(image.pixels)))) {
    printf("[SEAM-CARVER] Error: failed to save luminance image\n");
    return 1;
  }

  printf("[SEAM-CARVER] Succesfully outputted to %s\n", out_path);
  printf("[SEAM-CARVER] Output dimensions: width=%d, height=%d\n", image.width, image.height);
  printf("\n");

  return 0;
}

int dump_mag(const char* out_path, Magnitude mag, Image image, float max) {
  for (int i = 0; i < mag.width * mag.height; ++i) {
    uint32_t value = 255 * (mag.pixels[i] / max);
    image.pixels[i] = 0xFF000000|(value<<(8*2))|(value<<(8*1))|(value<<(8*0));
  }

  return dump_image(out_path, image);
}

float rgb_to_luminance(uint32_t pixel) {
  float r = ((pixel >> (8*0)) & 0xFF) / 255.0;
  float g = ((pixel >> (8*1)) & 0xFF) / 255.0;
  float b = ((pixel >> (8*2)) & 0xFF) / 255.0;

  return 0.299*r + 0.587*g + 0.114*b;
}

Magnitude calculate_lum_image(Image image) {
  float* lum = malloc(sizeof(double) * image.width * image.height);
  for (int i = 0; i < image.width * image.height; ++i) {
    lum[i] = rgb_to_luminance(image.pixels[i]);
  }

  return init_magnitude(image.width, image.height, lum);
}

Magnitude calculate_grad_image(Magnitude lum) {
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

  float* grad = malloc(sizeof(float) * lum.width * lum.height);

  for (int y = 0; y < lum.height; ++y) {
    for (int x = 0; x < lum.width; ++x) {
      float gx = 0.0;
      float gy = 0.0;
      for (int dy = -1; dy <= 1; ++dy) {
	for (int dx = -1; dx <= 1; ++dx) {
	  if (y+dy >= 0 && y+dy < lum.height && x+dx >= 0 && x+dx < lum.width) {
	    gx += lum.pixels[(y + dy)*lum.width + (x + dx)] * kernelx[dy+1][dx+1];
	    gy += lum.pixels[(y + dy)*lum.width + (x + dx)] * kernely[dy+1][dx+1];
	  }
	}
      }
      grad[y*lum.width + x] = sqrt(gx*gx + gy*gy);
    }
  }

  return init_magnitude(lum.width, lum.height, grad);
}

Magnitude calculate_dp_image(Magnitude grad) {
  float* dp = malloc(sizeof(double) * grad.width * grad.height);
  for (int x = 0; x < grad.width; ++x) {
    dp[x] = grad.pixels[x];
  }

  for (int y = 1; y < grad.height; ++y) {
    for (int x = 0; x < grad.width; ++x) {
      float min = FLT_MAX;
      for (int dx = -1; dx <= 1; ++dx) {
	if (dx+x >= 0 && dx+x < grad.width && dp[(y-1)*grad.width + (x+dx)] < min) {
	  min = dp[(y-1)*grad.width + (x+dx)];
	}
      }
      dp[y*grad.width + x] = min + grad.pixels[y*grad.width + x];
    }
  }

  return init_magnitude(grad.width, grad.height, dp);
}





int main() {
  srand(time(NULL));
  
  //  const char* file_path = "images/Broadway_tower_edit.jpg";
  const char* file_path = "images/Lena_512.png";
  
  int width, height;
  uint32_t* pixels = (uint32_t*)stbi_load(file_path, &width, &height, NULL, 4);
  if (pixels == NULL) {
    printf("[SEAM-CARVER] Failed to load %s", file_path);
  };
  Image image = init_image(width, height, pixels);
  
  uint32_t* original_pixels = malloc(sizeof(uint32_t) * width * height);
  memcpy(original_pixels, pixels, sizeof(uint32_t) * width * height);
  Image original_image = init_image(width, height, original_pixels);
  
  if (pixels == NULL) {
    printf("[SEAM-CARVER] Error: couldn't load image");
    return 1;
  }
  
  printf("[SEAM-CARVER] Loaded image succesfully\n");
  printf("[SEAM-CARVER] width=%d, height=%d\n", width, height);
  printf("\n");

  for (int i = 0; i < 350; ++i) { 
    printf("Iteration: %d\n", i);
    Magnitude lum = calculate_lum_image(original_image);
    //    check_min_max("Luminance", lum, NULL, NULL);
    //    if (dump_mag("lum.png", lum, image, 1.0)) return 1;
  
    Magnitude grad = calculate_grad_image(lum);
    //    float max_grad;
    //    check_min_max("Gradient", grad, NULL, &max_grad);
    //    if (dump_mag("grad.png", grad, image, max_grad)) return 1;

    Magnitude dp = calculate_dp_image(grad);
    //    float max_dp;
    //    check_min_max("Dynamic Programming", dp, NULL, &max_dp);
    //    if (dump_mag("dp.png", dp, image, max_dp)) return 1;

  
    int seam = rand()%dp.width;
    for (int y = dp.height-1; y >= 0; --y) {
      memmove(&(original_image.pixels[y*dp.width + seam]), &(original_image.pixels[y*dp.width + seam+1]), sizeof(uint32_t) * ((dp.width * dp.height) - (y*dp.width + seam)));
      //    original_image.pixels[y*width + seam] = 0xFF0000FF;
      seam = ((seam-1) >= 0 && dp.pixels[y*dp.width + seam-1] < dp.pixels[y*dp.width + seam]) ? seam-1 : seam;
      seam = ((seam+1) < dp.width && dp.pixels[y*dp.width + seam+1] < dp.pixels[y*dp.width + seam]) ? seam+1 : seam;
    }

    original_image.width -= 1;
  }
  dump_image("output.png", original_image);
  dump_image("dp_seam.png", image);
  
  return 0;
}
