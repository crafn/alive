#pragma once

#include "SDL.h"
#include <vector>

namespace Oddlib
{
    class IStream;
    class CompressionType4Or5
    {
    public:
        CompressionType4Or5() = default;
        std::vector<Uint8> Decompress(IStream& stream, Uint32 finalW, Uint32 w, Uint32 h, Uint32 dataSize);
    };
}

