// SPDX-License-Identifier: BSD-2-Clause
// SPDX-FileCopyrightText: 2025 Stefan Reinauer

/*
 * xSysInfo - Hardware detection using identify.library
 */

#include <string.h>
#include <stdio.h>

#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <resources/cia.h>
#include <libraries/identify.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/timer.h>
#include <proto/identify.h>

#include "xsysinfo.h"
#include "hardware.h"
#include "locale_str.h"
#include "debug.h"

/* Global hardware info */
HardwareInfo hw_info;

/* External library bases */
extern struct ExecBase *SysBase;
extern struct GfxBase *GfxBase;
extern struct Library *IdentifyBase;

/* String buffer for identify.library results */
static char id_buffer[256];

/* Helper to safely get string from identify.library */
static void get_hardware_string(ULONG type, char *buffer, size_t size)
{
    STRPTR result = IdHardware(type, NULL);
    if (result) {
        strncpy(buffer, (const char *)result, size - 1);
        buffer[size - 1] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

/*
 * Main hardware detection function
 */
BOOL detect_hardware(void)
{

    memset(&hw_info, 0, sizeof(HardwareInfo));

    debug("  hw: Detecting CPU...\n");
    detect_cpu();
    debug("  hw: Detecting FPU...\n");
    detect_fpu();
    debug("  hw: Detecting MMU...\n");
    detect_mmu();
    debug("  hw: Detecting chipset...\n");
    detect_chipset();
    debug("  hw: Detecting system chips...\n");
    detect_system_chips();
    debug("  hw: Detecting clock...\n");
    detect_clock();
    debug("  hw: Detecting batt mem ressources...\n");
    detect_batt_mem();
    debug("  hw: Detecting frequencies...\n");
    detect_frequencies();
    debug("  hw: Refreshing cache status...\n");
    refresh_cache_status();
    debug("  hw: Generating comment...\n");
    generate_comment();

    /* Get Kickstart info */
    UWORD kick_version = *((volatile UWORD *)0xF8000C);
    UWORD kick_revision = *((volatile UWORD *)0xF8000E);
    hw_info.kickstart_version = kick_version;
    hw_info.kickstart_revision = kick_revision;

    /* Fallback to exec version if above didn't provide ROM version */
    if (hw_info.kickstart_version == 0) {
        hw_info.kickstart_version = SysBase->LibNode.lib_Version;
        hw_info.kickstart_revision = SysBase->LibNode.lib_Revision;
    }

    /* Get ROM size*/
    UWORD kick_size = *((volatile UWORD *)0xF80000);
    if(kick_size == 0x1111){
        hw_info.kickstart_size = 256;
    }
    else{
        /* Fallback: default to 512K */
        hw_info.kickstart_size = 512;
    }

    debug("  hw: Hardware detection complete.\n");
    return TRUE;
}

/*
 * Detect CPU type and speed
 */
void detect_cpu(void)
{
    ULONG cpu_num;

    /* Get CPU string from identify.library */
    get_hardware_string(IDHW_CPU, id_buffer, sizeof(id_buffer));
    snprintf(hw_info.cpu_string, sizeof(hw_info.cpu_string), "%s", id_buffer);

    /* Get numeric CPU type */
    cpu_num = IdHardwareNum(IDHW_CPU, NULL);

    switch (cpu_num) {
        case IDCPU_68000:
            hw_info.cpu_type = CPU_68000;
            break;
        case IDCPU_68010:
            hw_info.cpu_type = CPU_68010;
            break;
        case IDCPU_68020:
            hw_info.cpu_type = CPU_68020;
            break;
        /* IDCPU_68EC020 not defined in identify.h V45 */
        case IDCPU_68030:
            hw_info.cpu_type = CPU_68030;
            break;
        case IDCPU_68EC030:
            hw_info.cpu_type = CPU_68EC030;
            break;
        case IDCPU_68040:
            hw_info.cpu_type = CPU_68040;
            break;
        case IDCPU_68LC040:
            hw_info.cpu_type = CPU_68LC040;
            break;
        case IDCPU_68060:
            hw_info.cpu_type = CPU_68060;
            break;
        /* IDCPU_68EC060 not defined in identify.h V45 */
        case IDCPU_68LC060:
            hw_info.cpu_type = CPU_68LC060;
            break;
        default:
            hw_info.cpu_type = CPU_UNKNOWN;
            break;
    }

    /* Get CPU frequency from identify.library */
    hw_info.cpu_mhz = measure_cpu_frequency();

    /* Get CPU revision from identify.library (returns string) */
    get_hardware_string(IDHW_CPUREV, hw_info.cpu_revision,
                        sizeof(hw_info.cpu_revision));
}

/*
 * Detect FPU type
 */
void detect_fpu(void)
{
    ULONG fpu_num;
    ULONG fpu_clock;

    /* Get FPU string from identify.library */
    get_hardware_string(IDHW_FPU, id_buffer, sizeof(id_buffer));
    snprintf(hw_info.fpu_string, sizeof(hw_info.fpu_string), "%s", id_buffer);

    /* Get numeric FPU type */
    fpu_num = IdHardwareNum(IDHW_FPU, NULL);

    switch (fpu_num) {
        case IDFPU_NONE:
            hw_info.fpu_type = FPU_NONE;
            strncpy(hw_info.fpu_string, get_string(MSG_NONE),
                    sizeof(hw_info.fpu_string) - 1);
            break;
        case IDFPU_68881:
            hw_info.fpu_type = FPU_68881;
            break;
        case IDFPU_68882:
            hw_info.fpu_type = FPU_68882;
            break;
        case IDFPU_68040:
            hw_info.fpu_type = FPU_68040;
            break;
        case IDFPU_68060:
            hw_info.fpu_type = FPU_68060;
            break;
        default:
            hw_info.fpu_type = FPU_UNKNOWN;
            break;
    }

    /* Get FPU clock from identify.library */
    fpu_clock = IdHardwareNum(IDHW_FPUCLOCK, NULL);
    if (fpu_clock > 0 && fpu_clock < 1000) {
        hw_info.fpu_mhz = fpu_clock * 100;
    } else {
        hw_info.fpu_mhz = 0;
    }
}

/*
 * Detect MMU type
 */
void detect_mmu(void)
{
    ULONG mmu_num;

    /* Get MMU string from identify.library */
    get_hardware_string(IDHW_MMU, id_buffer, sizeof(id_buffer));
    snprintf(hw_info.mmu_string, sizeof(hw_info.mmu_string), "%s", id_buffer);

    /* Get numeric MMU type */
    mmu_num = IdHardwareNum(IDHW_MMU, NULL);

    switch (mmu_num) {
        case IDMMU_NONE:
            hw_info.mmu_type = MMU_NONE;
            strncpy(hw_info.mmu_string, get_string(MSG_NA),
                    sizeof(hw_info.mmu_string) - 1);
            break;
        case IDMMU_68851:
            hw_info.mmu_type = MMU_68851;
            break;
        case IDMMU_68030:
            hw_info.mmu_type = MMU_68030;
            break;
        case IDMMU_68040:
            hw_info.mmu_type = MMU_68040;
            break;
        case IDMMU_68060:
            hw_info.mmu_type = MMU_68060;
            break;
        default:
            hw_info.mmu_type = MMU_UNKNOWN;
            break;
    }

    /* Check if MMU is in use (mmu.library loaded) */
    hw_info.mmu_enabled = FALSE;
    if (hw_info.mmu_type != MMU_NONE) {
        struct Library *mmulib = OpenLibrary((CONST_STRPTR)"mmu.library", 0);
        if (mmulib) {
            hw_info.mmu_enabled = TRUE;
            CloseLibrary(mmulib);
        }
    }

    /* Get VBR */
    get_hardware_string(IDHW_VBR, id_buffer, sizeof(id_buffer));
    hw_info.vbr = IdHardwareNum(IDHW_VBR, NULL);
}

/*
 * Detect chipset (Agnus/Denise)
 */
void detect_chipset(void)
{
    ULONG chipset, agnus_mode;
    UWORD tmp, i;

    /* Get Agnus/Alice info */
    get_hardware_string(IDHW_AGNUS, id_buffer, sizeof(id_buffer));
    snprintf(hw_info.agnus_string, sizeof(hw_info.agnus_string), "%s", id_buffer);

    chipset = IdHardwareNum(IDHW_CHIPSET, NULL);
    agnus_mode = IdHardwareNum(IDHW_AGNUSMODE, NULL);

    /* Determine Agnus type and max chip RAM */
    hw_info.max_chip_ram = 512 * 1024;  /* Default 512K */

    if (chipset == IDCS_AGA || chipset == IDCS_AAA || chipset == IDCS_SAGA) {
        /* AGA/Alice */
        hw_info.max_chip_ram = 2 * 1024 * 1024;
        if (agnus_mode == IDAM_PAL) {
            hw_info.agnus_type = AGNUS_ALICE_PAL;
        } else {
            hw_info.agnus_type = AGNUS_ALICE_NTSC;
        }
    } else if (chipset == IDCS_ECS || chipset == IDCS_NECS) {
        /* ECS */
        hw_info.max_chip_ram = 2 * 1024 * 1024; /* Can address up to 2MB, though usually 1MB */
        if (agnus_mode == IDAM_PAL) {
            hw_info.agnus_type = AGNUS_ECS_PAL;
        } else {
            hw_info.agnus_type = AGNUS_ECS_NTSC;
        }
    } else {
        /* OCS or Unknown */
        if (agnus_mode == IDAM_PAL) {
            hw_info.agnus_type = AGNUS_OCS_PAL;
        } else if (agnus_mode == IDAM_NTSC) {
            hw_info.agnus_type = AGNUS_OCS_NTSC;
        } else {
            hw_info.agnus_type = AGNUS_UNKNOWN;
        }
    }

    /*Get Paula revision*/
    hw_info.paula_rev = *((volatile UWORD *)(CUSTOM_PAULA_ID));
    hw_info.paula_rev &= 0x00FE; //mask irelevant bits
    switch(hw_info.paula_rev){
        case 0:
            hw_info.paula_type = PAULA_ORIG;
            break;
        case 2:
            hw_info.paula_type = PAULA_SAGA;
            break;
        default:
            hw_info.paula_type = PAULA_UNKNOWN;
            break;
    }

    /* Get Denise/Lisa info */
    hw_info.denise_rev = *((volatile UWORD *)(CUSTOM_DENISE_ID));
    hw_info.denise_rev &= 0xFF;
    for(i=0; i<32;++i){
        tmp = *((volatile UWORD *)(CUSTOM_DENISE_ID)); //OCS Denise puts gibberish on the bus
        tmp &= 0xFF;
        if(tmp != hw_info.denise_rev){
            hw_info.denise_rev = 0;
            break;
        }
    }

    if(hw_info.paula_type == PAULA_SAGA)
        hw_info.denise_type = DENISE_ISABEL;  
    else if(hw_info.denise_rev == 0){
        hw_info.denise_type = DENISE_OCS;
    }
    else if(hw_info.denise_rev == 0xFC){
        hw_info.denise_type = DENISE_ECS;
    }
    else if(hw_info.denise_rev == 0xF8){
        hw_info.denise_type = DENISE_LISA;
    }
    else if(hw_info.denise_rev == 0xF0){
        hw_info.denise_type = IDDN_MONICA;
    }
    else if(hw_info.denise_rev == 0xF1){
        hw_info.denise_type = IDDN_MONICA;
    }
    else{
        hw_info.denise_type = DENISE_UNKNOWN;
    }
}

/*
 * Detect RTC clock chip
 */
void detect_clock(void)
{
    unsigned char val;
    /*
    The clock registers are shifted: (reg*4)+3, so clockregister F becomes 3F
    Clock must be at dc0000 (Only A1000 has the clock somewhere else, A2000BSW?!?)

    Assumption: a trashed clock is untrashed by kickstart-initialization
    According to commodore the working procedure is:
    Read register F: MSM6242B is '0100' and RP5C01A '0000'
    If '0100' is read, MSM6242B should be there
    Because '0000' is not trustworthy do the second check:
    Read register D: MSM6242B is '0000' and RP5C01A '1001' (Timer Enabled,no alarm, Mode 01)
    If not 1001, write 1001! (This causes an MSM6242B to skip 30 secs)
    Now read the 12/24 register (Reg A): 
    0001 should appear on RP5C01A
    */
    if(hw_info.gary_type != GARY_A1000){ //this does not work in an stock A1000!
        val = *((volatile unsigned char *)(RTC_BASE+RTC_REG_F));
        val &= RTC_MASK;
        if(val == 0b0100){
            hw_info.clock_type = CLOCK_MSM6242;
            strncpy(hw_info.clock_string, get_string(MSG_MSM6242B),
                        sizeof(hw_info.clock_string) - 1);
            return;
        }
        if (val > 0){ //when val is not 0 we have no RTC!
            hw_info.clock_type = CLOCK_NONE;
            strncpy(hw_info.clock_string, get_string(MSG_CLOCK_NOT_FOUND),
                    sizeof(hw_info.clock_string) - 1);
            return;
        }
        val = *((volatile unsigned char *)(RTC_BASE+RTC_REG_D));
        val &= RTC_MASK;
        if(val != 0b1001){ //status not OK?
            //set correct status and try again
            *((volatile unsigned char *)(RTC_BASE+RTC_REG_D)) = 0b1001;        
            val = *((volatile unsigned char *)(RTC_BASE+RTC_REG_D));
            val &= RTC_MASK;
        }
        if(val == 0b1001){ //status OK?
            //write something to Reg c. it should be read as 0!
            *((volatile unsigned char *)(RTC_BASE+RTC_REG_C)) = 5;        
            val = *((volatile unsigned char *)(RTC_BASE+RTC_REG_C));
            val &= RTC_MASK;
            if(val == 0){ // comming closer: now read register A, which should be '0001!
                val = *((volatile unsigned char *)(RTC_BASE+RTC_REG_A));
                val &= RTC_MASK;
                if(val ==1 ){ // bingo! RP5C01A
                    hw_info.clock_type = CLOCK_RP5C01;
                    strncpy(hw_info.clock_string, get_string(MSG_RP5C01A),
                        sizeof(hw_info.clock_string) - 1);
                    return;
                }
            }
        }
    }   
    // if we drop here, we found no clock
    hw_info.clock_type = CLOCK_NONE;
    strncpy(hw_info.clock_string, get_string(MSG_CLOCK_NOT_FOUND),
            sizeof(hw_info.clock_string) - 1);
    return;
}

/*
 * Detect if NV-Ram (batt Mem is available and read values)
 * Call after detect_clock and detect_chips!
 */
void detect_batt_mem(void)
{        
    hw_info.battMemData.valid_data = FALSE;
    
    if( hw_info.clock_type == CLOCK_RP5C01 //clock with NV-ram
        && hw_info.ramsey_rev > 0 //we have a ramsey (and might be a A3000
        && openBattMem() //batt mem ressource is open
        ){
        hw_info.battMemData.valid_data = readBattMem(&hw_info.battMemData);
        hw_info.battMemData.valid_data = TRUE;
    }
    
}

/*
 * Detect Ramsey
 */
void detect_ramsey(void){

    /* Ramsey (A3000/A4000 RAM controller) */
/*  
    ULONG ramsey_num;
    ramsey_num = IdHardwareNum(IDHW_RAMSEY, NULL);
    if (ramsey_num != 0 && ramsey_num != IDRSY_NONE) {
        hw_info.ramsey_rev = ramsey_num;
    } else {
        hw_info.ramsey_rev = 0;
    }
*/
    if(hw_info.gary_type != FAT_GARY){ // no fat gary -> no ramsey!
        hw_info.ramsey_rev = 0;
        return;
    }

    hw_info.ramsey_rev = (ULONG)*((volatile unsigned char *)(RAMSEY_VER));
    if(hw_info.ramsey_rev == 0xFF){ // unlikely!
        hw_info.ramsey_rev = 0; 
    }
    if(hw_info.ramsey_rev > 0){
        hw_info.ramsey_ctl = *((volatile unsigned char *)(RAMSEY_CTRL));

        hw_info.ramsey_page_enabled = hw_info.ramsey_ctl & RAMSEY_PAGE_MODE;
        hw_info.ramsey_burst_enabled = hw_info.ramsey_ctl & RAMSEY_BURST_MODE;
        hw_info.ramsey_wrap_enabled = hw_info.ramsey_ctl & RAMSEY_WRAP_MODE;
        hw_info.ramsey_size_1M = hw_info.ramsey_ctl & RAMSEY_SIZE;
        hw_info.ramsey_skip_enabled = hw_info.ramsey_ctl & RAMSEY_SKIP_MODE;
        hw_info.ramsey_refresh_rate = (hw_info.ramsey_ctl & RAMSEY_REFESH_MODE)>>4;
    }
}

/*
 * Detect SDMAC
 */


/* Returns 2 for SDMAC-02, 4 for SDMAC-04/ReSDMAC, 0 if not present/detection fails */
void detect_sdmac(void)
{
    unsigned char sdmac_rev;
    uint32_t ovalue, rvalue, wvalue;
    uint8_t sdmac_version = 0;
    uint8_t istr;
    int pass;
    hw_info.sdmac_rev = 0;
    hw_info.is_A4000T = FALSE;
    if(hw_info.ramsey_rev>0){ //you need ramsey to access sdmac!    
        sdmac_rev = *((volatile unsigned char *)(SDMAC_REVISION)); //this works only on resdmac
        if (sdmac_rev != 0 && sdmac_rev != 0xFF) {
            hw_info.sdmac_rev = sdmac_rev;
        } 
        if(hw_info.sdmac_rev == 0){
            //now test for A4000T NCR53C710: upper four bits of CTEST8-register contains the chip-rev.
           sdmac_rev = *((volatile unsigned char *)(NCR_CTEST8_REG));  
           sdmac_rev = (sdmac_rev&0xF0)>>4; //only upper four bits matter           
           if (sdmac_rev != 0 && sdmac_rev != 0xF) {
            hw_info.sdmac_rev = sdmac_rev;
            hw_info.is_A4000T = TRUE;
           }    
        }
        if(hw_info.sdmac_rev == 0){
            /* Quick check: ISTR bits - FIFO cannot be both empty and full */
            istr = *SDMAC_ISTR;
            if (istr == 0xff)
                return;
            if ((istr & SDMAC_ISTR_FIFOE) && (istr & SDMAC_ISTR_FIFOF))
                return;

            /* Probe WTC registers to distinguish SDMAC-02 from SDMAC-04 */
            for (pass = 0; pass < 6; pass++) {
                switch (pass) {
                    case 0: wvalue = 0x00000000; break;
                    case 1: wvalue = 0xffffffff; break;
                    case 2: wvalue = 0xa5a5a5a5; break;
                    case 3: wvalue = 0x5a5a5a5a; break;
                    case 4: wvalue = 0xc2c2c3c3; break;
                    case 5: wvalue = 0x3c3c3c3c; break;
                }

                ovalue = *SDMAC_WTC_ALT;
                *SDMAC_WTC_ALT = wvalue;
                *((volatile unsigned char *)RAMSEY_VER) = 0;/* Push write to bus */
                rvalue = *SDMAC_WTC;
                *SDMAC_WTC_ALT = ovalue;

                if (rvalue == wvalue) {
                    if ((wvalue != 0x00000000) && (wvalue != 0xffffffff))
                        sdmac_version = 0; /* Detection failed */
                    else
                        sdmac_version = 2;
                } else if (((rvalue ^ wvalue) & 0x00ffffff) == 0) {
                    /* SDMAC-02: only upper byte differs */
                    sdmac_version = 2;
                } else if ((rvalue & (1 << 2)) == 0) {
                    /* SDMAC-04: bit 2 is always 0 */
                    if (wvalue & (1 << 2))
                        sdmac_version = 4;
                    else
                        sdmac_version = 2;
                } 
                hw_info.sdmac_rev = sdmac_version;
            }
        }
    }
}


/*
 * Detect Gary
 */
void detect_gary(void){
    
    UWORD testVal1, testVal2, i ;  
    unsigned char val, tmp,tmp2;  

    /*tricky part for manual detection:
        A1000 mirrors the chipregisters. 
        A save register to read is DMACONR. If this register is mirrored A1000 is there.
        If this register is not mirrored, proceed:
        A600/A1200 have an undocumented revision register at DE1000, but the 8-bit code is
        "morsed" only via the highest byte
        If this is "FF", a A500/2000 or FatGary is present:
        FatGary has a PowerUp-Detect-register at DE0002. However, it is read/writeable.
        If the value changes with the writes, it's a FatGary
    */
    hw_info.gary_type = GARY_UNKNOWN;
    //test for mirroring (A1000/ A2000BSW)
    testVal1 = *((volatile UWORD *)(CUSTOM_JOY0DAT));
    testVal2 = *((volatile UWORD *)(CUSTOM_JOY1DAT)); //avoid bus stickyness (A3000)
    testVal2 = *((volatile UWORD *)(CUSTOM_JOY0DAT_MIRR));
    if(testVal1 == testVal2){
        //do another test to be save
        testVal1 = *((volatile UWORD *)(CUSTOM_JOY1DAT));
        testVal2 = *((volatile UWORD *)(CUSTOM_JOY0DAT)); //avoid bus stickyness (A3000)
        testVal2 = *((volatile UWORD *)(CUSTOM_JOY1DAT_MIRR));
        if(testVal1 == testVal2){
            hw_info.gary_type = GARY_A1000;
            return;
        }
    }

    /*
    We are in A500..A4000 range!
    A3000(T/+), A4000(T)
    Next we have to check the Power-Up-Register (A3000/4000).
    */
    val =  *((volatile unsigned char *)(FAT_GARY_POWER)); //save old value
    //write a FF    
    *((volatile unsigned char *)FAT_GARY_POWER) = 0x80; //set bit 7
    //read something from the bus to clear sticky bus
    testVal1 = *((volatile UWORD *)(CUSTOM_JOY0DAT));
    //read back POWER register
    tmp = *((volatile unsigned char *)(FAT_GARY_POWER));
    tmp &= 0x80; //mask bus rubbish
    if(tmp == 0x80){
        //write a zero to MSB
        *((volatile unsigned char *)FAT_GARY_POWER) = 0;
        //read something from the bus to clear sticky bus
        testVal1 = *((volatile UWORD *)(CUSTOM_JOY0DAT));
        //read back
        tmp = *((volatile unsigned char *)(FAT_GARY_POWER));
        tmp &= 0x80; //mask bus rubbish
        if(tmp == 0){
            hw_info.gary_type = FAT_GARY;
            //restore old value
            *((volatile unsigned char *)FAT_GARY_POWER) = val;
            return;
        }

    }

    /*
    Leftovers:
    A500, A2000, CDTV, A600/A1200:
    now we read the GAYLE_ID: Write a zero to the ID-register and read it back 8 times!
    */

    val = 0;
    //test for mirroring (A500)
    tmp2 = *((volatile unsigned char *)(CUSTOM_BLTDDAT));
    *((volatile unsigned char *)(GAYLE_ID)) = 0;
    for(i=0; i<8; i++){
        tmp = *((volatile unsigned char *)(GAYLE_ID));
        if(i == 0 && tmp == tmp2){ //a500 gary!
            hw_info.gary_type = GARY_A500;
            return; 
        }
        //mask
        tmp &= 0x80>>i;
        val = val|(tmp>>i);
    }
    if(tmp!=0xFF && tmp !=0){
        hw_info.gary_type = GAYLE;
        hw_info.gary_rev = (ULONG)val;
        return;
    }
    
    hw_info.gary_type = GARY_A500;
    return;
}


/*
 * Detect Ramsey, Gary, and expansion slots
 */
void detect_system_chips(void)
{
    


    detect_gary();
    detect_ramsey();
    detect_sdmac();



    /* Check for expansion slots */
    /* Check for Zorro slots */
    hw_info.has_zorro_slots = FALSE;
    hw_info.has_pcmcia = FALSE;
    snprintf(hw_info.card_slot_string, sizeof(hw_info.card_slot_string),
             "%s", get_string(MSG_NA));

    /* Prefer real hardware: if card.resource exists, PCMCIA is present */
    if (OpenResource((CONST_STRPTR)"card.resource") != NULL) {
        hw_info.has_pcmcia = TRUE;
        snprintf(hw_info.card_slot_string, sizeof(hw_info.card_slot_string),
                 "%s", get_string(MSG_SLOT_PCMCIA));
        return;
    }

    /* Detect Zorro capability based on chipset presence:
     * - Ramsey (memory controller) indicates A3000/A4000 = Zorro III
     * - Gary without Ramsey indicates A500/A2000 = Zorro II
     * This is more reliable than system type detection which can fail
     * with CPU upgrades (e.g., 68060)
     */
    if (hw_info.ramsey_rev != 0) {
        /* A3000/A4000 have Ramsey - these have Zorro III slots */
        hw_info.has_zorro_slots = TRUE;
        snprintf(hw_info.card_slot_string, sizeof(hw_info.card_slot_string),
                 "%s", get_string(MSG_ZORRO_III));
        return;
    }

    if (hw_info.gary_type == GAYLE) {
        /* A600 A1200 have a Gayle chip with ID*/
        hw_info.has_pcmcia = TRUE;
        snprintf(hw_info.card_slot_string, sizeof(hw_info.card_slot_string),
                "%s", get_string(MSG_SLOT_PCMCIA));
        return;
    }

    /* A500/A1000/A2000/CDTV*/
    hw_info.has_zorro_slots = TRUE;
    snprintf(hw_info.card_slot_string, sizeof(hw_info.card_slot_string),
             "%s", get_string(MSG_ZORRO_II));
    return;
}

/*
 * Detect screen frequencies
 */
void detect_frequencies(void)
{
    /* PAL/NTSC detection */
    hw_info.is_pal = (GfxBase->DisplayFlags & PAL) ? TRUE : FALSE;

    if (hw_info.is_pal) {
        hw_info.horiz_freq = 15625;     /* 15.625 kHz */
        hw_info.vert_freq = 50;
        hw_info.supply_freq = 50;
        strncpy(hw_info.mode_string, get_string(MSG_MODE_PAL), sizeof(hw_info.mode_string) - 1);
    } else {
        hw_info.horiz_freq = 15734;     /* 15.734 kHz */
        hw_info.vert_freq = 60;
        hw_info.supply_freq = 60;
        strncpy(hw_info.mode_string, get_string(MSG_MODE_NTSC), sizeof(hw_info.mode_string) - 1);
    }

    /* EClock frequency from exec */
    hw_info.eclock_freq = SysBase->ex_EClockFrequency;
}

/*
 * Refresh cache status from current CACR
 */
void refresh_cache_status(void)
{
    ULONG cacr_bits;

    /* Determine available cache features based on CPU type */
    hw_info.has_icache = (hw_info.cpu_type >= CPU_68020);
    hw_info.has_dcache = (hw_info.cpu_type >= CPU_68030);
    hw_info.has_iburst = (hw_info.cpu_type >= CPU_68030);
    hw_info.has_dburst = (hw_info.cpu_type >= CPU_68030);
    hw_info.has_copyback = (hw_info.cpu_type >= CPU_68040 &&
                            hw_info.cpu_type != CPU_68LC040);

    /* Get current cache state */
    cacr_bits = CacheControl(0, 0);

    hw_info.icache_enabled = (cacr_bits & CACRF_EnableI) ? TRUE : FALSE;
    hw_info.dcache_enabled = (cacr_bits & CACRF_EnableD) ? TRUE : FALSE;
    hw_info.iburst_enabled = (cacr_bits & CACRF_IBE) ? TRUE : FALSE;
    hw_info.dburst_enabled = (cacr_bits & CACRF_DBE) ? TRUE : FALSE;
    hw_info.copyback_enabled = (cacr_bits & CACRF_CopyBack) ? TRUE : FALSE;
}

/*
 * Generate a comment based on system configuration
 */
void generate_comment(void)
{
    const char *comment;

    if (hw_info.cpu_type >= CPU_68060 && hw_info.cpu_mhz >= 5000) {
        comment = get_string(MSG_COMMENT_BLAZING);
    } else if (hw_info.cpu_type >= CPU_68060 && hw_info.cpu_mhz >= 2500) {
        comment = get_string(MSG_COMMENT_VERY_FAST);
    } else if (hw_info.cpu_type >= CPU_68040 && hw_info.cpu_mhz >= 2500) {
        comment = get_string(MSG_COMMENT_VERY_FAST);
    } else if (hw_info.cpu_type >= CPU_68030 && hw_info.cpu_mhz >= 2500) {
        comment = get_string(MSG_COMMENT_FAST);
    } else if (hw_info.cpu_type >= CPU_68020 && hw_info.cpu_mhz >= 1400) {
        comment = get_string(MSG_COMMENT_GOOD);
    } else if (hw_info.cpu_type <= CPU_68010) {
        comment = get_string(MSG_COMMENT_CLASSIC);
    } else {
        comment = get_string(MSG_COMMENT_DEFAULT);
    }

    strncpy(hw_info.comment, comment, sizeof(hw_info.comment) - 1);
}

/*
 * Get CPU frequency from identify.library (scaled by 100)
 * Falls back to estimates based on CPU type if not available
 */
ULONG measure_cpu_frequency(void)
{
    /* Try to get from identify.library first */
    ULONG speed_mhz = IdHardwareNum(IDHW_CPUCLOCK, NULL);

    if (speed_mhz > 0 && speed_mhz < 1000) {
        return speed_mhz * 100;
    }

    /* Fallback: estimate based on CPU type and system */
    switch (hw_info.cpu_type) {
        case CPU_68000:
        case CPU_68010:
            return 709;   /* Standard 68000 */
        case CPU_68020:
        case CPU_68EC020:
            return 1400;  /* Common for A1200/accelerators */
        case CPU_68030:
        case CPU_68EC030:
            return 2500;  /* Common for 030 accelerators */
        case CPU_68040:
        case CPU_68LC040:
            return 2500;  /* A4000 stock */
        case CPU_68060:
        case CPU_68EC060:
        case CPU_68LC060:
            return 5000;  /* Common 060 speed */
        default:
            return 709;
    }
}
