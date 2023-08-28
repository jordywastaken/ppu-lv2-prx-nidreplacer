#pragma once
#include <stdint.h>

enum ElfType : uint16_t
{
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4,

	ET_SCE_EXEC = 0xFE00,
	ET_SCE_RELEXEC = 0xFE04,
	ET_SCE_STUBLIB = 0xFE0C,
	ET_SCE_DYNEXEC = 0xFE10,
	ET_SCE_DYNAMIC = 0xFE18,
	ET_SCE_IOPRELEXEC = 0xFF80,
	ET_SCE_IOPRELEXEC2 = 0xFF81,
	ET_SCE_EERELEXEC = 0xFF90,
	ET_SCE_EERELEXEC2 = 0xFF91,
	ET_SCE_PSPRELEXEC = 0xFFA0,
	ET_SCE_PPURELEXEC = 0xFFA4,
	ET_SCE_ARMRELEXEC = 0xFFA5,
	ET_SCE_PSPOVERLAY = 0xFFA8
};

enum ElfMachine : uint16_t
{
	EM_NONE = 0,
	EM_PPC = 20,
	EM_PPC64 = 21,
	EM_SPU = 23
};

enum SectionHeaderType : uint32_t
{
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
    SHT_NOBITS = 8,
    SHT_REL = 9,

    SHT_SCE_RELA = 0x60000000,
    SHT_SCE_NID = 0x61000001,
    SHT_SCE_IOPMOD = 0x70000080,
    SHT_SCE_EEMOD = 0x70000090,
    SHT_SCE_PSPRELA = 0x700000A0,
    SHT_SCE_PPURELA = 0x700000A4
};

enum SectionHeaderFlags : uint64_t
{
	SHF_WRITE = (1 << 0),
	SHF_ALLOC = (1 << 1),
	SHF_EXECINSTR = (1 << 2)
};

static constexpr int EI_NIDENT = 16;

struct ElfHeader
{
	uint8_t e_ident[EI_NIDENT];       /* ELF identification */
	uint16_t e_type;                  /* object file type */
	uint16_t e_machine;               /* machine type */
	uint32_t e_version;               /* object file version */
	uint64_t e_entry;                 /* entry point address */
	uint64_t e_phoff;                 /* program header offset */
	uint64_t e_shoff;                 /* section header offset */
	uint32_t e_flags;                 /* processor-specific flags */
	uint16_t e_ehsize;                /* ELF header size */
	uint16_t e_phentsize;             /* size of program header entry */
	uint16_t e_phnum;                 /* number of program header entries */
	uint16_t e_shentsize;             /* size of section header entry */
	uint16_t e_shnum;                 /* number of section header entries */
	uint16_t e_shstrndx;              /* section name string table index */
};

struct ProgramHeader
{
	uint32_t p_type;			      /* Segment type */
	uint32_t p_flags;		          /* Segment flags */
	uint64_t p_offset;		          /* Segment file offset */
	uint64_t p_vaddr;		          /* Segment virtual address */
	uint64_t p_paddr;		          /* Segment physical address */
	uint64_t p_filesz;		          /* Segment size in file */
	uint64_t p_memsz;		          /* Segment size in memory */
	uint64_t p_align;				  /* Segment alignment */
};

struct SectionHeader
{
	uint32_t sh_name;                 /* section name */
	uint32_t sh_type;                 /* section type */
	uint64_t sh_flags;                /* section attributes */
	uint64_t sh_addr;                 /* virtual address in memory */
	uint64_t sh_offset;               /* offset in file */
	uint64_t sh_size;                 /* size of section */
	uint32_t sh_link;                 /* link to other section */
	uint32_t sh_info;                 /* miscellaneous information */
	uint64_t sh_addralign;            /* address alignment boundary */
	uint64_t sh_entsize;              /* size of entries, if section has table */
};