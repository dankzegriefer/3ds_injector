#include <3ds.h>
#include <string.h>
#include "patcher.h"
#include "fsldr.h"
#include "paths.h"

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

static const char hexDigits[] = "0123456789ABCDEF";

//Quick Search algorithm, adapted from http://igm.univ-mlv.fr/~lecroq/string/node19.html#SECTION00190
static u8 *memsearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize)
{
    const u8 *patternc = (const u8 *)pattern;

    //Preprocessing
    u32 table[256];

    for(u32 i = 0; i < 256; ++i)
        table[i] = patternSize + 1;
    for(u32 i = 0; i < patternSize; ++i)
        table[patternc[i]] = patternSize - i;

    //Searching
    u32 j = 0;

    while(j <= size - patternSize)
    {
        if(memcmp(patternc, startPos + j, patternSize) == 0)
            return startPos + j;
        j += table[startPos[j + patternSize]];
    }

    return NULL;
}

static u32 patch_memory(u8 *start, u32 size, const void *pattern, u32 patSize, int offset, const void *replace, u32 repSize, u32 count)
{
    u32 i;

    for(i = 0; i < count; i++)
    {
        u8 *found = memsearch(start, pattern, size, patSize);

        if(found == NULL) break;

        memcpy(found + offset, replace, repSize);

        u32 at = (u32)(found - start);

        if(at + patSize > size) break;

        size -= at + patSize;
        start = found + patSize;
    }

    return i;
}

static int file_open(Handle *file, FS_ArchiveID id, const char *path, int flags)
{
    FS_Path archive;
    FS_Path fspath;

    archive.type = PATH_EMPTY;
    archive.size = 1;
    archive.data = (u8 *)"";

    fspath.type = PATH_ASCII;
    fspath.data = path;
    fspath.size = strnlen(path, PATH_MAX) + 1;

    Result ret = FSLDR_OpenFileDirectly(file, id, archive, fspath, flags, 0);

    return ret;
}

static int load_title_locale_config(u64 progid, u8 *regionId, u8 *languageId)
{
    /* Here we look for "/injector/locales/[u64 titleID in hex, uppercase].txt"
       If it exists it should contain, for example, "EUR IT" */

    char path[] = LOCALES_PATH;

    u32 i = strlen(path) - 5;

    while(progid > 0)
    {
        path[i--] = hexDigits[(u32)(progid & 0xF)];
        progid >>= 4;
    }

    Handle file;
    Result ret = file_open(&file, ARCHIVE_SDMC, path, FS_OPEN_READ);
    if(R_SUCCEEDED(ret))
    {
        char buf[6];
        u32 total;

        ret = FSFILE_Read(file, &total, 0, buf, 6);
        FSFILE_Close(file);

        if(!R_SUCCEEDED(ret) || total < 6) return -1;

        for(u32 i = 0; i < 7; ++i)
        {
            static const char *regions[] = {"JPN", "USA", "EUR", "AUS", "CHN", "KOR", "TWN"};

            if(memcmp(buf, regions[i], 3) == 0)
            {
                *regionId = (u8)i;
                break;
            }
        }

        for(u32 i = 0; i < 12; ++i)
        {
            static const char *languages[] = {"JP", "EN", "FR", "DE", "IT", "ES", "ZH", "KO", "NL", "PT", "RU", "TW"};

            if(memcmp(buf + 4, languages[i], 2) == 0)
            {
                *languageId = (u8)i;
                break;
            }
        }
    }

    return ret;
}

