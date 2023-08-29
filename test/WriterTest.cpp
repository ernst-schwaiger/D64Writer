/*
 * MOS6502AssemblerTest.cpp
 *
 *  Created on: 19.08.2018
 *      Author: Ernst
 */
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>
#include <fstream>
#include <ostream>
#include <streambuf>
#include <algorithm>
#include <sstream>

#include "Writer.h"
#include "WriterTestHelper.h"
using namespace std;

namespace d64
{
    static std::string makeFileName(uint16_t idx)
    {
        std::stringstream strm;
        strm << "FILE_" << idx;
        return strm.str();
    }

    static void makeFileContent(uint8_t idx, size_t length, std::vector<uint8_t> &out)
    {
        out.clear();
        for (size_t idx = 0; idx < length; idx++)
        {
            out.push_back(idx);
        }
    }    
   
    TEST_CASE( "Small file", "Writer" )
    {
        vector<uint8_t> myProg = 
        {
            0x00, 0xc0, // starting address
            0xa0, 0x00, 
            0x8c, 0x20, 0xd0, 
            0xc8, 
            0xd0, 0xfa,
            0xea,
            0x60,
            0
        };

        Writer w;
        bool success = w.writeFile("hollarie.txt", &myProg[0], myProg.size());
        if (!success)
        {
            FAIL("File could not be written");
        }

        D64ImgBuf imageBuf;
        writeImageToBuf(imageBuf, w);
        assertProgOnImage(myProg, "HOLLARIE.TXT", imageBuf);
    }

    TEST_CASE( "Large file", "Writer" )
    {
        vector<uint8_t> myProg;
        // All sectors minus the sectors on track 16 (which are 19 sectors)
        uint16_t availableDataSectors = Writer::NUM_SECTORS - 19;

        for (uint16_t i = 0; i < Writer::NUM_SECTORS - 19; i++)
        {
            for(uint8_t j = 0; j < Writer::DATA_BYTES_PER_SECTOR; j++)
            {
                myProg.push_back(static_cast<uint8_t>(i & 0xff));
            }
        }

        Writer w;
        bool success = w.writeFile("big", &myProg[0], myProg.size());

        if (!success)
        {
            FAIL("File could not be written");
        }

        D64ImgBuf imageBuf;
        writeImageToBuf(imageBuf, w);

        assertProgOnImage(myProg, "BIG", imageBuf);        
    }

    TEST_CASE("Maximum Supported Files", "Writer")
    {
        uint16_t availableDataSectors = Writer::NUM_SECTORS - 19;
        size_t availableBytesOnImage = availableDataSectors * Writer::DATA_BYTES_PER_SECTOR;
        uint8_t maxSupportedFiles = 8;
        size_t fileLength = availableBytesOnImage / maxSupportedFiles;

        std::vector<uint8_t> file;
        Writer w;

        // write 8 files that fill up the whole image
        for (uint8_t fileIdx = 0; fileIdx < maxSupportedFiles; fileIdx++)
        {
            std::string fileName = makeFileName(fileIdx);
            makeFileContent(fileIdx, fileLength, file);
            bool success = w.writeFile(fileName, &file[0], file.size());

            if (!success)
            {
                FAIL("File could not be written");
            }
        }

        D64ImgBuf imageBuf;
        writeImageToBuf(imageBuf, w);        

        for (uint8_t fileIdx = 0; fileIdx < maxSupportedFiles; fileIdx++)
        {
            std::string fileName = makeFileName(fileIdx);
            makeFileContent(fileIdx, availableBytesOnImage / maxSupportedFiles, file);

            assertProgOnImage(file, fileName, imageBuf);  
        }
    }
}
