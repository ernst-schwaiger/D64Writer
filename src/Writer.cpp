
#include "Writer.h"
#include <sstream>
#include <cstring> // std::memset
#include <algorithm>
#include <assert.h>

using namespace d64;
using namespace std;

void Writer::initImage(std::string const &folderName)
{
    uint8_t *pBAM = getSector(BAM_SECTOR_IDX);
    pBAM[0] = TrackSector::getTrackAndSector(BAM_SECTOR_IDX).track + 1;
    pBAM[1] = 0x01; // is ignored, next sector is sector #3
    pBAM[2] = 0x41; // DOS version type
    pBAM[3] = 0x00; // unused

    // 0x04..0x8F BAM track entries cover which sector on which track is free/occupied
    for (uint8_t trackIdx = 0; trackIdx < NUM_TRACKS; trackIdx++)
    {
        uint8_t *pTrackEntry = &pBAM[4 + trackIdx* 4];
        // 4 bytes per track, first byte indicates the free sectors per track
        uint8_t sectorsOnTrack = TrackSector::getSectorsOnTrack(trackIdx);
        pTrackEntry[0] = sectorsOnTrack;
        // Bytes 1..3 provide '1' flag for available, '0' for occupied sector
        // indexing starts with first byte MSB first, i.e. MSB of first byte
        // contains status of sector 0
        uint32_t occupiedBits = (1 << sectorsOnTrack) - 1;
        pTrackEntry[1] = static_cast<uint8_t>(occupiedBits & 0xff);
        pTrackEntry[2] = static_cast<uint8_t>((occupiedBits >> 8) & 0xff);
        pTrackEntry[3] = static_cast<uint8_t>((occupiedBits >> 16) & 0xff);
    }
    setSectorOccupied(BAM_SECTOR_IDX); // the sector in which we are just writing

    writeStringWithPadding(&pBAM[0x90], makeD64FileName(folderName), 16, 0xa0); // Disk Name
    pBAM[0xa0] = 0xa0;
    pBAM[0xa1] = 0xa0;
    pBAM[0xa2] = 0x34; // Disk ID "42"
    pBAM[0xa3] = 0x32; // Disk ID
    pBAM[0xa4] = 0xa0;  
    pBAM[0xa5] = 0x32; // DOS Type "2A"
    pBAM[0xa6] = 0x41; 
    pBAM[0xa7] = 0xa0;
    pBAM[0xa8] = 0xa0;
    pBAM[0xa9] = 0xa0;
    pBAM[0xaa] = 0xa0;

    std::memset(&pBAM[0xab], 0x00, 0xff-0xab); // rest of BAM is unused
}

void Writer::setSectorOccupied(uint16_t sectorIdx)
{
    uint8_t *pBAM = getSector(BAM_SECTOR_IDX);
    TrackSector ts = TrackSector::getTrackAndSector(sectorIdx);
    uint8_t *pBAMEntryTrack = &pBAM[0x04 + (ts.track * 4)];
    --pBAMEntryTrack[0]; // number of free sectors resuced by one (ours)
    uint8_t *pBAMEntrySector = &pBAMEntryTrack[1 + ts.sector / 8];
    uint8_t mask = (1 << (ts.sector % 8)) ^ 0xff;
    *pBAMEntrySector &= mask;
}


uint32_t Writer::getSectorBitsOfTrack(uint8_t trackIdx) const
{
    uint8_t const *pBAM = getSector(BAM_SECTOR_IDX);
    uint8_t const *pTrackEntry = &pBAM[4 + trackIdx * 4];
    return pTrackEntry[1] + (pTrackEntry[2] << 8) + (pTrackEntry[3] << 16);
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
            // first byte of a track entry holds the free sectors
            ret = ret + pBAM[(trackIdx + 1) * 4];
        }
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
            strm << static_cast<uint8_t>((ch - 'a') + 'A');
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
    TrackSector ret = {0,0};
    return isTrackSectorAvailable(ret) ? ret : getNextFreeTrackSector(ret);
}

