#ifndef CODEPAGE_H
#define CODEPAGE_H
#include <tvision/tv.h>
#include <string_view>
#include <unordered_map>
#include <array>

// tables.cpp

class CpTranslator {

    CpTranslator();
    static CpTranslator instance;

    struct CpTable {
        std::string_view cp;
        const uint32_t *toUtf8Int;
        const std::unordered_map<std::string_view, char> fromUtf8;

        static auto initMap(const std::string_view toUtf8[256])
        {
            std::unordered_map<std::string_view, char> map;
            for (size_t i = 0; i < 256; ++i)
                map.emplace(toUtf8[i], char(i));
            return map;
        }

        CpTable( std::string_view cp,
                 const std::string_view toUtf8[256],
                 const std::array<uint32_t, 256> &toUtf8Int ) :
            cp(cp),
            toUtf8Int(toUtf8Int.data()),
            fromUtf8(initMap(toUtf8))
        {
        }
    };
    
    static const CpTable tables[2];
    static const CpTable *activeTable;

public:

    static void use(std::string_view cp) {
        for (const CpTable &t : tables)
            if (t.cp == cp) {
                activeTable = &t;
                return;
            }
        activeTable = &tables[0];
    }

    static TCellChar toUtf8Int(unsigned char c) {
        return activeTable->toUtf8Int[c];
    }

    static char fromUtf8(TStringView s) {
        auto it = activeTable->fromUtf8.find(s);
        if (it != activeTable->fromUtf8.end())
            return it->second;
        return 0;
    }

};

#endif
