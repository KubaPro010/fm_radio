#pragma once
#include <stdint.h>
#include <string>
#include <array>
#include <chrono>
#include <mutex>

#define RDS_BLOCK_A_TIMEOUT_MS  15000.0
#define RDS_BLOCK_B_TIMEOUT_MS  4000.0
#define RDS_GROUP_0_TIMEOUT_MS  6000.0
#define RDS_GROUP_2_TIMEOUT_MS  6500.0
#define RDS_GROUP_10_TIMEOUT_MS 7500.0
#define RDS_GROUP_15_TIMEOUT_MS 8000.0
#define RDS_ECC_TIMEOUT_MS  15000.0

namespace rds {
    enum BlockType {
        BLOCK_TYPE_A,
        BLOCK_TYPE_B,
        BLOCK_TYPE_C,
        BLOCK_TYPE_CP,
        BLOCK_TYPE_D,
        _BLOCK_TYPE_COUNT
    };

    enum GroupVersion {
        GROUP_VER_A,
        GROUP_VER_B
    };

    enum AreaCoverage {
        AREA_COVERAGE_INVALID           = -1,
        AREA_COVERAGE_LOCAL             = 0,
        AREA_COVERAGE_INTERNATIONAL     = 1,
        AREA_COVERAGE_NATIONAL          = 2,
        AREA_COVERAGE_SUPRA_NATIONAL    = 3,
        AREA_COVERAGE_REGIONAL1         = 4,
        AREA_COVERAGE_REGIONAL2         = 5,
        AREA_COVERAGE_REGIONAL3         = 6,
        AREA_COVERAGE_REGIONAL4         = 7,
        AREA_COVERAGE_REGIONAL5         = 8,
        AREA_COVERAGE_REGIONAL6         = 9,
        AREA_COVERAGE_REGIONAL7         = 10,
        AREA_COVERAGE_REGIONAL8         = 11,
        AREA_COVERAGE_REGIONAL9         = 12,
        AREA_COVERAGE_REGIONAL10        = 13,
        AREA_COVERAGE_REGIONAL11        = 14,
        AREA_COVERAGE_REGIONAL12        = 15
    };

    // enum AlternativeFrequencySpecialCodes {
    //     ALTERNATIVE_FREQUENCY_SPECIAL_CODES_FILLER = 205,
    //     ALTERNATIVE_FREQUENCY_SPECIAL_CODES_NO_AF = 224,
    //     ALTERNATIVE_FREQUENCY_SPECIAL_CODES_AF_COUNT_BASE = 224,
    //     ALTERNATIVE_FREQUENCY_SPECIAL_CODES_LFMF_FOLLOWS = 250,
    // };

    inline const char* AREA_COVERAGE_TO_STR[] = {
        "Local",
        "International",
        "National",
        "Supra-National",
        "Regional 1",
        "Regional 2",
        "Regional 3",
        "Regional 4",
        "Regional 5",
        "Regional 6",
        "Regional 7",
        "Regional 8",
        "Regional 9",
        "Regional 10",
        "Regional 11",
        "Regional 12",
    };

