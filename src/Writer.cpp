
#include "Writer.h"
#include <sstream>
#include <cstring> // std::memset
#include <algorithm>
#include <assert.h>

using namespace d64;
using namespace std;

// copy/paste/change from
// https://en.wikipedia.org/wiki/Hamming_weight
static int popcount64c(uint64_t x)
{
    const uint64_t m1  = 0x5555555555555555; //binary: 0101...
    const uint64_t m2  = 0x3333333333333333; //binary: 00110011..
    const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
    const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...

    x -= (x >> 1) & m1;             //put count of each 2 bits into those 2 bits
    x = (x & m2) + ((x >> 2) & m2); //put count of each 4 bits into those 4 bits 
    x = (x + (x >> 4)) & m4;        //put count of each 8 bits into those 8 bits 
    return (x * h01) >> 56;  //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
}

void Writer::initImage()
{
    uint8_t *pBAM = getSector(BAM_SECTOR_IDX);
    pBAM[0] = getTrackAndSector(BAM_SECTOR_IDX).track + 1;
    pBAM[1] = 0x01; // is ignored, next sector is sector #3
    pBAM[2] = 0x41; // DOS version type
    pBAM[3] = 0x00; // unused


    // 0x04..0x8F BAM track entries cover which sector on which track is free/occupied
    for (uint8_t trackIdx = 0; trackIdx < NUM_TRACKS; trackIdx++)
    {
        // 4 bytes per track, little endian, '1' marks free sector
        // unavailable sectors marked as occupied  '0'
        uint32_t occupiedBits = (1 << getSectorsOnTrack(trackIdx)) - 1;
        pBAM[((trackIdx + 1) * 4)] = occupiedBits & 0xff;
        pBAM[((trackIdx + 1) * 4) + 1] = (occupiedBits >> 8) & 0xff;
        pBAM[((trackIdx + 1) * 4) + 2] = (occupiedBits >> 16) & 0xff;
        pBAM[((trackIdx + 1) * 4) + 3] = (occupiedBits >> 24) & 0xff;
    }
    setSectorOccupied(BAM_SECTOR_IDX); // the sector in which we are just writing

    writeStringWithPadding(&pBAM[0x90], "DISK", 16, 0xa0); // Disk Name
    pBAM[0xa0] = 0xa0;
    pBAM[0xa1] = 0xa0;
    pBAM[0xa2] = 0x56;
    pBAM[0xa3] = 0x54;
    pBAM[0xa4] = 0xa0;
    pBAM[0xa5] = 0x2a;
    pBAM[0xa6] = 0x2a;
    pBAM[0xa7] = 0xa0;
    pBAM[0xa8] = 0xa0;
    pBAM[0xa9] = 0xa0;
    pBAM[0xaa] = 0xa0;

    std::memset(&pBAM[0xab], 0x00, 0xff-0xab); // rest of BAM is unused
}

void Writer::setSectorOccupied(uint16_t sectorIdx)
{
    uint8_t *pBAM = getSector(BAM_SECTOR_IDX);
    TrackSector ts = getTrackAndSector(sectorIdx);
    uint8_t *pBAMEntryByte = &pBAM[0x04 + (ts.track * 4) + (ts.sector / 8)];

    // clearing the bit of the sector marks it as occupied
    *pBAMEntryByte &= (0xff - (1 << ts.sector % 8));
}

// although in the documentation, tracks start with track number 1, we
// still start with zero, to make calculations easier.
TrackSector  Writer::getTrackAndSector(uint16_t sectorIdx) const
{
    // tracks 0..16: 21 sectors per track
    if (sectorIdx < 357)
    {
        return TrackSector
        {
            static_cast<uint8_t>(sectorIdx / 21),
            static_cast<uint8_t>(sectorIdx % 21)
        };
    }

    // tracks 17..23: 19 sectors per track
    if (sectorIdx < 490)
    {
        return TrackSector
        {
            static_cast<uint8_t>((sectorIdx - 357) / 19),
            static_cast<uint8_t>((sectorIdx - 357) % 19)
        };
    }

    // tracks 24..29: 18 sectors per track
    if (sectorIdx < 598)
    {
        return TrackSector
        {
            static_cast<uint8_t>((sectorIdx - 490) / 18),
            static_cast<uint8_t>((sectorIdx - 490) % 18)
        };
    }

    // tracks 30..34: 17 sectors per track
    return TrackSector
    {
        static_cast<uint8_t>((sectorIdx - 598) / 17),
        static_cast<uint8_t>((sectorIdx - 598) % 17)
    };
}

