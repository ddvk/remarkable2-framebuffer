#include <QCoreApplication>
#include <QImage>
#include <iostream>
#include <fbmanager.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <inkwave.h>
#include <experimental/filesystem>

using namespace std;


// load the serial number
int serial() {
    char *device = "/dev/mmcblk2boot1";
    return 292;
}

//TODO pass the serial
uint8_t *get_waveform(int lot) {
    const char* path = "/usr/share/remarkable";
    uint8_t *buffer = nullptr;
    DIR *dir;
    char fullPath[300];
    struct dirent *ent;
    waveform_data_header wbf_header;
    if (dir = opendir(path)){
        while (ent = readdir(dir))
        {
            const char *fileName = ent->d_name;
            if ( strstr( ent->d_name, ".wbf")){
                //TODO: length check
                sprintf(fullPath,"%s/%s", path, fileName);
                printf("file: %s\n", fullPath);

                FILE *f = fopen(fullPath,"rb");
                fseek(f, 0, SEEK_END);
                size_t size = ftell(f);
                rewind(f);


                // load the wavetable
                buffer = (uint8_t*)malloc(size);
                fread(buffer, 1, size, f);
                fclose(f);

                memcpy(&wbf_header, buffer, sizeof(waveform_data_header));

                if (lot == wbf_header.fpl_lot){
                    break;

                free(buffer);
                buffer = nullptr;
            }
        }
        closedir(dir);
    }
    return buffer;

}
struct bongo {
    int a;
    int b;
};

int main()
{
    auto serial = get_serial();
    auto waveform = get_waveform(serial);

    if (!waveform) {
        cout << "cannot find etc" << endl;
    }

    return 0; 
}