    enum ProgramType {
        // US Types
        PROGRAM_TYPE_US_NONE = 0,
        PROGRAM_TYPE_US_NEWS = 1,
        PROGRAM_TYPE_US_INFOMATION = 2,
        PROGRAM_TYPE_US_SPORTS = 3,
        PROGRAM_TYPE_US_TALK = 4,
        PROGRAM_TYPE_US_ROCK = 5,
        PROGRAM_TYPE_US_CLASSIC_ROCK = 6,
        PROGRAM_TYPE_US_ADULT_HITS = 7,
        PROGRAM_TYPE_US_SOFT_ROCK = 8,
        PROGRAM_TYPE_US_TOP_40 = 9,
        PROGRAM_TYPE_US_COUNTRY = 10,
        PROGRAM_TYPE_US_OLDIES = 11,
        PROGRAM_TYPE_US_SOFT = 12,
        PROGRAM_TYPE_US_NOSTALGIA = 13,
        PROGRAM_TYPE_US_JAZZ = 14,
        PROGRAM_TYPE_US_CLASSICAL = 15,
        PROGRAM_TYPE_US_RHYTHM_AND_BLUES = 16,
        PROGRAM_TYPE_US_SOFT_RHYTHM_AND_BLUES = 17,
        PROGRAM_TYPE_US_FOREIGN_LANGUAGE = 18,
        PROGRAM_TYPE_US_RELIGIOUS_MUSIC = 19,
        PROGRAM_TYPE_US_RELIGIOUS_TALK = 20,
        PROGRAM_TYPE_US_PERSONALITY = 21,
        PROGRAM_TYPE_US_PUBLIC = 22,
        PROGRAM_TYPE_US_COLLEGE = 23,
        PROGRAM_TYPE_US_UNASSIGNED0 = 24,
        PROGRAM_TYPE_US_UNASSIGNED1 = 25,
        PROGRAM_TYPE_US_UNASSIGNED2 = 26,
        PROGRAM_TYPE_US_UNASSIGNED3 = 27,
        PROGRAM_TYPE_US_UNASSIGNED4 = 28,
        PROGRAM_TYPE_US_WEATHER = 29,
        PROGRAM_TYPE_US_EMERGENCY_TEST = 30,
        PROGRAM_TYPE_US_EMERGENCY = 31,

        // EU Types
        PROGRAM_TYPE_EU_NONE = 0,
        PROGRAM_TYPE_EU_NEWS = 1,
        PROGRAM_TYPE_EU_CURRENT_AFFAIRS = 2,
        PROGRAM_TYPE_EU_INFORMATION = 3,
        PROGRAM_TYPE_EU_SPORTS = 4,
        PROGRAM_TYPE_EU_EDUCATION = 5,
        PROGRAM_TYPE_EU_DRAMA = 6,
        PROGRAM_TYPE_EU_CULTURE = 7,
        PROGRAM_TYPE_EU_SCIENCE = 8,
        PROGRAM_TYPE_EU_VARIED = 9,
        PROGRAM_TYPE_EU_POP_MUSIC = 10,
        PROGRAM_TYPE_EU_ROCK_MUSIC = 11,
        PROGRAM_TYPE_EU_EASY_LISTENING_MUSIC = 12,
        PROGRAM_TYPE_EU_LIGHT_CLASSICAL = 13,
        PROGRAM_TYPE_EU_SERIOUS_CLASSICAL = 14,
        PROGRAM_TYPE_EU_OTHER_MUSIC = 15,
        PROGRAM_TYPE_EU_WEATHER = 16,
        PROGRAM_TYPE_EU_FINANCE = 17,
        PROGRAM_TYPE_EU_CHILDRENS_PROGRAM = 18,
        PROGRAM_TYPE_EU_SOCIAL_AFFAIRS = 19,
        PROGRAM_TYPE_EU_RELIGION = 20,
        PROGRAM_TYPE_EU_PHONE_IN = 21,
        PROGRAM_TYPE_EU_TRAVEL = 22,
        PROGRAM_TYPE_EU_LEISURE = 23,
        PROGRAM_TYPE_EU_JAZZ_MUSIC = 24,
        PROGRAM_TYPE_EU_COUNTRY_MUSIC = 25,
        PROGRAM_TYPE_EU_NATIONAL_MUSIC = 26,
        PROGRAM_TYPE_EU_OLDIES_MUSIC = 27,
        PROGRAM_TYPE_EU_FOLK_MUSIC = 28,
        PROGRAM_TYPE_EU_DOCUMENTARY = 29,
        PROGRAM_TYPE_EU_ALARM_TEST = 30,
        PROGRAM_TYPE_EU_ALARM = 31
    };

    inline const char* PROGRAM_TYPE_EU_TO_STR[] = {
        "None",
        "News",
        "Current Affairs",
        "Information",
        "Sports",
        "Education",
        "Drama",
        "Culture",
        "Science",
        "Varied",
        "Pop Music",
        "Rock Music",
        "Easy Listening Music",
        "Light Classical",
        "Serious Classical",
        "Other Music",
        "Weather",
        "Finance",
        "Children Program",
        "Social Affairs",
        "Religion",
        "Phone-in",
        "Travel",
        "Leisure",
        "Jazz Music",
        "Country Music",
        "National Music",
        "Oldies Music",
        "Folk Music",
        "Documentary",
        "Alarm Test",
        "Alarm",
    };

