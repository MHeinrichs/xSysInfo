/*
 * xSysInfo - CPU cache control
 */

#include <exec/execbase.h>

#include <proto/exec.h>

#include "xsysinfo.h"
#include "cache.h"
#include "hardware.h"

/* External references */
extern struct ExecBase *SysBase;
extern HardwareInfo hw_info;

/*
 * Toggle instruction cache
 */
void toggle_icache(void)
{
    if (!cpu_has_icache()) return;

    ULONG current = CacheControl(0, 0);

    if (current & CACRF_EnableI) {
        /* Disable ICache */
        CacheControl(0, CACRF_EnableI);
    } else {
        /* Enable ICache */
        CacheControl(CACRF_EnableI, CACRF_EnableI);
    }
}

/*
 * Toggle data cache
 */
void toggle_dcache(void)
{
    if (!cpu_has_dcache()) return;

    ULONG current = CacheControl(0, 0);

    if (current & CACRF_EnableD) {
        /* Disable DCache */
        CacheControl(0, CACRF_EnableD);
    } else {
        /* Enable DCache */
        CacheControl(CACRF_EnableD, CACRF_EnableD);
    }
}

/*
 * Toggle instruction burst
 */
void toggle_iburst(void)
{
    if (!cpu_has_iburst()) return;

    ULONG current = CacheControl(0, 0);

    if (current & CACRF_IBE) {
        /* Disable IBurst */
        CacheControl(0, CACRF_IBE);
    } else {
        /* Enable IBurst */
        CacheControl(CACRF_IBE, CACRF_IBE);
    }
}

/*
 * Toggle data burst
 */
void toggle_dburst(void)
{
    if (!cpu_has_dburst()) return;

    ULONG current = CacheControl(0, 0);

    if (current & CACRF_DBE) {
        /* Disable DBurst */
        CacheControl(0, CACRF_DBE);
    } else {
        /* Enable DBurst */
        CacheControl(CACRF_DBE, CACRF_DBE);
    }
}

/*
 * Toggle copyback mode
 */
void toggle_copyback(void)
{
    if (!cpu_has_copyback()) return;

    ULONG current = CacheControl(0, 0);

    if (current & CACRF_CopyBack) {
        /* Disable CopyBack */
        CacheControl(0, CACRF_CopyBack);
    } else {
        /* Enable CopyBack */
        CacheControl(CACRF_CopyBack, CACRF_CopyBack);
    }
}

/*
 * Read current cache state
 */
void read_cache_state(BOOL *icache, BOOL *dcache,
                      BOOL *iburst, BOOL *dburst, BOOL *copyback)
{
    ULONG cacr = CacheControl(0, 0);

    if (icache)   *icache   = (cacr & CACRF_EnableI) ? TRUE : FALSE;
    if (dcache)   *dcache   = (cacr & CACRF_EnableD) ? TRUE : FALSE;
    if (iburst)   *iburst   = (cacr & CACRF_IBE) ? TRUE : FALSE;
    if (dburst)   *dburst   = (cacr & CACRF_DBE) ? TRUE : FALSE;
    if (copyback) *copyback = (cacr & CACRF_CopyBack) ? TRUE : FALSE;
}

/*
 * Check if CPU has instruction cache
 * 68020 and above have ICache
 */
BOOL cpu_has_icache(void)
{
    return (SysBase->AttnFlags & AFF_68020) ? TRUE : FALSE;
}

/*
 * Check if CPU has data cache
 * 68030 and above have DCache (except EC versions)
 */
BOOL cpu_has_dcache(void)
{
    /* 68030+ has DCache, but EC030 doesn't have MMU/DCache */
    if (!(SysBase->AttnFlags & AFF_68030)) return FALSE;

    /* EC030 doesn't have DCache - detect via identify or other means */
    if (hw_info.cpu_type == CPU_68EC030) return FALSE;

    return TRUE;
}

/*
 * Check if CPU has instruction burst mode
 * 68030 and above support burst
 */
BOOL cpu_has_iburst(void)
{
    return (SysBase->AttnFlags & AFF_68030) ? TRUE : FALSE;
}

/*
 * Check if CPU has data burst mode
 * 68030 and above support burst
 */
BOOL cpu_has_dburst(void)
{
    if (!(SysBase->AttnFlags & AFF_68030)) return FALSE;
    if (hw_info.cpu_type == CPU_68EC030) return FALSE;
    return TRUE;
}

/*
 * Check if CPU has copyback mode
 * 68040 and 68060 support copyback
 */
BOOL cpu_has_copyback(void)
{
    if (SysBase->AttnFlags & AFF_68040) return TRUE;
    if (SysBase->AttnFlags & AFF_68060) return TRUE;
    return FALSE;
}
