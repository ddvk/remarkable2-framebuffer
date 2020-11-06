# how i think it works


* read the device serial
* scan /usr/share/remarkable for wbf files
* load the file matching the serial

* get the ambient temperature (and check every minute)
* load the corresponding table


* init
* creates a 1404x1872 buffer


* start 2 threads
* thread 1 waits for events
* get's the updated region
* does >> 1 & 0x0F for every pixel and creates a temporary map
* loads the previous state and ORs with (0xF0)  


pan(offset 16)
* foreach waveform entry in the table  
    * maps the pixel with the correct waveform (old new)
    * writes to fbmmap at offset table length % 16
    * clears the fbmmap region
* ioctl (fbmmap, PAN, yoffset + 1404*offset)
pan(offset 16)





