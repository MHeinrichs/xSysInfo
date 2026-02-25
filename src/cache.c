// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Stefan Reinauer

/*
 * xSysInfo - CPU cache control
 */

#include <proto/exec.h>

#include "xsysinfo.h"
#include "cache.h"
#include "hardware.h"
#include "cpu.h"
#include "debug.h"

/* External references */
extern HardwareInfo hw_info;

/*
    helper to convert 68030-style cachebits to 68040-style cachebits
*/

ULONG convert68030to68040(ULONG input){
    ULONG output = 0, keeper;
    if(hw_info.cpu_type >= CPU_68040 && hw_info.cpu_type <= CPU_68080){
        if((input & (CACRF_CopyBack|CACRF_EnableD|CACRF_DBE))>0){//datacache enable
            output = CACRF_CopyBack;
        }
        if((input & (CACRF_EnableI|CACRF_IBE))>0){//instructioncache enable
            output = output | CACRF_ICACHE040;
        }
        //now keep all unaffected flags
        keeper = input & ~(CACRF_CopyBack|CACRF_EnableD|CACRF_DBE|CACRF_EnableI|CACRF_IBE|CACRF_ICACHE040);
        output = output | keeper;
        debug("  cache: convert68030to68040 in %X out %X\n", input, output);
    }
    else{
        output = input;
    }
    return output;
}

ULONG convert68040to68030(ULONG input){
    ULONG output = 0, keeper;
    if(hw_info.cpu_type >= CPU_68040 && hw_info.cpu_type <= CPU_68080){
        if((input & CACRF_CopyBack)>0){//datacache enable
            output = CACRF_CopyBack|CACRF_EnableD|CACRF_DBE;
        }
        if((input & CACRF_ICACHE040)>0){//instructioncache enable
            output = output | CACRF_EnableI|CACRF_IBE;
        }
        keeper = input & ~(CACRF_CopyBack|CACRF_EnableD|CACRF_DBE|CACRF_EnableI|CACRF_IBE|CACRF_ICACHE040);
        output = output | keeper;
        debug("  cache: convert68040to68030 in %X out %X\n", input, output);
    }
    else{
        output = input;
    }
    return output;
}

/*
The 68040 has no burst mode just a general instruction cache and datacache.
So any change in I/D-cache causes all I/D-cache-bits to be accesed
*/
ULONG convertFlagsFor68040(ULONG input){
    ULONG output = 0;
    if(hw_info.cpu_type >= CPU_68040 && hw_info.cpu_type <= CPU_68080){
        if((input & (CACRF_CopyBack|CACRF_EnableD|CACRF_DBE))>0){//datacache enable
            output = CACRF_CopyBack|CACRF_EnableD|CACRF_DBE;
        }
        if((input & (CACRF_EnableI|CACRF_IBE))>0){//instructioncache enable
            output = output | CACRF_EnableI|CACRF_IBE;
        }
        debug("  cache: convertFlagsFor68040 in %X out %X\n", input, output);
    }
    else{
        output = input;
    }
    return output;

}


/*
 * Helper to toggle a cache flag
 */
static void toggle_cache_flag(ULONG flag)
{
    ULONG current = convert68040to68030(GetCacheBits());
    flag = convertFlagsFor68040(flag);
    if ((current & flag)>0) {
        /* Disable */
        current = current & ~(flag);
        debug("  cache: toggle_cache_flag disabling %X result: %X\n", flag,current);
    } else {
        /* Enable */
        current = current | flag;
        debug("  cache: toggle_cache_flag enabling %X result: %X\n", flag,current);
    }

    SetCacheBits(convert68030to68040(current));
}

/*
 * Toggle instruction cache
 */
void toggle_icache(void)
{
    if (cpu_has_icache()) {
        toggle_cache_flag(CACRF_EnableI);
    }
}

/*
 * Toggle data cache
 */
void toggle_dcache(void)
{
    if (cpu_has_dcache()) {
        toggle_cache_flag(CACRF_EnableD);
    }
}

/*
 * Toggle instruction burst
 */
void toggle_iburst(void)
{
    if (cpu_has_iburst()) {
        toggle_cache_flag(CACRF_IBE);
    }
}

/*
 * Toggle data burst
 */
void toggle_dburst(void)
{
    if (cpu_has_dburst()) {
        toggle_cache_flag(CACRF_DBE);
    }
}

/*
 * Toggle copyback mode
 */
void toggle_copyback(void)
{
    if (cpu_has_copyback()) {
        toggle_cache_flag(CACRF_CopyBack);
    }
}

/*
 * Toggle copyback mode
 */
void toggle_super_scalar(void)
{
    if (cpu_has_super_scalar()) {
        hw_info.super_scalar_enabled = set_super_scalar_mode(!(hw_info.super_scalar_enabled));
    }
}

/*
 * Check if CPU has instruction cache
 */
BOOL cpu_has_icache(void)
{
    return hw_info.has_icache;
}

/*
 * Check if CPU has data cache
 */
BOOL cpu_has_dcache(void)
{
    return hw_info.has_dcache;
}

/*
 * Check if CPU has instruction burst mode
 */
BOOL cpu_has_iburst(void)
{
    return hw_info.has_iburst;
}

/*
 * Check if CPU has data burst mode
 */
BOOL cpu_has_dburst(void)
{
    return hw_info.has_dburst;
}

/*
 * Check if CPU has copyback mode
 */
BOOL cpu_has_copyback(void)
{
    return hw_info.has_copyback;
}

/*
 * Check if CPU has copyback mode
 */
BOOL cpu_has_super_scalar(void)
{
    return hw_info.has_super_scalar;
}
