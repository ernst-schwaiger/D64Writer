#ifndef TRACK_SECTOR_H
#define TRACK_SECTOR_H

#include <cstdint>

namespace d64
{
class TrackSector
{
public:
    static constexpr uint16_t INVALID = 65535;

    uint8_t track; // zero-based track index
    uint8_t sector; // zero-based sector index within track

    bool operator == (TrackSector const &rhs) const
    {
        return ((track == rhs.track) && (sector == rhs.sector));
    }

    bool operator != (TrackSector const &rhs) const
    {
        return !(*this == rhs); 
    }

    static uint16_t getSectorIdx(TrackSector ts);
    static TrackSector getTrackAndSector(uint16_t sectorIdx);
    static uint8_t getSectorsOnTrack(uint8_t track);
    static uint8_t getInterleaveOnTrack(uint8_t track);    
};

}

#endif
