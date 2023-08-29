#include <assert.h>
#include "TrackSector.h"

namespace d64
{

uint16_t TrackSector::getSectorIdx(TrackSector ts)
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

TrackSector TrackSector::getTrackAndSector(uint16_t sectorIdx)
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
            static_cast<uint8_t>(((sectorIdx - 357) / 19) + 17),
            static_cast<uint8_t>((sectorIdx - 357) % 19)
        };
    }

    // tracks 24..29: 18 sectors per track
    if (sectorIdx < 598)
    {
        return TrackSector
        {
            static_cast<uint8_t>(((sectorIdx - 490) / 18) + 24),
            static_cast<uint8_t>((sectorIdx - 490) % 18)
        };
    }

    // tracks 30..34: 17 sectors per track
    return TrackSector
    {
        static_cast<uint8_t>(((sectorIdx - 598) / 17) + 30),
        static_cast<uint8_t>((sectorIdx - 598) % 17)
    };
}

uint8_t TrackSector::getSectorsOnTrack(uint8_t track)
{
    if (track < 17) return 21;
    if (track < 24) return 19;
    if (track < 30) return 18;
    return 17;
}

uint8_t TrackSector::getInterleaveOnTrack(uint8_t track)
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

}
