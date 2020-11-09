#include <QCoreApplication>
#include <QImage>
#include <iostream>
#include <fbmanager.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <inkwave.h>
#include <experimental/filesystem>
#include <fstream>
using namespace std;

int decode(const string& serial){
    cout << "Decoding: " << serial << endl;

    if (serial.length() != 25){
        cerr << "invalid epd serial length" << endl;
        return -1;
    }
    char a = serial[6];
    int part2 = serial[7]-'0';
    if (part2 > 9){
        return -1;
    }
    int part1 = 0;

    if (a == '0'){
        return part2;
    }

    if (a > 'Z' || a < 'A'){
        return -1;
    }
    if (a >= 'A' && a <= 'H') {
        part1 = 10 * (a - 'A') + 100;

    }
    else if (a >= 'O') {
        part1 = 10 * (a - 'Q') + 230;
    }
    else {
        part1 = 10 * (a - 'J') + 180;
    }

    return part1+part2;

}

// load the serial number
int get_serial() {

    string device = "/dev/mmcblk2boot1";
    ifstream fileBuffer(device, ios::in|ios::binary);
    uint32_t length;

    if (!fileBuffer.is_open())
    {
        cerr << "cannot open " << device << endl;
        return -1;
    }
    int field = 0;
    string epdSerial;

    while (true) {
        fileBuffer.read(reinterpret_cast<char*>(&length), 4);
        length = __builtin_bswap32(length);
        cout << "length: " << length;
        unique_ptr<char> tmpBuf (new char[length]);
        fileBuffer.read(tmpBuf.get(), length);

        string field_value(tmpBuf.get(), length);
        cout << " field: " << field_value << endl;
        if (field == 3) {
            epdSerial = tmpBuf.get();
            break;
        }
        field++;
    }

    int number = decode(epdSerial);
    return number;
}

uint8_t *get_waveform(int lot) {
    const char* path = "/usr/share/remarkable";
    uint8_t *buffer = nullptr;
    DIR *dir;
    char fullPath[300];
    struct dirent *ent;
    waveform_data_header wbf_header;
    if (!(dir = opendir(path))){
        cerr << "can't find: " << path << endl;
        return nullptr;
    }
    while ((ent = readdir(dir)) != nullptr)
    {
        const char *fileName = ent->d_name;
        if ( strstr( ent->d_name, ".wbf")){
            //TODO: length check
            sprintf(fullPath,"%s/%s", path, fileName);

            FILE *f = fopen(fullPath,"rb");
            fseek(f, 0, SEEK_END);
            //TODO: read the header only
            size_t size = ftell(f);
            rewind(f);

            buffer = (uint8_t*)malloc(size);
            fread(buffer, 1, size, f);
            fclose(f);

            memcpy(&wbf_header, buffer, sizeof(waveform_data_header));

            if (lot == wbf_header.fpl_lot){
                cout << "found match: " << fullPath << endl;
                break;
            }

            free(buffer);
            buffer = nullptr;
        }
    }
    closedir(dir);

    return buffer;
}


void parse_wv(uint8_t* waveform){
    struct waveform_data_header wfh;
    memcpy(&wfh, waveform, sizeof(waveform_data_header));

    print_header(&wfh, 1);
}

int main()
{
    auto serial = get_serial();
    if (serial < 0) {
        cerr << "wrong lot/serial" << endl;

    }
    auto waveform = get_waveform(serial);

    if (!waveform) {
        cerr << "cannot load waveform file" << endl;
    }
    parse_wv(waveform);
    return 0; 
}
