#ifndef FBMANAGER_H
#define FBMANAGER_H
#include <pthread.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

class FBManager
{
public:
    FBManager();
    void SendUpdate();

private:
    void Init();
    static const int height = 1872;
    static const int width = 1404;

    const int lineWidth = 1408;
    const int bufferWidth = 1040;
    char initMask[height*width];
};

#endif // FBMANAGER_H
