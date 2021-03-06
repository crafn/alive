#pragma once

#include "SDL.h"
#include <string>
#include <vector>
#include <array>

namespace Oddlib
{
    class IStream;


    class Path
    {
    public:
        struct Point
        {
            Uint16 mX;
            Uint16 mY;
        };
        static_assert(sizeof(Point) == 4, "Wrong point size");

        struct MapObject
        {
            // TLV
            Uint16 mFlags;
            Uint16 mLength;
            Uint32 mType;

            // RECT
            Point mRectTopLeft;
            Point mRectBottomRight;

            // TODO: Set to biggest known size
            // Assume 64 bytes is the biggest length for now
            std::array<Uint8, 512> mData;
        };

        class Camera
        {
        public:
            explicit Camera(const std::string& name)
                : mName(name)
            {

            }
            std::string mName;
            std::vector<MapObject> mObjects;
        };

        struct CollisionItem
        {
            Point mP1;
            Point mP2;
            Uint16 mType;
            // TODO Actually contains links to previous/next collision
            // item link depending on the type 
            Uint16 mUnknown[4];
            Uint16 mLineLength;
        };
        static_assert(sizeof(CollisionItem) == 20, "Wrong collision item size");

        Path(const Path&) = delete;
        Path& operator = (const Path&) = delete;
        Path(IStream& pathChunkStream, 
             Uint32 collisionDataOffset,
             Uint32 objectIndexTableOffset, 
             Uint32 objectDataOffset,
             Uint32 mapXSize, 
             Uint32 mapYSize,
             bool isAo);

        Uint32 XSize() const;
        Uint32 YSize() const;
        const Camera& CameraByPosition(Uint32 x, Uint32 y) const;
        const std::vector<CollisionItem>& CollisionItems() const { return mCollisionItems; }
        bool IsAo() const { return mIsAo; }
    private:
        Uint32 mXSize = 0;
        Uint32 mYSize = 0;

        void ReadCameraMap(IStream& stream);

        void ReadCollisionItems(IStream& stream, Uint32 numberOfCollisionItems);
        void ReadMapObjects(IStream& stream, Uint32 objectIndexTableOffset);

        std::vector<Camera> mCameras;

        std::vector<CollisionItem> mCollisionItems;
        bool mIsAo = false;
    };
}