
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <algorithm>

#include "WriterTestHelper.h"

using namespace std;

namespace d64
{
    static uint8_t const *getSector(uint16_t idx, uint8_t const *pImage) {  return &pImage[idx * Writer::BYTES_PER_SECTOR];}

    static uint32_t getSectorBitsOfTrack(uint8_t trackIdx, uint8_t const *pImage)
    {
        uint8_t const *pBAM = getSector(Writer::BAM_SECTOR_IDX, pImage);
        uint8_t const *pTrackEntry = &pBAM[4 + trackIdx * 4];
        return pTrackEntry[1] + (pTrackEntry[2] << 8) + (pTrackEntry[3] << 16);
    }

    static bool isTrackSectorAvailable(TrackSector ts, uint8_t const *pImage)
    {
        uint32_t sectorBits = getSectorBitsOfTrack(ts.track, pImage);
        return (sectorBits & (1 << ts.sector));
    }

    static std::string getFileName(uint8_t const *pFileEntry)
    {
        std::stringstream strm;
        uint8_t idx = 0;

        while (idx < 16)
        {
            uint8_t ch = pFileEntry[5 + idx];
            if ((ch == 0xa0) || (ch == 0x00)) break;

            strm << ch;
            ++idx;
        }

        return strm.str();
    }

    static uint8_t const *getFileEntry(std::string const &d64ProgName, uint8_t const *pImage)
    {
        uint8_t const *pDIRSector = getSector(Writer::FIRST_DIR_SECTOR_IDX, pImage);
        uint8_t entryIdx = 0;

        while (entryIdx < 8)
        {
            uint8_t const *pFileEntry = &pDIRSector[ 32 * entryIdx];
            std::string entryFileName = getFileName(pFileEntry);
            if (entryFileName == d64ProgName)
            {
                break;
            }

            ++entryIdx;     
        }

        if (entryIdx >= 8)
        {
            FAIL ("Did not find a file entry with the given name.");
        }

        return &pDIRSector[ 32 * entryIdx];
    }

    static void assertValidTrackAndSector(uint8_t track, uint8_t sector)
    {
        if (track > 34)
        {
            FAIL("Illegal track number in TrackSector detected.");
        }

        if (sector >= TrackSector::getSectorsOnTrack(track))
        {
            FAIL("Illegal sector number in TrackSector detected.");
        }
    }

    static TrackSector getStartingTrackSector(uint8_t const *pFileEntry)
    {
        uint8_t track = pFileEntry[3];
        uint8_t sector = pFileEntry[4];
        assertValidTrackAndSector(track, sector);
        return TrackSector{static_cast<uint8_t>(track - 1), sector};
    }

    static uint16_t getNumberOfSectors(uint8_t const *pFileEntry)
    {
        return static_cast<uint16_t>(pFileEntry[30] + (pFileEntry[31] << 8));
    }

    static void getFilePayload(TrackSector ts, uint16_t sectors, uint8_t const *pImage, std::vector<uint8_t> &out)
    {
        uint8_t const *pSector = getSector(TrackSector::getSectorIdx(ts), pImage);

        while ((sectors > 0) && (pSector))
        {
            uint8_t track = pSector[0];
            uint8_t sector = pSector[1];

            if (track >= 1)
            {
                // Complete sector to stream, next Track and Sector must be valid
                ts = TrackSector{static_cast<uint8_t>(track - 1), sector};
                assertValidTrackAndSector(ts.track, ts.sector);

                std::for_each(&pSector[2], &pSector[256], [&out](uint8_t b) {out.push_back(b);} );
                pSector = getSector(TrackSector::getSectorIdx(ts), pImage);
            }
            else
            {
                // we are in the last sector of a file. Here, sector field indicates the number of
                // bytes occupied, anything in the range [1..254]
                if ((sector < 1) || (sector > 254))
                {
                    FAIL ("Detected illegal terminating TrackSector entry");
                }

                std::for_each(&pSector[2], &pSector[sector + 2], [&out](auto const b) {out.push_back(b);} );
                pSector = nullptr;
            }

            // The sector has to be marked as occupied ('0')
            if (isTrackSectorAvailable(ts, pImage))
            {
                FAIL("Track and Sector detected as available although containing file data");
            }

            ts = TrackSector{static_cast<uint8_t>(track - 1), sector};

            --sectors;
        }

        if (sectors)
        {
            FAIL("File occupies fewer sectors than specified");
        }
        if (pSector)
        {
            FAIL("File occupies more sectors than specified");
        }
    }

    void assertProgOnImage(std::vector<uint8_t> const &prog, std::string const &d64ProgName, D64ImgBuf &imageBuf)
    {
        uint8_t const *pImage = &imageBuf[0];
        uint8_t const *pDIRSector = getSector(Writer::FIRST_DIR_SECTOR_IDX, pImage);
        uint8_t const *pFileEntry = getFileEntry(d64ProgName, pImage);

        std::vector<uint8_t> progInImage; // this shall contain our program after getFilePayload() is called
        getFilePayload(getStartingTrackSector(pFileEntry), getNumberOfSectors(pFileEntry), pImage, progInImage);

        if (prog != progInImage)
        {
            FAIL("The program in the image does not equal the written program.");
        }

    }

    void writeImageToBuf(D64ImgBuf &dest, Writer &src)
    {
        D64ImageStreamBuf sb(dest);
        std::ostream testStream(&sb);
        testStream << src;
    }

}