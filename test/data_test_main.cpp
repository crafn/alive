#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/bits_factory.hpp"
#include "oddlib/ao_bits_pc.hpp"
#include "oddlib/ae_bits_pc.hpp"
#include "oddlib/psx_bits.hpp"
#include "oddlib/anim.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include "stdthread.h"
#include "msvc_sdl_link.hpp"
#include "gamedata.hpp"

class LvlFileReducer
{
public:
    LvlFileReducer(const LvlFileReducer&) = delete;
    LvlFileReducer& operator = (const LvlFileReducer&) = delete;
    LvlFileReducer(const std::string& resourcePath, const std::vector<std::string>& lvlFiles, std::vector<std::string> fileFilter = std::vector<std::string>())
        : mLvlFiles(lvlFiles), mFileFilter(fileFilter)
    {
        mFs.Init();

        // Clear out the ones loaded from resource paths json
        mFs.ResourcePaths().ClearAllResourcePaths();

        mGameData.Init(mFs);

        // Add the only one we're interested in
        mFs.ResourcePaths().AddResourcePath(resourcePath, 1);

        for (const auto& lvl : mLvlFiles)
        {
            auto stream = mFs.ResourcePaths().Open(lvl);
            if (stream)
            {
                Reduce(std::make_unique<Oddlib::LvlArchive>(std::move(stream)));
            }
            else
            {
                LOG_WARNING("LVL not found: " << lvl);
            }
        }
    }

    const std::vector<std::pair<Oddlib::LvlArchive::FileChunk*, std::pair<std::string, bool>>>& Chunks() const
    {
        return mChunks;
    }

private:
    void Reduce(std::unique_ptr<Oddlib::LvlArchive> lvl)
    {
        bool foundCam = false;
        bool isPsx = false;
        for (auto i = 0u; i < lvl->FileCount(); i++)
        {
            auto file = lvl->FileByIndex(i);
            {
                if (string_util::ends_with(file->FileName(), ".CAM", true))
                {
                    auto stream = file->ChunkByType(Oddlib::MakeType('B', 'i', 't', 's'))->Stream();
                    isPsx = Oddlib::IsPsxCamera(*stream);
                    foundCam = true;
                    break;
                }
            }
        }
        if (!foundCam)
        {
            abort();
        }

        bool chunkTaken = false;
        for (auto i = 0u; i < lvl->FileCount(); i++)
        {
            auto file = lvl->FileByIndex(i);

            bool bUseFile = true;
            if (!mFileFilter.empty())
            {
                bUseFile = false;
                for (const auto& includedFile : mFileFilter)
                {
                    if (includedFile == file->FileName())
                    {
                        bUseFile = true;
                        break;
                    }
                }
            }

            if (bUseFile)
            {
                for (auto j = 0u; j < file->ChunkCount(); j++)
                {
                    auto chunk = file->ChunkByIndex(j);
                  //  if (chunk->Id() == 360)
                    {
                        if (!ChunkExists(*chunk))
                        {
                            AddChunk(chunk, file->FileName(), isPsx);
                            chunkTaken = true;
                        }
                    }
                }
            }
        }
        if (chunkTaken)
        {
            AddLvl(std::move(lvl));
        }
    }

    bool ChunkExists(Oddlib::LvlArchive::FileChunk& chunkToCheck)
    {
        for (auto& chunk : mChunks)
        {
            if (*chunk.first == chunkToCheck)
            {
                return true;
            }
        }
        return false;
    }

    void AddLvl(std::unique_ptr<Oddlib::LvlArchive> lvl)
    {
        mLvls.emplace_back(std::move(lvl));
    }

    void AddChunk(Oddlib::LvlArchive::FileChunk* chunk, const std::string& fileName, bool isPsx)
    {
        mChunks.push_back(std::make_pair(chunk, std::make_pair(fileName, isPsx)));
    }

    const std::vector<std::string>& mLvlFiles;
    std::vector<std::string> mFileFilter;

    GameData mGameData;
    FileSystem mFs;

