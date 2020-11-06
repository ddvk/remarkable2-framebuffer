#include <QCoreApplication>
#include <QImage>
#include <iostream>
#include <fbmanager.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>

using namespace std;


// load the serial number
uint8_t* serial() {


}

//TODO pass the serial
uint8_t *get_waveform() {
    const char* path = "/usr/share/remarkable";
    uint8_t *buffer = nullptr;
    DIR *dir;
    char fullPath[300];
    struct dirent *ent;
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


                //TODO: check if correct one
                break;
                free(buffer);
                fclose(f);

            }
        }
        closedir(dir);
    }
    return buffer;

}

int main()
{
    auto waveform = get_waveform();
    FBManager fb;
    fb.SendUpdate();
    cout << "Done" << endl;
    return 0; 
}