    inline const char* PROGRAM_TYPE_US_TO_STR[] = {
        "None",
        "News",
        "Information",
        "Sports",
        "Talk",
        "Rock",
        "Classic Rock",
        "Adult Hits",
        "Soft Rock",
        "Top 40",
        "Country",
        "Oldies",
        "Soft",
        "Nostalgia",
        "Jazz",
        "Classical",
        "Rythm and Blues",
        "Soft Rythm and Blues",
        "Foreign Language",
        "Religious Music",
        "Religious Talk",
        "Personality",
        "Public",
        "College",
        "Unassigned",
        "Unassigned",
        "Unassigned",
        "Unassigned",
        "Unassigned",
        "Weather",
        "Emergency Test",
        "Emergency",
    };

    enum DecoderIdentification {
        DECODER_IDENT_STEREO = (1 << 0),
        DECODER_IDENT_ARTIFICIAL_HEAD = (1 << 1),
        DECODER_IDENT_COMPRESSED = (1 << 2),
        DECODER_IDENT_DYNAMIC_PTY = (1 << 3)
    };

    class Decoder {
    public:
        unsigned int getMJDDay(double mjd);
        unsigned int getMJDMonth(double mjd);
        unsigned int getMJDYear(double mjd);

        void process(uint8_t* symbols, int count);

        bool piCodeValid() { std::lock_guard<std::mutex> lck(blockAMtx); return blockAValid(); }
        uint16_t getPICode() { std::lock_guard<std::mutex> lck(blockAMtx); return piCode; }
        uint8_t getProgramCoverage() { std::lock_guard<std::mutex> lck(blockAMtx); return programCoverage; }
        std::string getCallsign() { std::lock_guard<std::mutex> lck(blockAMtx); return callsign; }

        bool programTypeValid() { std::lock_guard<std::mutex> lck(blockBMtx); return blockBValid(); }
        ProgramType getProgramType() { std::lock_guard<std::mutex> lck(blockBMtx); return programType; }

        double getClockMJD() { std::lock_guard<std::mutex> lck(group4AMtx) ;return clock_mjd; }
        uint8_t getClockHour() { std::lock_guard<std::mutex> lck(group4AMtx); return clock_hour; }
        uint8_t getClockMinute() { std::lock_guard<std::mutex> lck(group4AMtx); return clock_minute; }
        uint8_t getClockOffset() { std::lock_guard<std::mutex> lck(group4AMtx); return clock_offset; }
        bool getClockOffsetSense() { std::lock_guard<std::mutex> lck(group4AMtx); return clock_offset_sense; }

        bool CTReceived() { std::lock_guard<std::mutex> lck(group4AMtx); return getMJDMonth(clock_mjd) != 0; }

        bool LPSNameValid() { std::lock_guard<std::mutex> lck(group15AMtx); return group15Valid(); }
        std::string getLPSName() { std::lock_guard<std::mutex> lck(group15AMtx); return longPS; }

        bool eccValid();
        uint16_t getEcc() { std::lock_guard<std::mutex> lck(group1Mtx); return ecc; }

        bool tpValid() { std::lock_guard<std::mutex> lck(group0Mtx); return group0Valid(); }
        bool getTp() { std::lock_guard<std::mutex> lck(group0Mtx); return trafficProgram; }

        bool taValid() { std::lock_guard<std::mutex> lck(group0Mtx); return group0Valid(); }
        bool getTa() { std::lock_guard<std::mutex> lck(group0Mtx); return trafficAnnouncement; }

        bool diValid() { std::lock_guard<std::mutex> lck(group0Mtx); return group0Valid(); }
        uint8_t getDi() { std::lock_guard<std::mutex> lck(group0Mtx); return decoderIdent; }

        bool musicValid() { std::lock_guard<std::mutex> lck(group0Mtx); return group0Valid(); }

