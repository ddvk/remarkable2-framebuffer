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
static uint8_t InitMask[0x16580];
static uint8_t buffer[SCWIDTH*SCHEIGHT];

void create_mask() {
}

void check_error(int err, char *msg){
    if (err < 0) {
        printf(msg);
        exit(1);
    }
}
void load_waveform(char *fileName){
    FILE *f = fopen(fileName, "rb");
    if (f == NULL) {
        puts("cant open waveform");
        exit(1);
    }
    // more loading logic
}

int main(int argc, char *argv[])
{
    //QCoreApplication a(argc, argv);

    fb_var_screeninfo vinfo;
    fb_fix_screeninfo finfo;

    auto fd = open("/dev/fb0", O_RDWR);
    check_error(fd, "cant get vcreen");

    auto result = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
    check_error(result, "cant get vcreen");

    result = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
    check_error(result, "cant get fcreen");


    // todo: set the fb
    vinfo.yscreen = 1;
    result = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
    check_error(result, "cant set vcreen");

    size_t size = vinfo.yres * finfo.line_length;
    printf("%d", size);
    uint8_t *fbp = (uint8_t*) mmap(0, size, PROT_READ | PROT_WRITE,  MAP_SHARED, fd,0);
    if (fbp == NULL){
        puts("cant mmap");
        exit(1);
    }

    QImage img(SCWIDTH, SCHEIGHT, QImage::Format_RGB16);
    img.fill(Qt::white);
    
    //mangle and rotate the bits to a grayscale

    auto bits = img.bits();
    memcpy(fbp, bits, 1000);


    //apply the waveform to the mangled
    //copy the mangled to fb
    //ioctl(4606)


    printf("exiting...\n");

    return 0; 
}
