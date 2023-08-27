#ifndef D_64_WRITER_H
#define D_64_WRITER_H

#include <array>
#include <vector>
#include <string>


namespace d64
{

struct TrackSector_tag
{
    uint8_t track; // zero-based track index
    uint8_t sector; // zero-based sector index within track

    bool operator == (TrackSector_tag const &rhs) const
    {
        return ((track == rhs.track) && (sector == rhs.sector));
    }

    bool operator != (TrackSector_tag const &rhs) const
    {
        return !(*this == rhs); 
    }
};

typedef struct TrackSector_tag TrackSector;

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

    Writer()
    {
        diskBytes.fill(0x00);
        initImage();
    }

    bool writeFile(std::string const &name, uint8_t *pData, size_t length);

private:
    void initImage();

    uint8_t *getSector(uint16_t idx) {  return &diskBytes[idx * BYTES_PER_SECTOR];}
    uint8_t const *getSector(uint16_t idx) const {  return &diskBytes[idx * BYTES_PER_SECTOR];}
    TrackSector getFirstFreeTrackSector() const;
    TrackSector getNextFreeTrackSector(TrackSector previous) const;

    uint8_t writeDataToSector(uint16_t sectorIdx, uint8_t *pData, size_t length, uint16_t prevSectorIdx);
    TrackSector writeData(uint8_t *pData, size_t length);

    void setSectorOccupied(uint16_t sectorIdx);

    TrackSector getTrackAndSector(uint16_t sectorIdx) const;
    uint16_t getSectorIdx(TrackSector ts) const;
    

    uint16_t getNumberOfFreeSectors() const;
    uint16_t getNumberOfAvailableBytes() const { return getNumberOfFreeSectors() * DATA_BYTES_PER_SECTOR; }
    uint32_t getSectorBitsOfTrack(uint8_t trackIdx) const;
    uint8_t getSectorsOnTrack(uint8_t track) const;
    uint8_t getInterleaveOnTrack(uint8_t track) const;

    std::string makeD64FileName(std::string const &origFileName);
    void writeString(uint8_t *pDest, std::string const &in);
    void writeStringWithPadding(uint8_t *pDest, std::string const &in, size_t len, uint8_t padding);

    std::array<uint8_t, BYTES_PER_SECTOR * NUM_SECTORS> diskBytes;
};

}


#endif