uint16_t Writer::getSectorIdx(TrackSector ts) const
{
    uint16_t ret = INVALID;

    if (ts.track < 17)
    {
        return (ts.track * 21) + ts.sector;
    }

    if (ts.track < 24)
    {
        return (17 * 21) + ((ts.track - 17) * 19) + ts.sector;
    }

    if (ts.track < 30)
    {
        return (17 * 21) + (7 * 19) + ((ts.track - 24) * 18) + ts.sector;
    }

    return (17 * 21) + (7 * 19) + (6 * 18) + ((ts.track - 30) * 17) + ts.sector;
}



uint32_t Writer::getSectorBitsOfTrack(uint8_t trackIdx) const
{
    uint8_t const *pBAM = getSector(BAM_SECTOR_IDX);
    uint32_t sectorBits = 
        pBAM[((trackIdx + 1) * 4)] +
        pBAM[((trackIdx + 1) * 4) + 1] << 8 +
        pBAM[((trackIdx + 1) * 4) + 2] << 16 +
        pBAM[((trackIdx + 1) * 4) + 3] << 24;

    return sectorBits;
}

uint16_t Writer::getNumberOfFreeSectors() const
{
    uint16_t ret = 0;
    uint8_t const *pBAM = getSector(BAM_SECTOR_IDX);

    for (uint8_t trackIdx = 0; trackIdx < NUM_TRACKS; trackIdx++)
    {
        // directory track cannot be used for data
        if (trackIdx != DIRECTORY_TRACK)
        {
            uint32_t sectorBits = getSectorBitsOfTrack(trackIdx);
            ret = ret + static_cast<uint16_t>(popcount64c(sectorBits));
        }
    }

    return ret;
}

// takes zero byte index of track
uint8_t Writer::getSectorsOnTrack(uint8_t track) const
{
    if (track < 17) return 21;
    if (track < 24) return 19;
    if (track < 30) return 18;
    return 17;
}

// a number relative prime to the number of sectors on the track,
// so we can visit every sector
uint8_t Writer::getInterleaveOnTrack(uint8_t track) const
{
    uint8_t ret = 0;
    switch(getSectorsOnTrack(track))
    {
        case 21:
        case 19:
            ret = 10;
            break;
        case 18:
            ret = 7;
            break;
        case 17:
            ret = 9;
            break;
        default:
            assert(0);
    }

    return ret;
}



std::string Writer::makeD64FileName(std::string const &origFileName)
{
    std::stringstream strm;

    for (size_t idx = 0; idx < std::min(origFileName.length(), static_cast<size_t>(16)); idx++)
    {
        uint8_t ch = origFileName[idx];
        if ((ch >= '0' && ch <= '9') || (ch >= 'A'  && ch <= 'Z') || (ch == '_') || (ch == '.'))
        {
            strm << ch;
        }
        else if (ch >= 'a'  && ch <= 'z')
        {
            // toUpper
            strm << (ch - 'a') + 'A';
        }
        else
        {
            strm << '_';
        }
    }

    return strm.str();
}

void Writer::writeString(uint8_t *pDest, std::string const &in)
{
    size_t idx = 0;
    for (auto ch : in)
    {
        pDest[idx++] = ch;
    }
}

void Writer::writeStringWithPadding(uint8_t *pDest, std::string const &in, size_t len, uint8_t padding)
{
    writeString(pDest, in);
    for (size_t idx = in.length(); idx < len; idx++)
    {
        pDest[idx] = padding;
    }
}

TrackSector Writer::getFirstFreeTrackSector() const
{
    TrackSector ret = TRACK_SECTOR_INVALID;
    uint8_t const *pBAM = getSector(BAM_SECTOR_IDX);

    for (uint8_t trackIdx = 0; trackIdx < NUM_TRACKS; trackIdx++)
    {
        if (trackIdx != DIRECTORY_TRACK)
        {
            // found a track with at least one sector
            uint32_t sectorBits = getSectorBitsOfTrack(trackIdx);
            uint8_t sectorsOfTrack = getSectorsOnTrack(trackIdx);

            // get the track idx, find the first set bit
            uint8_t sectorIdx = 0;
            while ((sectorIdx < sectorsOfTrack) && (!(sectorBits && (0x01 << sectorIdx))))
            {
                sectorIdx++;
            }

            ret = {trackIdx, sectorIdx};
            break;
        }
    }

    return ret;
}