static u8 *get_cfg_offsets(u8 *code, u32 size, u32 *CFGUHandleOffset)
{
    /* HANS:
       Look for error code which is known to be stored near cfg:u handle
       this way we can find the right candidate
       (handle should also be stored right after end of candidate function) */

    u32 n = 0,
        possible[24];

    for(u8 *pos = code + 4; n < 24 && pos < code + size - 4; pos += 4)
    {
        if(*(u32 *)pos == 0xD8A103F9)
        {
            for(u32 *l = (u32 *)pos - 4; n < 24 && l < (u32 *)pos + 4; l++)
                if(*l <= 0x10000000) possible[n++] = *l;
        }
    }

    for(u8 *CFGU_GetConfigInfoBlk2_endPos = code; CFGU_GetConfigInfoBlk2_endPos < code + size - 8; CFGU_GetConfigInfoBlk2_endPos += 4)
    {
        static const u32 CFGU_GetConfigInfoBlk2_endPattern[] = {0xE8BD8010, 0x00010082};

        //There might be multiple implementations of GetConfigInfoBlk2 but let's search for the one we want
        u32 *cmp = (u32 *)CFGU_GetConfigInfoBlk2_endPos;

        if(cmp[0] == CFGU_GetConfigInfoBlk2_endPattern[0] && cmp[1] == CFGU_GetConfigInfoBlk2_endPattern[1])
        {
            *CFGUHandleOffset = *((u32 *)CFGU_GetConfigInfoBlk2_endPos + 2);

            for(u32 i = 0; i < n; i++)
                if(possible[i] == *CFGUHandleOffset) return CFGU_GetConfigInfoBlk2_endPos;

            CFGU_GetConfigInfoBlk2_endPos += 4;
        }
    }

    return NULL;
}

static void patch_CfgGetLanguage(u8 *code, u32 size, u8 languageId, u8 *CFGU_GetConfigInfoBlk2_endPos)
{
    u8 *CFGU_GetConfigInfoBlk2_startPos; //Let's find STMFD SP (there might be a NOP before, but nevermind)

    for(CFGU_GetConfigInfoBlk2_startPos = CFGU_GetConfigInfoBlk2_endPos - 4;
        CFGU_GetConfigInfoBlk2_startPos >= code && *((u16 *)CFGU_GetConfigInfoBlk2_startPos + 1) != 0xE92D;
        CFGU_GetConfigInfoBlk2_startPos -= 2);

    for(u8 *languageBlkIdPos = code; languageBlkIdPos < code + size; languageBlkIdPos += 4)
    {
        if(*(u32 *)languageBlkIdPos == 0xA0002)
        {
            for(u8 *instr = languageBlkIdPos - 8; instr >= languageBlkIdPos - 0x1008 && instr >= code + 4; instr -= 4) //Should be enough
            {
                if(instr[3] == 0xEB) //We're looking for BL
                {
                    u8 *calledFunction = instr;
                    u32 i = 0,
                        found;

                    do
                    {
                        u32 low24 = (*(u32 *)calledFunction & 0x00FFFFFF) << 2;
                        u32 signMask = (u32)(-(low24 >> 25)) & 0xFC000000; //Sign extension
                        s32 offset = (s32)(low24 | signMask) + 8;          //Branch offset + 8 for prefetch

                        calledFunction += offset;

                        found = calledFunction >= CFGU_GetConfigInfoBlk2_startPos - 4 && calledFunction <= CFGU_GetConfigInfoBlk2_endPos;
                        i++;
                    }
                    while(i < 2 && !found && calledFunction[3] == 0xEA);

                    if(found) 
                    {
                        *((u32 *)instr - 1)  = 0xE3A00000 | languageId; // mov    r0, sp                 => mov r0, =languageId
                        *(u32 *)instr        = 0xE5CD0000;              // bl     CFGU_GetConfigInfoBlk2 => strb r0, [sp]
                        *((u32 *)instr + 1)  = 0xE3B00000;              // (1 or 2 instructions)         => movs r0, 0             (result code)

                        //We're done
                        return;
                    }
                }
            }
        }
    }
}

