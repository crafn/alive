#pragma once

#include "SDL.h"
#include <memory>
#include <string>

namespace Oddlib
{
    class IStream;

    class IBits
    {
    public:
        virtual ~IBits() = default;
        virtual SDL_Surface* GetSurface() const = 0;

        void Save()
        {
            static int i = 1;
            SDL_SaveBMP(GetSurface(), ("camera" + std::to_string(i++) + ".bmp").c_str());
        }
    };

    bool IsPsxCamera(IStream& stream);
    std::unique_ptr<IBits> MakeBits(IStream& stream);
}
