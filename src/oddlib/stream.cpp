#include <sstream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include "logger.hpp"
#include "oddlib/stream.hpp"
#include "oddlib/exceptions.hpp"

namespace Oddlib
{
    Stream::Stream(std::vector<Uint8>&& data)
    {
        auto s = std::make_unique<std::stringstream>();
        std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(*s));
        mStream.reset(s.release());
        Seek(0);
    }

    Stream::Stream(const std::string& fileName)
    {
        auto s = std::make_unique<std::ifstream>();
        s->open(fileName, std::ios::binary);
        if (!*s)
        {
            LOG_ERROR("Lvl file not found %s", fileName.c_str());
            throw Exception("File not found");
        }
        mStream.reset(s.release());
    }

    void Stream::ReadUInt32(Uint32& output)
    {
        if (!mStream->read(reinterpret_cast<char*>(&output), sizeof(output)))
        {
            throw Exception("ReadUInt32 failure");
        }
    }

    void Stream::ReadUInt16(Uint16& output)
    {
        if (!mStream->read(reinterpret_cast<char*>(&output), sizeof(output)))
        {
            throw Exception("ReadUInt32 failure");
        }
    }

    void Stream::ReadBytes(Sint8* pDest, size_t destSize)
    {
        if (!mStream->read(reinterpret_cast<char*>(pDest), destSize))
        {
            throw Exception("ReadBytes failure");
        }
    }

    void Stream::ReadBytes(Uint8* pDest, size_t destSize)
    {
        if (!mStream->read(reinterpret_cast<char*>(pDest), destSize))
        {
            throw Exception("ReadBytes failure");
        }
    }

    void Stream::Seek(size_t pos)
    {
        if (!mStream->seekg(pos))
        {
            throw Exception("Seek failure");
        }
    }

    size_t Stream::Pos() const
    {
        return static_cast<size_t>(mStream->tellg());
    }

}