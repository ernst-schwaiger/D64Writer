# D64Writer
Generates D64 images out of files in a given folder

## Build

```
mkdir D64Writer/build
cd D64Writer/build
cmake ..
make -j
```

## Usage
Usage: D64Writer <srcpath> <imagepath>
Reads all '.prg' files found in a folder path to generate a .D64 image file.