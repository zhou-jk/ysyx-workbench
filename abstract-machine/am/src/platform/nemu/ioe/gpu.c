#include <am.h>
#include <nemu.h>
#include <string.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t screen = inl(VGACTL_ADDR);
  uint32_t screen_width = screen >> 16;
  uint32_t screen_height = screen & 0xffff;
  uint32_t screen_size = screen_width * screen_height * sizeof(uint32_t);

  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = screen_width, .height = screen_height,
    .vmemsz = screen_size
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  if (!ctl->sync && (w == 0 || h == 0))
    return;
  uint32_t *pixels = ctl->pixels;
  uint32_t *fb = (uint32_t *)FB_ADDR;
  uint32_t screen_width = inl(VGACTL_ADDR) >> 16;

  uint32_t bytes = w * sizeof(uint32_t);
  uint32_t *dest = fb + x + y * screen_width;
  uint32_t *src = pixels;
  for (uint32_t i = 0; i < h; i++) {
    memcpy(dest, src, bytes);
    dest += screen_width;
    src += w;
  }
  //   for (int j = 0; j < w; j++) {
  // for (int i = 0; i < h; i++) {
  //     fb[(j + x) + (i + y) * screen_width] = pixels[j + i * w];
  //   }
  // }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = (bool) inl(SYNC_ADDR);
  return;
}