    std::vector<std::pair<Oddlib::LvlArchive::FileChunk*, std::pair<std::string, bool>>> mChunks;
    std::vector<std::unique_ptr<Oddlib::LvlArchive>> mLvls;
};

class DataTest
{
public:
    enum eDataType
    {
        eAoPc,
        eAoPcDemo,
        eAoPsx,
        eAoPsxDemo,
        eAePc,
        eAePcDemo,
        eAePsx,
        eAePsxDemo
    };

    static const char* ToString(eDataType type)
    {
        switch (type)
        {
        case eAoPc:
            return "AoPc";

        case eAoPcDemo:
            return "AoPcDemo";

        case eAoPsx:
            return "AoPsx";

        case eAoPsxDemo:
            return "AoPsxDemo";

        case eAePc:
            return "AePc";

        case eAePcDemo:
            return "AePcDemo";

        case eAePsx:
            return "AePsx";

        case eAePsxDemo:
            return "AePsxDemo";
        }
        return "unknown";
    }

    DataTest(eDataType eType, const std::string& resourcePath, const std::vector<std::string>& lvls, const std::vector<std::string>& fileFilter)
        : mType(eType), mReducer(resourcePath, lvls, fileFilter)
    {
        ReadAllAnimations();
        //ReadFg1s();
        //ReadFonts();
        //ReadAllPaths();
        //ReadAllCameras();
        // TODO: Handle sounds/fmvs
    }

    void ForChunksOfType(Uint32 type, std::function<void(const std::string&, Oddlib::LvlArchive::FileChunk&, bool)> cb)
    {
        for (auto chunkPair : mReducer.Chunks())
        {
            auto chunk = chunkPair.first;
            auto fileName = chunkPair.second.first;
            bool isPsx = chunkPair.second.second;
            if (chunk->Type() == type)
            {
                cb(fileName, *chunk, isPsx);
            }
        }
    }

    void ReadFg1s()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'G', '1', ' '), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: FG1 parsing
        });
    }

    void ReadFonts()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'o', 'n', 't'), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: Font parsing
        });
    }

    void ReadAllPaths()
    {
        ForChunksOfType(Oddlib::MakeType('P', 'a', 't', 'h'), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: Load the game data json for the required hard coded data to load the path
            /*
            Oddlib::Path path(*chunk.Stream(),
                entry->mCollisionDataOffset,
                entry->mObjectIndexTableOffset,
                entry->mObjectDataOffset,
                entry->mMapXSize,
                entry->mMapYSize);*/
        });
    }

    void ReadAllCameras()
    {
        ForChunksOfType(Oddlib::MakeType('B', 'i', 't', 's'), [&](const std::string&, Oddlib::LvlArchive::FileChunk& chunk, bool )
        {
            auto bits = Oddlib::MakeBits(*chunk.Stream());
            Oddlib::IBits* ptr = nullptr;
            switch (mType)
            {
            case eAoPc:
                ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                break;

            case eAePc:
                ptr = dynamic_cast<Oddlib::AeBitsPc*>(bits.get());
                break;

            case eAePsxDemo:
            case eAePsx:
            {
                auto tmp = dynamic_cast<Oddlib::PsxBits*>(bits.get());
                if (tmp && tmp->IncludeLength())
                {
                    ptr = tmp;
                }
            }
                break;

            case eAoPsxDemo:
            case eAoPsx:
            {
                auto tmp = dynamic_cast<Oddlib::PsxBits*>(bits.get());
                if (tmp && !tmp->IncludeLength())
                {
                    ptr = tmp;
                }
            }
                break;

            case eAoPcDemo:
                ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                break;

            case eAePcDemo:
                ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                if (!ptr)
                {
                    // Very strange, AE PC demo contains "half" cameras of random old PSX
                    // cameras, even half of the anti piracy screen is in here
                    auto tmp = dynamic_cast<Oddlib::PsxBits*>(bits.get());
                    if (tmp && tmp->IncludeLength())
                    {
                        ptr = tmp;
                    }
                }
                break;

            default:
                abort();
            }

            if (!ptr)
            {
                LOG_ERROR("Wrong camera type constructed");
                abort();
            }
            else
            {
                ptr->Save();
            }

        });
    }

    void ReadAllAnimations()
    {
        ForChunksOfType(Oddlib::MakeType('A', 'n', 'i', 'm'), [&](const std::string& fileName, Oddlib::LvlArchive::FileChunk& chunk, bool isPsx)
        {
            if (chunk.Id() == 0x1770)
            {
                // ELECWALL.BAN
             //   abort();
            }
            Oddlib::DebugDumpAnimationFrames(fileName, chunk.Id(), *chunk.Stream(), isPsx, ToString(mType));
        });
    }

