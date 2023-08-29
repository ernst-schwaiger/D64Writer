#ifndef D_64_WRITER_H
#define D_64_WRITER_H

#include <array>
#include <vector>
#include <string>
#include <ostream>

#include "TrackSector.h"

namespace d64
{

// class TrackSector
// {
// public:
//     uint8_t track; // zero-based track index
//     uint8_t sector; // zero-based sector index within track

//     bool operator == (TrackSector const &rhs) const
//     {
//         return ((track == rhs.track) && (sector == rhs.sector));
//     }

//     bool operator != (TrackSector const &rhs) const
//     {
//         return !(*this == rhs); 
//     }

//     static uint16_t getSectorIdx(TrackSector ts);
//     static TrackSector getTrackAndSector(uint16_t sectorIdx);
//     static uint8_t getSectorsOnTrack(uint8_t track);
//     static uint8_t getInterleaveOnTrack(uint8_t track);    
// };

class Writer
{
public:
    static constexpr uint16_t NUM_SECTORS = 683;
    static constexpr uint16_t NUM_TRACKS = 35;
    static constexpr uint16_t DIRECTORY_TRACK = 17;
    static constexpr uint16_t INVALID = 65535;

    static constexpr uint16_t BYTES_PER_SECTOR = 256;
    static constexpr uint16_t DATA_BYTES_PER_SECTOR = 254;
    static constexpr uint16_t BAM_SECTOR_IDX = 357;
    static constexpr uint16_t FIRST_DIR_SECTOR_IDX = 358;

    static constexpr uint8_t DIR_ENTRIES_PER_SECTOR = 8;
    static constexpr uint16_t BYTES_PER_DIR_ENTRY = 32;


    static constexpr TrackSector TRACK_SECTOR_INVALID = {255, 255};

    Writer(std::string const folderName = "Demo")
    {
        diskBytes.fill(0x00);
        initImage(folderName);
    }

    bool writeFile(std::string const &name, uint8_t *pData, size_t length);
    friend std::ostream & operator << (std::ostream &os, d64::Writer const &writer);

private:
    void initImage(std::string const &folderName);

    uint8_t *getSector(uint16_t idx) {  return &diskBytes[idx * BYTES_PER_SECTOR];}
    uint8_t const *getSector(uint16_t idx) const {  return &diskBytes[idx * BYTES_PER_SECTOR];}
    TrackSector getFirstFreeTrackSector() const;
    TrackSector getNextFreeTrackSector(TrackSector previous) const;

    uint8_t writeDataToSector(uint16_t sectorIdx, uint8_t *pData, size_t length, uint16_t prevSectorIdx);
    TrackSector writeData(uint8_t *pData, size_t length);

    bool isTrackSectorAvailable(TrackSector ts) const;
    void setSectorOccupied(uint16_t sectorIdx);

    uint16_t getNumberOfFreeSectors() const;
    size_t getNumberOfAvailableBytes() const { return getNumberOfFreeSectors() * DATA_BYTES_PER_SECTOR; }
    uint32_t getSectorBitsOfTrack(uint8_t trackIdx) const;

    std::string makeD64FileName(std::string const &origFileName);
    void writeString(uint8_t *pDest, std::string const &in);
    void writeStringWithPadding(uint8_t *pDest, std::string const &in, size_t len, uint8_t padding);

    std::array<uint8_t, BYTES_PER_SECTOR * NUM_SECTORS> diskBytes;
};

std::ostream & operator << (std::ostream &os, d64::Writer const &writer);

}


#endif