TrackSector Writer::getNextFreeTrackSector(TrackSector previous) const
{
    uint8_t sectorStartIDx = previous.sector;

    for (uint8_t trackIdx = previous.track; trackIdx < NUM_TRACKS; trackIdx++)
    {
        if (trackIdx != DIRECTORY_TRACK)
        {
            uint8_t interleave = getInterleaveOnTrack(trackIdx);
            uint8_t numSectors = getSectorsOnTrack(trackIdx);
            uint32_t sectorBits = getSectorBitsOfTrack(trackIdx);

            for (uint8_t i = 0; i < numSectors; i++)
            {
                uint8_t sectorOnTrack = static_cast<uint8_t>((sectorStartIDx + (i * interleave)) % numSectors);
                if (sectorBits & (0x01 << sectorOnTrack))
                {
                    return {previous.track, previous.sector};
                }
            }
        }

        // complete track is full, we continue on sector 0 of next track
        sectorStartIDx = 0;
    }

    return TRACK_SECTOR_INVALID;
}


bool Writer::writeFile(string const &name, uint8_t *pData, size_t length)
{
    bool success = false;
    uint8_t *pFirstDirSector = getSector(FIRST_DIR_SECTOR_IDX); // sector 1 on DIRECTORY_TRACK

    // FIXME: Put that into a separate functions
    uint8_t dirIdx = 0;
    uint8_t *pDirEntry = &pFirstDirSector[BYTES_PER_DIR_ENTRY * dirIdx];
    while ((dirIdx < DIR_ENTRIES_PER_SECTOR) && (pDirEntry[2] != 0))
    {
        ++dirIdx;
        pDirEntry = &pFirstDirSector[BYTES_PER_DIR_ENTRY * dirIdx];
    }

    // For now, we do not use the other directory sectors
    if (dirIdx < DIR_ENTRIES_PER_SECTOR)
    {
        if (dirIdx == 0)
        {
            pDirEntry[0] = DIRECTORY_TRACK + 1; // 1 based
            pDirEntry[1] = 4; // next sector, interleave is 3
        }
        else
        {
            pDirEntry[0] = 0x00;
            pDirEntry[1] = 0x00;
        }

        pDirEntry[2] = 2; // .PRG
        TrackSector ts = writeData(pData, length);

        if (ts != TRACK_SECTOR_INVALID)
        {
            pDirEntry[3] = ts.track + 1;
            pDirEntry[4] = ts.sector;

            string d64Name = makeD64FileName(name);
            writeStringWithPadding(&pDirEntry[5], d64Name, d64Name.length(), 0xa0);
            // 9 bytes unused for .PRG leave at 0x00 as is
            // file length in bytes, little endian
            pDirEntry[30] = static_cast<uint8_t>(length & 0xff);
            pDirEntry[31] = static_cast<uint8_t>((length >> 8) & 0xff);

            success = true;
        }
    }


    return success;

}


TrackSector Writer::writeData(uint8_t *pData, size_t length)
{
    TrackSector ret = TRACK_SECTOR_INVALID;

    if (length < getNumberOfAvailableBytes() && length > 0)
    {
        TrackSector ret = getFirstFreeTrackSector();

        uint16_t sectorIdx = getSectorIdx(ret);
        uint16_t previousSectorIdx = INVALID;

        while (length && (sectorIdx != INVALID))
        {
            length -= writeDataToSector(sectorIdx, pData, length, previousSectorIdx);


            previousSectorIdx = sectorIdx;
            // Check is there a way to get this faster?
            sectorIdx = getSectorIdx(getNextFreeTrackSector(getTrackAndSector(sectorIdx)));
        }
    }

    return ret;
}

// returns the number of written bytes
uint8_t Writer::writeDataToSector(uint16_t sectorIdx, uint8_t *pData, size_t length, uint16_t prevSectorIdx)
{
    uint8_t *pSector = getSector(sectorIdx);
    uint8_t ret = std::min(length, static_cast<size_t>(254));

    // link to previous sector
    if (prevSectorIdx != INVALID)
    {
        uint8_t *pPrevSector = getSector(prevSectorIdx);
        TrackSector tsCurrent = getTrackAndSector(sectorIdx);
        pPrevSector[0] = tsCurrent.track + 1; // on the disk system, tracks start with "1"
        pPrevSector[1] = tsCurrent.sector; // but sectors are still zero-based
    }

    // copy data
    std::copy(&pSector[2], &pSector[2 + ret], pData);
    // mark as occupied, prevents overwriting
    setSectorOccupied(sectorIdx);

    return ret;
}



