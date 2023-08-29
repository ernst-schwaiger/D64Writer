
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <array>

// POSIX API to read folders and files within
#include <sys/types.h>
#include <dirent.h>

#include "Writer.h"

using namespace std;
using namespace d64;

static constexpr size_t MAX_PROG_FILE_LEN = 65535;

void usage(char const *argv0)
{
    cerr << "Usage: " << argv0 << " <srcpath> <imagepath>" << endl;
    cerr << "Reads a file or a folder path to generate a .D64 image" << endl;
}

// returns a value > 0 if a .PRG file was read into the buffer
size_t readProgFile(std::string const &filePath, array<uint8_t, MAX_PROG_FILE_LEN> &fileBuf)
{
    // check .prg or .PRG extension
    // first two bytes offset in Memory
    // offset + byte length < 64k
    size_t ret = 0;

    std::string fileSuffix = filePath.length() > 4 ? filePath.substr(filePath.length() - 4, 4) : "";

    if (fileSuffix == ".PRG" || fileSuffix == ".prg")
    {
        std::ifstream is(filePath, ios_base::in | ios_base::binary);
        is.seekg(0, ios::end);
        streampos fileSize = is.tellg();

        if (fileSize <= MAX_PROG_FILE_LEN)
        {
            is.seekg(0, ios::beg);
            static_assert(sizeof(char) == sizeof(uint8_t), "the types char and uint8_t do not have the same size");
            is.read(reinterpret_cast<char *>(&fileBuf[0]), fileSize);
            uint16_t offset = fileBuf[0] + (fileBuf[1] << 8);

            // very unsufficient check. we cannot guarantee that parts of the file won't
            // show up in memory since the area is taken by the basic interpreter or OS
            if (offset + fileSize - 1 <= MAX_PROG_FILE_LEN)
            {
                ret = fileSize;
            }
        }
    }

    return ret;
}

// returns the name of the bottommost directory of the path
// getDirName("./foo/bar/baz") returns "baz"
std::string getDirName(std::string path)
{
    // if present, remove trailing '/'
    if (path.at(path.length() - 1) == '/')
    {
        path = path.substr(0, path.length() -1);
    }

    auto pos = path.find_last_of('/');
    return (pos >= 0) ? path.substr(pos + 1) : path;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        usage (argv[0]);
        return 1;
    }

    // FIXME: Do we have to close any handles after opendir()/readdir()?
    DIR *pDIR = opendir(argv[1]);
    if (pDIR != nullptr)
    {
        struct dirent *dp = nullptr;
        array<uint8_t, MAX_PROG_FILE_LEN> fileBuf;
        Writer d64Writer(getDirName(argv[1]));

        bool writeOK = true;

        while (((dp = readdir(pDIR)) != nullptr) && writeOK)
        {
            std::stringstream filePath;
            filePath << argv[1] << "/" << dp->d_name;

            // put the file content into the array, if name and content of file pass our checks
            size_t readBytes = readProgFile(filePath.str(), fileBuf);

            if (readBytes > 0)
            {
                writeOK = d64Writer.writeFile(dp->d_name, &fileBuf[0], readBytes);
            }

            if (!writeOK)
            {
                cerr << "Could not write file " << dp->d_name << " to image." << std::endl;
            }
        }

        // all files have been added, now generate the image
        if (writeOK)
        {
            std::ofstream d64Image(argv[2], std::ios::binary | std::ios::out | std::ios::trunc);
            d64Image << d64Writer;
            d64Image.close();
        }
        else
        {
            return 1;
        }
    }
    else
    {
        // error handling: did not find folder
        cerr << "Could not find folder " << argv[1] << "." << std::endl;
        return 1;
    }

    return 0;
}