        bool afValid() { std::lock_guard<std::mutex> lck(group0Mtx); return group0Valid() && afCount != 0; }
        uint8_t getAFCount() { std::lock_guard<std::mutex> lck(group0Mtx); return afCount; }
        std::array<uint32_t, 25> getAFs() { std::lock_guard<std::mutex> lck(group0Mtx); return afs; }

        bool PSNameValid() { std::lock_guard<std::mutex> lck(group0Mtx); return group0Valid(); }
        std::string getPSName() { std::lock_guard<std::mutex> lck(group0Mtx); return ps; }

        bool radioTextValid() { std::lock_guard<std::mutex> lck(group2Mtx); return group2Valid(); }
        std::string getRadioText() { std::lock_guard<std::mutex> lck(group2Mtx); return radioText; }
        std::string getRadioTextAB() { std::lock_guard<std::mutex> lck(group2Mtx); return lastRTAB ? (std::string)"B" : (std::string)"A"; }

        bool programTypeNameValid() { std::lock_guard<std::mutex> lck(group10Mtx); return group10Valid(); }
        std::string getProgramTypeName() { std::lock_guard<std::mutex> lck(group10Mtx); return programTypeName; }

        void reset();
    private:
        static uint16_t calcSyndrome(uint32_t block);
        static uint32_t correctErrors(uint32_t block, BlockType type, bool& recovered);
        void decodeBlockA();
        void decodeBlockB();
        void decodeGroup0();
        void decodeGroup2();
        void decodeGroup10A();
        void decodeGroup1();
        void decodeGroup15A();
        void decodeGroup15B();
        void decodeGroup4A();
        void decodeGroup();

        void decodeAlternativeFrequencies();

        static std::string base26ToCall(uint16_t pi);
        static std::string decodeCallsign(uint16_t pi);

        bool blockAValid();
        bool blockBValid();
        bool group0Valid();
        bool group2Valid();
        bool group10Valid();
        bool group15Valid();

        // State machine
        uint32_t shiftReg = 0;
        int sync = 0;
        int skip = 0;
        BlockType lastType = BLOCK_TYPE_A;
        int contGroup = 0;
        uint32_t blocks[_BLOCK_TYPE_COUNT];
        bool blockAvail[_BLOCK_TYPE_COUNT];

        // Block A (All groups)
        std::mutex blockAMtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> blockALastUpdate{};  // 1970-01-01
        uint16_t piCode;
        AreaCoverage programCoverage;
        std::string callsign;

        // Block B (All groups)
        std::mutex blockBMtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> blockBLastUpdate{};  // 1970-01-01
        uint8_t groupType;
        GroupVersion groupVer;
        bool trafficProgram;
        ProgramType programType;

        // Group type 0
        std::mutex group0Mtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> group0LastUpdate{};  // 1970-01-01
        bool trafficAnnouncement;
        uint8_t decoderIdent;
        uint16_t alternativeFrequency;        
        std::string ps = "        ";

        std::array<uint32_t, 25> afs;
        uint8_t afCount;
        uint8_t afState;
        uint8_t afLfMfIncoming;

        // Group type 2
        std::mutex group2Mtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> group2LastUpdate{};  // 1970-01-01
        bool lastRTAB = false;
        std::string radioText = "                                                                ";

        // Group type 10
        std::mutex group10Mtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> group10LastUpdate{};  // 1970-01-01
        bool lastPTYNAB = false;
        std::string programTypeName = "        ";

        // Group type 1
        std::mutex group1Mtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> group1LastUpdate{};  // 1970-01-01
        std::chrono::time_point<std::chrono::high_resolution_clock> eccLastUpdate{};  // 1970-01-01
        uint8_t ecc = 0;

        // Group type 15A
        std::mutex group15AMtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> group15ALastUpdate{};  // 1970-01-01
        std::string longPS = "                                ";

        // Group type 15B
        std::mutex group15BMtx;
        std::chrono::time_point<std::chrono::high_resolution_clock> group15BLastUpdate{};  // 1970-01-01

        // Group type 4A
        std::mutex group4AMtx;
        uint8_t clock_hour = 0;
        uint8_t clock_minute = 0;
        bool clock_offset_sense = false;
        uint8_t clock_offset = 0;
        double clock_mjd = 0;
    };
}