static void patch_CfgGetRegion(u8 *code, u32 size, u8 regionId, u32 CFGUHandleOffset)
{
    for(u8 *cmdPos = code; cmdPos < code + size - 28; cmdPos += 4)
    {
        static const u32 cfgSecureInfoGetRegionCmdPattern[] = {0xEE1D4F70, 0xE3A00802, 0xE5A40080};

        u32 *cmp = (u32 *)cmdPos;

        if(cmp[0] == cfgSecureInfoGetRegionCmdPattern[0] && cmp[1] == cfgSecureInfoGetRegionCmdPattern[1] &&
           cmp[2] == cfgSecureInfoGetRegionCmdPattern[2] && *((u16 *)cmdPos + 7) == 0xE59F &&
           *(u32 *)(cmdPos + 20 + *((u16 *)cmdPos + 6)) == CFGUHandleOffset)
        {
            *((u32 *)cmdPos + 4) = 0xE3A00000 | regionId; // mov    r0, =regionId
            *((u32 *)cmdPos + 5) = 0xE5C40008;            // strb   r0, [r4, 8]
            *((u32 *)cmdPos + 6) = 0xE3B00000;            // movs   r0, 0            (result code) ('s' not needed but nvm)
            *((u32 *)cmdPos + 7) = 0xE5840004;            // str    r0, [r4, 4]

            //The remaining, not patched, function code will do the rest for us
            break;
        }
    }
}

static int replace_code(u64 progid, u8 *code, u32 size) {

	char path[] = CODE_PATH;

	u32 end = strlen(path) - 5;

	for (int x = 0; x < 16; x++)
		path[end - x] = hexDigits[((progid >> 4*x) & 0xF)];

	Handle file;
	Result ret;
	u32 total;

	ret = file_open(&file, ARCHIVE_SDMC, path, FS_OPEN_READ);
	if (R_SUCCEEDED(ret))
    {
		ret = FSFILE_Read(file, &total, 0, code, size);
		FSFILE_Close(file);
	}

	return ret;
}

static int get_clock_config()
{
    char path[] = CLOCK_PATH;

    Handle file;
    Result ret;
    u32 total;
    u8 cfg[1] = {0};

    ret = file_open(&file, ARCHIVE_SDMC, path, FS_OPEN_READ);
    if (R_SUCCEEDED(ret))
    {
        FSFILE_Read(file, &total, 0, &cfg, 1);
        FSFILE_Close(file);
        cfg[0] -= '0';
    }
    
    return cfg[0];
}


