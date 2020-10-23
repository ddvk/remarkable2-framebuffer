#include <QCoreApplication>
#include <stdio.h>
#include <linux/fb.h>
#include <QImage>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SCWIDTH 1404
#define SCHEIGHT 1872
static uint8_t arr[0x16580];
static uint8_t buffer[SCWIDTH*SCHEIGHT];

void check_error(int err, char *msg){

}
void load_waveform(char *fileName){
    FILE *f = fopen(fileName, "rb");
    if (f == NULL) {
        puts("cant open wavform");
        exit(1);
    }

    // more loading logic
}

int main(int argc, char *argv[])
{
    fb_var_screeninfo vinfo;
    fb_fix_screeninfo finfo;

    auto fd = open("/dev/fb0", O_RDWR);
    auto result = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
    if (result)
    result = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
    size_t size = vinfo.yres * finfo.line_length;
    printf("%d", size);
    uint8_t *fbp = (uint8_t*) mmap(0, size, PROT_READ | PROT_WRITE,  MAP_SHARED, fd,0);
    if (fbp == NULL){
        puts("cant mmap");
        exit(1);
    }

    QCoreApplication a(argc, argv);

    //todo check the format
    QImage img(SCWIDTH, SCHEIGHT, QImage::Format_RGB32);
    img.fill(Qt::white);
    auto bits = img.bits();
    memcpy(fbp, bits, 1000);


    printf("exiting...\n");



    return a.exec();
}
