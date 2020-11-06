#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char** argv) {
  printf("FB\n");

  int flags = 0;
  int waveform = 2;
  if (argc > 1) {
      sscanf(argv[1],"%d", &waveform);
      printf("waveform %d\n",waveform);
  }
  if (argc > 2) {
      sscanf(argv[2],"%d", &flags);
  }
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
  unsigned len;

  char * device_path = "/dev/fb0";
  int fd = open(device_path, O_RDWR);
  if (fd < 0) {
    printf("Could not open %s\n", device_path);
    return -1;
  }
  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Could not get screen vinfo for %s\n", device_path);
    close(fd);
    return -1;
  }
  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Could not get screen finfo for %s\n", device_path);
    close(fd);
    return -1;
  }
  printf("x %d\n", vinfo.xres);
  printf("y %d\n", vinfo.yres);
  printf("z %d\n", vinfo.rotate);

  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Could not get screen vinfo for %s\n", device_path);
    close(fd);
    return NULL;
  }
  if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Could not get screen finfo for %s\n", device_path);
    close(fd);
    return NULL;
  }
  int line_length = finfo.line_length;
  len = vinfo.yres_virtual * line_length ;
  printf("linelengh %d\n", line_length);
  printf("buflength %d\n", len);
  uint8_t *buf = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);
  memset(buf,1, len);
  vinfo.yres = 0x580;
  vinfo.yres_virtual = 17*0x580;
  vinfo.yoffset = 0x5800;
  vinfo.xres = 0x104;
  vinfo.xres_virtual = 0x104;
  vinfo.pixclock = 0x7080;
  vinfo.left_margin = 1;
  vinfo.right_margin = 1;
  vinfo.upper_margin =1 ;
  vinfo.hsync_len = 1;
  vinfo.vsync_len = 1;
  vinfo.bits_per_pixel = 0x20;

  if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo)) {
    printf("Could not get screen vinfo for %s\n", device_path);
    close(fd);
    return NULL;
  }

  if (ioctl(fd, FBIOBLANK,0)){
      printf("cannot blank\n");
  }
  if (ioctl(fd, FBIOBLANK,4)){
      printf("cannot blank\n");
  }
  for(int i= 100 ; i < 20000; i*=10) {
      if (ioctl(fd, FBIOPAN_DISPLAY,&vinfo)){
          vinfo.yoffset = 0x10*0x580;
          printf("cannot pan\n");
      }
  }



  return 0;
}