TrackSector Writer::getNextFreeTrackSector(TrackSector previous) const
{
    uint8_t sectorStartIDx = previous.sector;

    for (uint8_t trackIdx = previous.track; trackIdx < NUM_TRACKS; trackIdx++)
    {
        if (trackIdx != DIRECTORY_TRACK)
        {
            uint8_t interleave = TrackSector::getInterleaveOnTrack(trackIdx);
            uint8_t numSectors = TrackSector::getSectorsOnTrack(trackIdx);
            uint32_t sectorBits = getSectorBitsOfTrack(trackIdx);

            for (uint8_t i = 0; i < numSectors; i++)
            {
                uint8_t sectorOnTrack = static_cast<uint8_t>((sectorStartIDx + (i * interleave)) % numSectors);
                TrackSector nextTS = {trackIdx, sectorOnTrack};
                if (isTrackSectorAvailable(nextTS))
                {
                    return nextTS;
                }
            }
        }

        // complete track is full, we continue on sector 0 of next track
        sectorStartIDx = 0;
    }

    return TRACK_SECTOR_INVALID;
}

bool Writer::isTrackSectorAvailable(TrackSector ts) const
{
    uint32_t sectorBits = getSectorBitsOfTrack(ts.track);
    return (sectorBits & (1 << ts.sector));
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
            pDirEntry[0] = 0x00; // invalid, there is no next entry
            pDirEntry[1] = 0xff; // invalid, there is no next entry
        }
        else
        {
            pDirEntry[0] = 0x00;
            pDirEntry[1] = 0x00;
        }

        pDirEntry[2] = 0x82; // .PRG
        TrackSector ts = writeData(pData, length);

        if (ts != TRACK_SECTOR_INVALID)
        {
            pDirEntry[3] = ts.track + 1;
            pDirEntry[4] = ts.sector;

            string d64Name = makeD64FileName(name);
            writeStringWithPadding(&pDirEntry[5], d64Name, 16, 0xa0);
            // 9 bytes unused for .PRG leave at 0x00 as is
            // file length in sectors, aka "blocks", little endian
            uint16_t numberOfBlocks = (length + (DATA_BYTES_PER_SECTOR - 1)) / DATA_BYTES_PER_SECTOR;

            pDirEntry[30] = static_cast<uint8_t>(numberOfBlocks & 0xff);
            pDirEntry[31] = static_cast<uint8_t>((numberOfBlocks >> 8) & 0xff);

            success = true;
        }
    }


    return success;
}


std::ostream & d64::operator << (std::ostream &os, d64::Writer const &writer)
{
    for (auto ch : writer.diskBytes)
    {
        os << ch;
    }

    return os;
}


TrackSector Writer::writeData(uint8_t *pData, size_t length)
{
    TrackSector ret = TRACK_SECTOR_INVALID;

    size_t availableBytes = getNumberOfAvailableBytes();

    if (length <= availableBytes && length > 0)
    {
        ret = getFirstFreeTrackSector();

        uint16_t sectorIdx = TrackSector::getSectorIdx(ret);
        uint16_t previousSectorIdx = INVALID;

        while (length && (sectorIdx != INVALID))
        {
            uint8_t writtenData = writeDataToSector(sectorIdx, pData, length, previousSectorIdx);
            length -= writtenData;
            pData=&pData[writtenData];

            previousSectorIdx = sectorIdx;
            // Check is there a way to get this faster?
            sectorIdx = TrackSector::getSectorIdx(getNextFreeTrackSector(TrackSector::getTrackAndSector(sectorIdx)));
        }
    }

    return ret;
}

// returns the number of written bytes
uint8_t Writer::writeDataToSector(uint16_t sectorIdx, uint8_t *pData, size_t length, uint16_t prevSectorIdx)
{
    uint8_t *pSector = getSector(sectorIdx);
    uint8_t ret = std::min(length, static_cast<size_t>(DATA_BYTES_PER_SECTOR));

    // link to previous sector
    if (prevSectorIdx != INVALID)
    {
        uint8_t *pPrevSector = getSector(prevSectorIdx);
        TrackSector tsCurrent = TrackSector::getTrackAndSector(sectorIdx);
        pPrevSector[0] = tsCurrent.track + 1; // on the disk system, tracks start with "1"
        pPrevSector[1] = tsCurrent.sector; // but sectors are still zero-based
    }

    // copy data
    std::copy(&pData[0], &pData[ret], &pSector[2]);
    // mark as occupied, prevents overwriting
    setSectorOccupied(sectorIdx);

    if (ret <= DATA_BYTES_PER_SECTOR)
    {
        // this is the last sector of our file. conclude with track := 0, sector := <used bytes>
        pSector[0] = 0x00;
        pSector[1] = ret;
    }

    return ret;
}