void patch_code(u64 progid, u8 *code, u32 size)
{	
    u32 tid_high = (progid & 0xFFFFFFF000000000LL) >> 0x24;

	replace_code(progid, code, size); // WARNING: IT REPLACES SYSTEM TITLES' CODE AS WELL

    switch(progid)
    {
        case 0x0004003000008F02LL: // USA Menu
        case 0x0004003000008202LL: // EUR Menu
        case 0x0004003000009802LL: // JPN Menu
        case 0x000400300000A102LL: // CHN Menu
        case 0x000400300000A902LL: // KOR Menu
        case 0x000400300000B102LL: // TWN Menu
        {
            static const u8 region_free_pattern[] = {0x00, 0x00, 0x55, 0xE3, 0x01, 0x10, 0xA0, 0xE3},
                            region_free_patch[]   = {0x01, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1};

            //Patch SMDH region checks
            patch_memory(code, size, 
                region_free_pattern, 
                sizeof(region_free_pattern), -16, 
                region_free_patch, 
                sizeof(region_free_patch), 1);

            break;
        }

        case 0x0004013000001702LL: // CFG
        {
            static const u8 secureinfo_sig_check_pattern[] = {0x06, 0x46, 0x10, 0x48, 0xFC},
                            secureinfo_sig_check_patch[] = {0x00, 0x26};
            
            patch_memory(code, size, 
                secureinfo_sig_check_pattern, 
                sizeof(secureinfo_sig_check_pattern), 0, 
                secureinfo_sig_check_patch, 
                sizeof(secureinfo_sig_check_patch), 1);

            break;
        }
        
        case 0x0004013000002C02LL: // NIM
        {
            static const u8 block_updates_pattern[] = {0x25, 0x79, 0x0B, 0x99},
                            block_updates_patch[] = {0xE3, 0xA0},
                            block_eshop_updates_pattern[] = {0x30, 0xB5, 0xF1, 0xB0},
                            block_eshop_updates_patch[] = {0x00, 0x20, 0x08, 0x60, 0x70, 0x47};

            patch_memory(code, size, 
                block_updates_pattern, 
                sizeof(block_updates_pattern), 0, 
                block_updates_patch, 
                sizeof(block_updates_patch), 1);

            patch_memory(code, size, 
                block_eshop_updates_pattern, 
                sizeof(block_eshop_updates_pattern), 0, 
                block_eshop_updates_patch, 
                sizeof(block_eshop_updates_patch), 1);

            break;
        }

        case 0x0004001000021000LL: // USA MSET
        case 0x0004001000020000LL: // JPN MSET
        case 0x0004001000022000LL: // EUR MSET
        case 0x0004001000026000LL: // CHN MSET
        case 0x0004001000027000LL: // KOR MSET
        case 0x0004001000028000LL: // TWN MSET
        {
                // UTF-16, just roll with it...
                static const u8 version_pattern[] = {'V', 0, 'e', 0, 'r', 0, '.', 0},
				                version_patch[]   = {'C', 0, 'a', 0, 'k', 0, 'e', 0};

				patch_memory(code, size,
				version_pattern,
                sizeof(version_pattern), 0,
				version_patch, sizeof(version_patch), 1);

            break;
        }

        case 0x0004013000008002LL: // NS
        {
            static const u8 stop_cart_updates_pattern[] = {0x0C, 0x18, 0xE1, 0xD8},
                            stop_cart_updates_patch[]   = {0x0B, 0x18, 0x21, 0xC8};

            //Disable updates from foreign carts (makes carts region-free)
            patch_memory(code, size, 
                stop_cart_updates_pattern, 
                sizeof(stop_cart_updates_pattern), 0, 
                stop_cart_updates_patch,
                sizeof(stop_cart_updates_patch), 2);

			u32 clock_cfg = get_clock_config();

			if (!clock_cfg)
				break;

			static const u8 cfg_N3dsCpuPattern[] = {0x40, 0xA0, 0xE1, 0x07, 0x00};

			u8 *cfg_N3dsCpuLoc = memsearch(code, cfg_N3dsCpuPattern, size, sizeof(cfg_N3dsCpuPattern));

			if(cfg_N3dsCpuLoc != NULL)
			{
				*(u32 *)(cfg_N3dsCpuLoc + 3) = 0xE1A00000;
				*(u32 *)(cfg_N3dsCpuLoc + 0x1F) = (clock_cfg  == 2) ? 0xE3A00003 : 0xE3A00000;
			}
			break;
        }
	}

	if(tid_high == 0x0004000 || tid_high == 0x00040002)
	{
		//Language emulation, only for regular titles and demos
		u8 region_id = 0xFF,
		language_id = 0xFF;
		int ret = load_title_locale_config(progid, &region_id, &language_id);

		if(R_SUCCEEDED(ret))
		{
			u32 CFGUHandleOffset;
			u8 *CFGU_GetConfigInfoBlk2_endPos = get_cfg_offsets(code, size, &CFGUHandleOffset);
			if(CFGU_GetConfigInfoBlk2_endPos != NULL) {
				if(language_id != 0xFF) patch_CfgGetLanguage(code, size, language_id, CFGU_GetConfigInfoBlk2_endPos);
				if(region_id != 0xFF) patch_CfgGetRegion(code, size, region_id, CFGUHandleOffset); // Only patch if region isn't WLD yet
			}
		}
	}	
}
