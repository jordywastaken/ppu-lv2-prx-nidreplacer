#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <vector>
#include <string>

#include "elf.h"

template <typename T>
static inline T SwapEndian(T u)
{
    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (int k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

char* LoadFile(const char* path, size_t* byteRead)
{
    int v6;
    void* v7;
    size_t v8;

    FILE* fd = fopen(path, "rb");

    if (fd)
    {
        fseek(fd, 0, SEEK_END);
        v6 = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        v7 = calloc(1, v6 + 2);
        v8 = fread(v7, 1, v6, fd);
        fclose(fd);

        if (v8 == v6)
        {
            *byteRead = v8;
            return (char*)v7;
        }
        else
        {
            printf("\"%s\" can't read\n", path);
            *byteRead = 0;
            return 0;
        }
    }

    printf("\"%s\" can't open\n", path);
    return 0;
}

void WriteFile(const char* path, void* data, size_t dataLen, size_t offset)
{
    FILE* fd = fopen(path, "r+");

    if (fd)
    {
        fseek(fd, (long)offset, SEEK_SET);
        fwrite(data, 1, dataLen, fd);
        fclose(fd);
        return;
    }

    printf("\"%s\" can't open\n", path);
}

namespace arlib
{
    struct MemberHeader
    {
        char start[16];
        char value[12];
        char value2[6];
        char value3[6];
        char value4[8];
        char archiveMemberSize[10];
        char end[2];

        size_t GetDataSize()
        {
            return strtoul(archiveMemberSize, 0, 10);
        }
    };

    struct Member
    {
        std::string obj;
        size_t objSize;
        size_t fileOffset;
    };

    Member s_firstEntry;
    Member s_secondEntry;
    std::vector<Member> s_archiveData;

    bool ParseArchive(char* buffer, size_t bufferLen)
    {
        if (bufferLen < 8)
            return false;

        if (strncmp(buffer, "!<arch>\n", 8))
            return false;

        char* bufferEnd = buffer + bufferLen;
        char* bufferPos = buffer + 8;
        MemberHeader* currentHeader = reinterpret_cast<MemberHeader*>(bufferPos);

        // Parse the first file header
        if (!strncmp(currentHeader->start, "/               ", 16))
        {
            if (strncmp(currentHeader->end, "`\n", 2))
            {
                printf("Panic !! internal error in arlib. illegal archive member header\n");
                exit(1);
            }

            char* obj = bufferPos + sizeof(MemberHeader);
            size_t objSize = currentHeader->GetDataSize();
            size_t fileOffset = obj - buffer;

            s_firstEntry.obj.resize(objSize);
            memcpy((void*)s_firstEntry.obj.data(), obj, objSize);
            s_firstEntry.objSize = objSize;
            s_firstEntry.fileOffset = fileOffset;

            bufferPos = obj + (objSize & ~1);
            currentHeader = reinterpret_cast<MemberHeader*>(bufferPos);
        }

        // Parse the second file header
        if (!strncmp(currentHeader->start, "//              ", 16))
        {
            if (strncmp(currentHeader->end, "`\n", 2))
            {
                printf("Panic !! internal error in arlib. illegal archive member header\n");
                exit(1);
            }

            char* obj = bufferPos + sizeof(MemberHeader);
            size_t objSize = currentHeader->GetDataSize();
            size_t fileOffset = obj - buffer;

            s_secondEntry.obj.resize(objSize);
            memcpy((void*)s_secondEntry.obj.data(), obj, objSize);
            s_secondEntry.objSize = objSize;
            s_secondEntry.fileOffset = fileOffset;

            bufferPos = obj + ((objSize + 1) & ~1);
            currentHeader = reinterpret_cast<MemberHeader*>(bufferPos);
        }

        while (bufferPos < bufferEnd - sizeof(MemberHeader))
        {
            char* obj = bufferPos + sizeof(MemberHeader);
            size_t objSize = currentHeader->GetDataSize();
            size_t fileOffset = obj - buffer;

            std::string objCopy;
            objCopy.resize(objSize);
            memcpy((void*)objCopy.data(), obj, objSize);

            s_archiveData.push_back({ objCopy, objSize, fileOffset });

            bufferPos = obj + objSize;
            currentHeader = reinterpret_cast<MemberHeader*>(bufferPos);
        }
        return true;
    }

    bool ParseArchive(const char* path)
    {
        bool ret = false;
        size_t fileSize;
        char* buffer = LoadFile(path, &fileSize);

        if (buffer)
        {
            ret = arlib::ParseArchive(buffer, fileSize);
            free(buffer);
        }
        return ret;
    }

    void ReplaceFnid(std::vector<std::pair<std::string, uint32_t>> fnidList, const char* filePath)
    {
        for (auto& archiveEntry : s_archiveData)
        {
            char* elfBase = (char*)(archiveEntry.obj.data());
            ElfHeader* elf = (ElfHeader*)(elfBase);

            // Make sure it is an elf file
            if (elf->e_ident[0] != 0x7F ||
                elf->e_ident[1] != 'E' ||
                elf->e_ident[2] != 'L' ||
                elf->e_ident[3] != 'F')
                continue;

            SectionHeader* shdr = (SectionHeader*)(elfBase + SwapEndian(elf->e_shoff));
            char* shstr = elfBase + SwapEndian(shdr[SwapEndian(elf->e_shstrndx)].sh_offset);
            char* symbols = 0;
            char* symbolsEnd = 0;

            // Look for symbols section
            for (int i = 0; i < SwapEndian(elf->e_shnum); i++)
            {
                char* sectionName = shstr + SwapEndian(shdr[i].sh_name);
                if (!strncmp(sectionName, ".strtab", 7))
                {
                    symbols = elfBase + SwapEndian(shdr[i].sh_offset);
                    symbolsEnd = symbols + SwapEndian(shdr[i].sh_size);
                    break;
                }
            }

            if (!symbols)
                continue;

            for (auto& fnid : fnidList)
            {
                for (char* pos = symbols; pos < symbolsEnd - fnid.first.size() + 1; pos++)
                {
                    // Current entry has the symbol we're looking for
                    if (!strcmp(pos, fnid.first.c_str()))
                    {
                        printf("Replacing '%s' fnid at 0x%zX\n", fnid.first.c_str(), archiveEntry.fileOffset + 0x6C);

                        uint32_t nid = SwapEndian(fnid.second);
                        WriteFile(filePath, &nid, sizeof(uint32_t), archiveEntry.fileOffset + 0x6C);
                        break;
                    }
                }
            }
        }
        printf("All done!\n");
    }
}

const char* executableName = "";
const char* GetExecutableName(const char* arg)
{
    const char* v1 = strrchr(arg, '/');
    if (v1 || (v1 = strrchr(arg, '\\')) != 0)
    {
        return v1 + 1;
    }
    return arg;
}

std::vector<std::pair<std::string, uint32_t>> ParseFnidList(const char* fnidListPath)
{
    std::vector<std::pair<std::string, uint32_t>> fnidList;

    size_t fileSize;
    char* buffer = LoadFile(fnidListPath, &fileSize);
    char* currentLine = buffer;

    if (buffer && fileSize > 3)
    {
        while (currentLine < buffer + fileSize - 3)
        {
            // Very unsafe code but idc
            char* endOfLine = strchr(currentLine, '\n');
            if (!endOfLine)
                endOfLine = strchr(currentLine, '\0');

            char* nextLine = endOfLine + 1;

            if (endOfLine[-1] == '\r')
                endOfLine--;

            char* fnidStart = currentLine;
            char* symbolStart = strchr(currentLine, ':') + 1;
            size_t symbolLen = endOfLine - symbolStart;

            uint32_t fnid = strtoul(fnidStart, 0, 16);
            std::string symbol;
            symbol.resize(symbolLen);
            strncpy((char*)symbol.data(), symbolStart, symbolLen);

            fnidList.push_back(std::make_pair(symbol, fnid));

            currentLine = nextLine;
        }

        free(buffer);
    }

    return fnidList;
}

int main(int argc, const char* argv[])
{
    const int archive_file = 1;
    const int fnid_list = 2;
    const char* help = "usage: %s <archive_file> <fnid_list>\n";

    executableName = GetExecutableName(argv[0]);

    if (argc != 3)
    {
        printf(help, executableName);
        exit(0);
    }

    arlib::ParseArchive(argv[archive_file]);
    arlib::ReplaceFnid(ParseFnidList(argv[fnid_list]), argv[archive_file]);

    return 0;
}