private:
    eDataType mType;
    LvlFileReducer mReducer;
};


int main(int /*argc*/, char** /*argv*/)
{
    const std::vector<std::string> aoLvls =
    {
        "s1.lvl",
        "r6.lvl",
        "r2.lvl",
        "r1.lvl",
        "l1.lvl",
        "f4.lvl",
        "f2.lvl",
        "f1.lvl",
        "e2.lvl",
        "e1.lvl",
        "d7.lvl",
        "d2.lvl",
        "c1.lvl"
    };

    const std::vector<std::string> aoDemoLvls =
    {
        "c1.lvl",
        "r1.lvl",
        "s1.lvl"
    };

    const std::vector<std::string> aoDemoPsxLvls =
    {
        "ABESODSE\\C1.LVL",
        "ABESODSE\\E1.LVL",
        "ABESODSE\\R1.LVL",
        "ABESODSE\\S1.LVL"
    };

    const std::vector<std::string> aeLvls =
    {
        "mi.lvl",
        "ba.lvl",
        "bm.lvl",
        "br.lvl",
        "bw.lvl",
        "cr.lvl",
        "fd.lvl",
        "ne.lvl",
        "pv.lvl",
        "st.lvl",
        "sv.lvl"
    };

    const std::vector<std::string> aeDemoLvls =
    {
        "cr.lvl",
        "mi.lvl",
        "st.lvl"
    };

    const std::vector<std::string> aeDemoPsxLvls =
    {
        "ABE2\\CR.LVL",
        "ABE2\\MI.LVL",
        "ABE2\\ST.LVL"
    };

    const std::map<DataTest::eDataType, const std::vector<std::string>*> DataTypeLvlMap =
    {
        { DataTest::eAoPc, &aoLvls },
        { DataTest::eAoPsx, &aoLvls },
        { DataTest::eAePc, &aeLvls },
        { DataTest::eAePsx, &aeLvls },
        { DataTest::eAoPcDemo, &aoDemoLvls },
        { DataTest::eAoPsxDemo, &aoDemoPsxLvls },
        { DataTest::eAePcDemo, &aeDemoLvls },
        { DataTest::eAePsxDemo, &aeDemoPsxLvls }
    };

    const std::vector<std::pair<DataTest::eDataType, std::string>> datas =
    {
      //  { DataTest::eAePsxDemo, "C:\\Users\\paul\\Desktop\\alive\\AE_RE\\testing.bin" },

        { DataTest::eAePc, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Exoddus" },
        { DataTest::eAePcDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\exoddemo" },
        { DataTest::eAePsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Euro Demo 38 (E) (Track 1) [SCED-01148].bin" },
        { DataTest::eAePsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin" },
        { DataTest::eAePsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin" },
        { DataTest::eAoPc, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Oddysee" },
        { DataTest::eAoPcDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\abeodd" },
        { DataTest::eAoPsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (E) [SLES-00664].bin" },
        { DataTest::eAoPsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (Demo) (E) [SLED-00725].bin" },
    };

    std::vector<std::string> fileFilter;
   // fileFilter.push_back("OMMFLARE.BAN");
    //fileFilter.push_back("DUST.BAN");
    //fileFilter.push_back("METAL.BAN");
    //fileFilter.push_back("VAPOR.BAN");
    //fileFilter.push_back("DEBRIS00.BAN");
    //fileFilter.push_back("DRPROCK.BAN");
    //fileFilter.push_back("DRPSPRK.BAN");
   // fileFilter.push_back("EVILFART.BAN");
   // fileFilter.push_back("SHELL.BAN");
   // fileFilter.push_back("SQBSMK.BAN");
   // fileFilter.push_back("WELLLEAF.BAN");
    //fileFilter.push_back("EXPLODE.BND");

    /*
    std::vector<std::unique_ptr<LvlFileReducer>> reducedData;
    for (const auto& data : datas)
    {
        const auto it = DataTypeLvlMap.find(data.first);
        if (it == std::end(DataTypeLvlMap))
        {
            // Defined struct is wrong
            abort();
        }
        reducedData.emplace_back(std::make_unique<LvlFileReducer>(data.second, *it->second, fileFilter));
    }

    std::set<std::string> animsWithoutMatchingIds;
    for (const auto& dataSet : reducedData)
    {
        for (const auto& chunk : dataSet->Chunks())
        {
            if (chunk.first->Type() == Oddlib::MakeType('A', 'n', 'i', 'm'))
            {
                for (const auto& otherData : reducedData)
                {
                    if (otherData != dataSet)
                    {
                        bool exists = false;
                        bool idMatches = false;
                        for (const auto& otherChunk : otherData->Chunks())
                        {
                            if (chunk.first->Type() == otherChunk.first->Type())
                            {
                                if (chunk.second.first == otherChunk.second.first)
                                {
                                    exists = true;
                                    if (chunk.first->Id() == otherChunk.first->Id())
                                    {
                                        idMatches = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (exists && !idMatches)
                        {
                            animsWithoutMatchingIds.insert(chunk.second.first + "_" + std::to_string(chunk.first->Id()));
                            //std::cout << chunk.second.first.c_str() << std::endl;
                        }
                    }
                }
            }
        }
    }
    */

    /*
        D2LIFT.BND_1001
        DOOR.BAN_2012 AE "mine" door
        DOOR.BAN_8001 AO "intro" door
        GLUKKON.BND_801 "flying head" from explosion - not sure why this ends up here??
        PARAMITE.BND_2034 ??
        SLIG.BND_319
        SLIG.BND_360 moved into SHELL.BAN?
        STARTANM.BND_132 moved into ABESPEK2.BAN?
        STARTANM.BND_8002
    */
    
    /*
    for (const auto& item : animsWithoutMatchingIds)
    {
        std::cout << item.c_str() << std::endl;
    }
    */

    // FALLROCK.BAN corrupted frames in AePcDemo
    // ^ TODO: Where is this used in the real game, can't find it? Probably bad in src data

    // ABESPEAK.BAN AE PSX, AE PSX DEMO 1 bad frame
    // ^ Bad in the src data, real engine displays it the same.

    // ABEHOIST.BAN, ABEWELM.BAN ANEBASIC.BAN ANEDSMNT.BAN ANEEDGE.BAN D1HIVE.BAN AO PSX, AO PSX DEMO, blue pixels
    // ^ fixed via forcing these to black, changes some pixels in unrelated anims near the edges so can't really be noticed
    
    // ABEKNBK.BAN all psx ae, some pixels too far right? maybe a couple on ao pc too
    // ABEWASP.BAN ao/AePsxDemo has right aligned pixels?
    // ^ Some problem with Type 7 sprites, always adds some junk pixels near the end?

    // Type 2 in psx just has black frames, has overrun issue?

    //fileFilter.push_back("ABECAR.BAN");
 //   fileFilter.push_back("ABEWASP.BAN");
   // fileFilter.push_back("FALLROCK.BAN");
   // fileFilter.push_back("SPARKS.BAN");
   // fileFilter.push_back("ABEBLOW.BAN");

    for (const auto& data : datas)
    {
        const auto it = DataTypeLvlMap.find(data.first);
        if (it == std::end(DataTypeLvlMap))
        {
            // Defined struct is wrong
            abort();
        }
        DataTest dataDecoder(data.first, data.second, *it->second, fileFilter);
    }

    return 0;
}
