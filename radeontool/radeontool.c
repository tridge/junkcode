 
/* Radeontool   v1.4
 * by Frederick Dean <software@fdd.com>
 * Copyright 2002-2004 Frederick Dean
 * Use hereby granted under the zlib license.
 *
 * Warning: I do not have the Radeon documents, so this was engineered from 
 * the radeon_reg.h header file.  
 *
 * USE RADEONTOOL AT YOUR OWN RISK
 *
 * Thanks to Deepak Chawla, Erno Kuusela, Rolf Offermanns, and Soos Peter
 * for patches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <asm/page.h>
#include <fnmatch.h>

#include "radeon_reg.h"

int debug;
int skip;

/* *radeon_cntl_mem is mapped to the actual device's memory mapped control area. */
/* Not the address but what it points to is volatile. */
unsigned char * volatile radeon_cntl_mem;

static void fatal(char *why)
{
    fprintf(stderr,why);
    exit (-1);
}

static unsigned long radeon_get(unsigned long offset,char *name)
{
    unsigned long value;
    if(debug) 
        printf("reading %s (%lx) is ",name,offset);
    if(radeon_cntl_mem == NULL) {
        printf("internal error\n");
	exit(-2);
    };
    value = *(unsigned long * volatile)(radeon_cntl_mem+offset);  
    if(debug) 
        printf("%08lx\n",value);
    return value;
}
static void radeon_set(unsigned long offset,char *name,unsigned long value)
{
    if(debug) 
        printf("writing %s (%lx) -> %08lx\n",name,offset,value);
    if(radeon_cntl_mem == NULL) {
        printf("internal error\n");
	exit(-2);
    };
    *(unsigned long * volatile)(radeon_cntl_mem+offset) = value;  
}

static void usage(void)
{
    printf("usage: radeontool [options] [command]\n");
    printf("         --debug            - show a little debug info\n");
    printf("         --skip=1           - use the second radeon card\n");
    printf("         dac [on|off]       - power down the external video outputs (%s)\n",
	   (radeon_get(RADEON_DAC_CNTL,"RADEON_DAC_CNTL")&RADEON_DAC_PDWN)?"off":"on");
    printf("         light [on|off]     - power down the backlight (%s)\n",
	   (radeon_get(RADEON_LVDS_GEN_CNTL,"RADEON_LVDS_GEN_CNTL")&RADEON_LVDS_ON)?"on":"off");
    printf("         stretch [on|off|vert|horiz|auto|manual]   - stretching for resolution mismatch \n");
    printf("         regs               - show a listing of some random registers\n");
    printf("         regmatch <pattern> - show registers matching wildcard pattern\n");
    printf("         regset <pattern> <value> - set registers matching wildcard pattern\n");
    exit(-1);
}


/* Ohh, life would be good if we could simply address all memory addresses */
/* with /dev/mem, then I could write this whole program in perl, */
/* but sadly this is only the size of physical RAM.  If you */
/* want to be truely bad and poke into device memory you have to mmap() */
static unsigned char * map_devince_memory(unsigned int base,unsigned int length) 
{
    int mem_fd;
    unsigned char *device_mem;

    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR) ) < 0) {
        fatal("can't open /dev/mem\nAre you root?\n");
    }

    /* mmap graphics memory */
    if ((device_mem = malloc(length + (PAGE_SIZE-1))) == NULL) {
        fatal("allocation error \n");
    }
    if ((unsigned long)device_mem % PAGE_SIZE)
        device_mem += PAGE_SIZE - ((unsigned long)device_mem % PAGE_SIZE);
    device_mem = (unsigned char *)mmap(
        (caddr_t)device_mem, 
        length,
        PROT_READ|PROT_WRITE,
        MAP_SHARED|MAP_FIXED,
        mem_fd, 
        base
    );
    if ((long)device_mem < 0) {
        if(debug)
            fprintf(stderr,"mmap returned %d\n",(int)device_mem);
        fatal("mmap error \n");
    }
    return device_mem;
}

void radeon_cmd_regs(void)
{
	#define SHOW_REG(r) printf("%s\t%08lx\n", #r, radeon_get(r, #r))
	
	SHOW_REG(RADEON_DAC_CNTL);
	SHOW_REG(RADEON_DAC_CNTL2);
	SHOW_REG(RADEON_TV_DAC_CNTL);
	SHOW_REG(RADEON_DISP_OUTPUT_CNTL);
	SHOW_REG(RADEON_CONFIG_MEMSIZE);
	SHOW_REG(RADEON_AUX_SC_CNTL);
	SHOW_REG(RADEON_CRTC_EXT_CNTL);
	SHOW_REG(RADEON_CRTC_GEN_CNTL);
	SHOW_REG(RADEON_CRTC2_GEN_CNTL);
	SHOW_REG(RADEON_DEVICE_ID);
	SHOW_REG(RADEON_DISP_MISC_CNTL);
	SHOW_REG(RADEON_GPIO_MONID);
	SHOW_REG(RADEON_GPIO_MONIDB);
	SHOW_REG(RADEON_GPIO_CRT2_DDC);
	SHOW_REG(RADEON_GPIO_DVI_DDC);
	SHOW_REG(RADEON_GPIO_VGA_DDC);
	SHOW_REG(RADEON_LVDS_GEN_CNTL);
	SHOW_REG(RADEON_FP_GEN_CNTL);
}

#define REGLIST(r) { #r, RADEON_ ## r }
static struct {
	const char *name;
	unsigned address;
} reg_list[] = {
	REGLIST(ADAPTER_ID),
	REGLIST(AGP_BASE),
	REGLIST(AGP_CNTL),
	REGLIST(AGP_COMMAND),
	REGLIST(AGP_STATUS),
	REGLIST(AMCGPIO_A_REG),
	REGLIST(AMCGPIO_EN_REG),
	REGLIST(AMCGPIO_MASK),
	REGLIST(AMCGPIO_Y_REG),
	REGLIST(ATTRDR),
	REGLIST(ATTRDW),
	REGLIST(ATTRX),
	REGLIST(AUX_SC_CNTL),
	REGLIST(AUX1_SC_BOTTOM),
	REGLIST(AUX1_SC_LEFT),
	REGLIST(AUX1_SC_RIGHT),
	REGLIST(AUX1_SC_TOP),
	REGLIST(AUX2_SC_BOTTOM),
	REGLIST(AUX2_SC_LEFT),
	REGLIST(AUX2_SC_RIGHT),
	REGLIST(AUX2_SC_TOP),
	REGLIST(AUX3_SC_BOTTOM),
	REGLIST(AUX3_SC_LEFT),
	REGLIST(AUX3_SC_RIGHT),
	REGLIST(AUX3_SC_TOP),
	REGLIST(AUX_WINDOW_HORZ_CNTL),
	REGLIST(AUX_WINDOW_VERT_CNTL),
	REGLIST(BASE_CODE),
	REGLIST(BIOS_0_SCRATCH),
	REGLIST(BIOS_1_SCRATCH),
	REGLIST(BIOS_2_SCRATCH),
	REGLIST(BIOS_3_SCRATCH),
	REGLIST(BIOS_4_SCRATCH),
	REGLIST(BIOS_5_SCRATCH),
	REGLIST(BIOS_6_SCRATCH),
	REGLIST(BIOS_7_SCRATCH),
	REGLIST(BIOS_ROM),
	REGLIST(BIST),
	REGLIST(BUS_CNTL),
	REGLIST(BUS_CNTL1),
	REGLIST(CACHE_CNTL),
	REGLIST(CACHE_LINE),
	REGLIST(CAP0_TRIG_CNTL),
	REGLIST(CAP1_TRIG_CNTL),
	REGLIST(CAPABILITIES_ID),
	REGLIST(CAPABILITIES_PTR),
	REGLIST(CLOCK_CNTL_DATA),
	REGLIST(CLOCK_CNTL_INDEX),
	REGLIST(CLR_CMP_CLR_3D),
	REGLIST(CLR_CMP_CLR_DST),
	REGLIST(CLR_CMP_CLR_SRC),
	REGLIST(CLR_CMP_CNTL),
	REGLIST(CLR_CMP_MASK),
	REGLIST(CLR_CMP_MASK_3D),
	REGLIST(COMMAND),
	REGLIST(COMPOSITE_SHADOW_ID),
	REGLIST(CONFIG_APER_0_BASE),
	REGLIST(CONFIG_APER_1_BASE),
	REGLIST(CONFIG_APER_SIZE),
	REGLIST(CONFIG_BONDS),
	REGLIST(CONFIG_CNTL),
	REGLIST(CONFIG_MEMSIZE),
	REGLIST(CONFIG_MEMSIZE_EMBEDDED),
	REGLIST(CONFIG_REG_1_BASE),
	REGLIST(CONFIG_REG_APER_SIZE),
	REGLIST(CONFIG_XSTRAP),
	REGLIST(CONSTANT_COLOR_C),
	REGLIST(CRC_CMDFIFO_ADDR),
	REGLIST(CRC_CMDFIFO_DOUT),
	REGLIST(CRTC_CRNT_FRAME),
	REGLIST(CRTC_DEBUG),
	REGLIST(CRTC_EXT_CNTL),
	REGLIST(CRTC_EXT_CNTL_DPMS_BYTE),
	REGLIST(CRTC_GEN_CNTL),
	REGLIST(CRTC2_GEN_CNTL),
	REGLIST(CRTC_GUI_TRIG_VLINE),
	REGLIST(CRTC_H_SYNC_STRT_WID),
	REGLIST(CRTC2_H_SYNC_STRT_WID),
	REGLIST(CRTC_H_TOTAL_DISP),
	REGLIST(CRTC2_H_TOTAL_DISP),
	REGLIST(CRTC_OFFSET),
	REGLIST(CRTC2_OFFSET),
	REGLIST(CRTC_OFFSET_CNTL),
	REGLIST(CRTC2_OFFSET_CNTL),
REGLIST(CRTC_PITCH),
REGLIST(CRTC2_PITCH),
REGLIST(CRTC_STATUS),
REGLIST(CRTC_V_SYNC_STRT_WID),
REGLIST(CRTC2_V_SYNC_STRT_WID),
REGLIST(CRTC_V_TOTAL_DISP),
REGLIST(CRTC2_V_TOTAL_DISP),
REGLIST(CRTC_VLINE_CRNT_VLINE),
REGLIST(CRTC2_CRNT_FRAME),
REGLIST(CRTC2_DEBUG),
REGLIST(CRTC2_GUI_TRIG_VLINE),
REGLIST(CRTC2_STATUS),
REGLIST(CRTC2_VLINE_CRNT_VLINE),
REGLIST(CRTC8_DATA),
REGLIST(CRTC8_IDX),
REGLIST(CUR_CLR0),
REGLIST(CUR_CLR1),
REGLIST(CUR_HORZ_VERT_OFF),
REGLIST(CUR_HORZ_VERT_POSN),
REGLIST(CUR_OFFSET),
REGLIST(CUR2_CLR0),
REGLIST(CUR2_CLR1),
REGLIST(CUR2_HORZ_VERT_OFF),
REGLIST(CUR2_HORZ_VERT_POSN),
REGLIST(CUR2_OFFSET),
REGLIST(DAC_CNTL),
REGLIST(DAC_CNTL2),
REGLIST(TV_DAC_CNTL),
REGLIST(DISP_OUTPUT_CNTL),
REGLIST(DAC_CRC_SIG),
REGLIST(DAC_DATA),
REGLIST(DAC_MASK),
REGLIST(DAC_R_INDEX),
REGLIST(DAC_W_INDEX),
REGLIST(DDA_CONFIG),
REGLIST(DDA_ON_OFF),
REGLIST(DEFAULT_OFFSET),
REGLIST(DEFAULT_PITCH),
REGLIST(DEFAULT_SC_BOTTOM_RIGHT),
REGLIST(DESTINATION_3D_CLR_CMP_VAL),
REGLIST(DESTINATION_3D_CLR_CMP_MSK),
REGLIST(DEVICE_ID),
REGLIST(DISP_MISC_CNTL),
REGLIST(DP_BRUSH_BKGD_CLR),
REGLIST(DP_BRUSH_FRGD_CLR),
REGLIST(DP_CNTL),
REGLIST(DP_CNTL_XDIR_YDIR_YMAJOR),
REGLIST(DP_DATATYPE),
REGLIST(DP_GUI_MASTER_CNTL),
REGLIST(DP_GUI_MASTER_CNTL_C),
REGLIST(DP_MIX),
REGLIST(DP_SRC_BKGD_CLR),
REGLIST(DP_SRC_FRGD_CLR),
REGLIST(DP_WRITE_MASK),
REGLIST(DST_BRES_DEC),
REGLIST(DST_BRES_ERR),
REGLIST(DST_BRES_INC),
REGLIST(DST_BRES_LNTH),
REGLIST(DST_BRES_LNTH_SUB),
REGLIST(DST_HEIGHT),
REGLIST(DST_HEIGHT_WIDTH),
REGLIST(DST_HEIGHT_WIDTH_8),
REGLIST(DST_HEIGHT_WIDTH_BW),
REGLIST(DST_HEIGHT_Y),
REGLIST(DST_LINE_START),
REGLIST(DST_LINE_END),
REGLIST(DST_LINE_PATCOUNT),
REGLIST(DST_OFFSET),
REGLIST(DST_PITCH),
REGLIST(DST_PITCH_OFFSET),
REGLIST(DST_PITCH_OFFSET_C),
REGLIST(DST_WIDTH),
REGLIST(DST_WIDTH_HEIGHT),
REGLIST(DST_WIDTH_X),
REGLIST(DST_WIDTH_X_INCY),
REGLIST(DST_X),
REGLIST(DST_X_SUB),
REGLIST(DST_X_Y),
REGLIST(DST_Y),
REGLIST(DST_Y_SUB),
REGLIST(DST_Y_X),
REGLIST(FLUSH_1),
REGLIST(FLUSH_2),
REGLIST(FLUSH_3),
REGLIST(FLUSH_4),
REGLIST(FLUSH_5),
REGLIST(FLUSH_6),
REGLIST(FLUSH_7),
REGLIST(FOG_3D_TABLE_START),
REGLIST(FOG_3D_TABLE_END),
REGLIST(FOG_3D_TABLE_DENSITY),
REGLIST(FOG_TABLE_INDEX),
REGLIST(FOG_TABLE_DATA),
REGLIST(FP_CRTC_H_TOTAL_DISP),
REGLIST(FP_CRTC_V_TOTAL_DISP),
REGLIST(FP_CRTC2_H_TOTAL_DISP),
REGLIST(FP_CRTC2_V_TOTAL_DISP),
REGLIST(FP_GEN_CNTL),
REGLIST(FP2_GEN_CNTL),
REGLIST(FP_H_SYNC_STRT_WID),
REGLIST(FP_H2_SYNC_STRT_WID),
REGLIST(FP_HORZ_STRETCH),
REGLIST(FP_HORZ2_STRETCH),
REGLIST(FP_V_SYNC_STRT_WID),
REGLIST(FP_VERT_STRETCH),
REGLIST(FP_V2_SYNC_STRT_WID),
REGLIST(FP_VERT2_STRETCH),
REGLIST(GEN_INT_CNTL),
REGLIST(GEN_INT_STATUS),
REGLIST(GENENB),
REGLIST(GENFC_RD),
REGLIST(GENFC_WT),
REGLIST(GENMO_RD),
REGLIST(GENMO_WT),
REGLIST(GENS0),
REGLIST(GENS1),
REGLIST(GPIO_MONID),
REGLIST(GPIO_MONIDB),
REGLIST(GPIO_CRT2_DDC),
REGLIST(GPIO_DVI_DDC),
REGLIST(GPIO_VGA_DDC),
REGLIST(GRPH8_DATA),
REGLIST(GRPH8_IDX),
REGLIST(GUI_DEBUG0),
REGLIST(GUI_DEBUG1),
REGLIST(GUI_DEBUG2),
REGLIST(GUI_DEBUG3),
REGLIST(GUI_DEBUG4),
REGLIST(GUI_DEBUG5),
REGLIST(GUI_DEBUG6),
REGLIST(GUI_SCRATCH_REG0),
REGLIST(GUI_SCRATCH_REG1),
REGLIST(GUI_SCRATCH_REG2),
REGLIST(GUI_SCRATCH_REG3),
REGLIST(GUI_SCRATCH_REG4),
REGLIST(GUI_SCRATCH_REG5),
REGLIST(HEADER),
REGLIST(HOST_DATA0),
REGLIST(HOST_DATA1),
REGLIST(HOST_DATA2),
REGLIST(HOST_DATA3),
REGLIST(HOST_DATA4),
REGLIST(HOST_DATA5),
REGLIST(HOST_DATA6),
REGLIST(HOST_DATA7),
REGLIST(HOST_DATA_LAST),
REGLIST(HOST_PATH_CNTL),
REGLIST(HW_DEBUG),
REGLIST(HW_DEBUG2),
REGLIST(I2C_CNTL_1),
REGLIST(DVI_I2C_CNTL_1),
REGLIST(INTERRUPT_LINE),
REGLIST(INTERRUPT_PIN),
REGLIST(IO_BASE),
REGLIST(LATENCY),
REGLIST(LEAD_BRES_DEC),
REGLIST(LEAD_BRES_LNTH),
REGLIST(LEAD_BRES_LNTH_SUB),
REGLIST(LVDS_GEN_CNTL),
REGLIST(MAX_LATENCY),
REGLIST(MC_AGP_LOCATION),
REGLIST(MC_FB_LOCATION),
REGLIST(MDGPIO_A_REG),
REGLIST(MDGPIO_EN_REG),
REGLIST(MDGPIO_MASK),
REGLIST(MDGPIO_Y_REG),
REGLIST(MEM_ADDR_CONFIG),
REGLIST(MEM_BASE),
REGLIST(MEM_CNTL),
REGLIST(MEM_INIT_LAT_TIMER),
REGLIST(MEM_INTF_CNTL),
REGLIST(MEM_SDRAM_MODE_REG),
REGLIST(MEM_STR_CNTL),
REGLIST(MEM_VGA_RP_SEL),
REGLIST(MEM_VGA_WP_SEL),
REGLIST(MIN_GRANT),
REGLIST(MM_DATA),
REGLIST(MM_INDEX),
REGLIST(MPP_TB_CONFIG),
REGLIST(MPP_GP_CONFIG),
REGLIST(N_VIF_COUNT),
REGLIST(OV0_SCALE_CNTL),
REGLIST(OVR_CLR),
REGLIST(OVR_WID_LEFT_RIGHT),
REGLIST(OVR_WID_TOP_BOTTOM),
REGLIST(OV0_Y_X_START),
REGLIST(OV0_Y_X_END),
REGLIST(OV0_EXCLUSIVE_HORZ),
REGLIST(OV0_EXCLUSIVE_VERT),
REGLIST(OV0_REG_LOAD_CNTL),
REGLIST(OV0_SCALE_CNTL),
REGLIST(OV0_V_INC),
REGLIST(OV0_P1_V_ACCUM_INIT),
REGLIST(OV0_P23_V_ACCUM_INIT),
REGLIST(OV0_P1_BLANK_LINES_AT_TOP),
REGLIST(OV0_P23_BLANK_LINES_AT_TOP),
REGLIST(OV0_VID_BUF0_BASE_ADRS),
REGLIST(OV0_VID_BUF1_BASE_ADRS),
REGLIST(OV0_VID_BUF2_BASE_ADRS),
REGLIST(OV0_VID_BUF3_BASE_ADRS),
REGLIST(OV0_VID_BUF4_BASE_ADRS),
REGLIST(OV0_VID_BUF5_BASE_ADRS),
REGLIST(OV0_VID_BUF_PITCH0_VALUE),
REGLIST(OV0_VID_BUF_PITCH1_VALUE),
REGLIST(OV0_AUTO_FLIP_CNTL),
REGLIST(OV0_DEINTERLACE_PATTERN),
REGLIST(OV0_H_INC),
REGLIST(OV0_STEP_BY),
REGLIST(OV0_P1_H_ACCUM_INIT),
REGLIST(OV0_P23_H_ACCUM_INIT),
REGLIST(OV0_P1_X_START_END),
REGLIST(OV0_P2_X_START_END),
REGLIST(OV0_P3_X_START_END),
REGLIST(OV0_FILTER_CNTL),
REGLIST(OV0_FOUR_TAP_COEF_0),
REGLIST(OV0_FOUR_TAP_COEF_1),
REGLIST(OV0_FOUR_TAP_COEF_2),
REGLIST(OV0_FOUR_TAP_COEF_3),
REGLIST(OV0_FOUR_TAP_COEF_4),
REGLIST(OV0_COLOUR_CNTL),
REGLIST(OV0_VIDEO_KEY_CLR),
REGLIST(OV0_VIDEO_KEY_MSK),
REGLIST(OV0_GRAPHICS_KEY_CLR),
REGLIST(OV0_GRAPHICS_KEY_MSK),
REGLIST(OV0_KEY_CNTL),
REGLIST(OV0_TEST),
REGLIST(PALETTE_DATA),
REGLIST(PALETTE_30_DATA),
REGLIST(PALETTE_INDEX),
REGLIST(PCI_GART_PAGE),
REGLIST(PLANE_3D_MASK_C),
REGLIST(PMI_CAP_ID),
REGLIST(PMI_DATA),
REGLIST(PMI_NXT_CAP_PTR),
REGLIST(PMI_PMC_REG),
REGLIST(PMI_PMCSR_REG),
REGLIST(PMI_REGISTER),
REGLIST(PWR_MNGMT_CNTL_STATUS),
REGLIST(RBBM_SOFT_RESET),
REGLIST(RBBM_STATUS),
REGLIST(RB2D_DSTCACHE_CTLSTAT),
REGLIST(RB2D_DSTCACHE_MODE),
REGLIST(REG_BASE),
REGLIST(REGPROG_INF),
REGLIST(REVISION_ID),
REGLIST(SC_BOTTOM),
REGLIST(SC_BOTTOM_RIGHT),
REGLIST(SC_BOTTOM_RIGHT_C),
REGLIST(SC_LEFT),
REGLIST(SC_RIGHT),
REGLIST(SC_TOP),
REGLIST(SC_TOP_LEFT),
REGLIST(SC_TOP_LEFT_C),
REGLIST(SDRAM_MODE_REG),
REGLIST(SEQ8_DATA),
REGLIST(SEQ8_IDX),
REGLIST(SNAPSHOT_F_COUNT),
REGLIST(SNAPSHOT_VH_COUNTS),
REGLIST(SNAPSHOT_VIF_COUNT),
REGLIST(SRC_OFFSET),
REGLIST(SRC_PITCH),
REGLIST(SRC_PITCH_OFFSET),
REGLIST(SRC_SC_BOTTOM),
REGLIST(SRC_SC_BOTTOM_RIGHT),
REGLIST(SRC_SC_RIGHT),
REGLIST(SRC_X),
REGLIST(SRC_X_Y),
REGLIST(SRC_Y),
REGLIST(SRC_Y_X),
REGLIST(STATUS),
REGLIST(SUBPIC_CNTL),
REGLIST(SUB_CLASS),
REGLIST(SURFACE_CNTL),
REGLIST(SURFACE0_INFO),
REGLIST(SURFACE0_LOWER_BOUND),
REGLIST(SURFACE0_UPPER_BOUND),
REGLIST(SURFACE1_INFO),
REGLIST(SURFACE1_LOWER_BOUND),
REGLIST(SURFACE1_UPPER_BOUND),
REGLIST(SURFACE2_INFO),
REGLIST(SURFACE2_LOWER_BOUND),
REGLIST(SURFACE2_UPPER_BOUND),
REGLIST(SURFACE3_INFO),
REGLIST(SURFACE3_LOWER_BOUND),
REGLIST(SURFACE3_UPPER_BOUND),
REGLIST(SURFACE4_INFO),
REGLIST(SURFACE4_LOWER_BOUND),
REGLIST(SURFACE4_UPPER_BOUND),
REGLIST(SURFACE5_INFO),
REGLIST(SURFACE5_LOWER_BOUND),
REGLIST(SURFACE5_UPPER_BOUND),
REGLIST(SURFACE6_INFO),
REGLIST(SURFACE6_LOWER_BOUND),
REGLIST(SURFACE6_UPPER_BOUND),
REGLIST(SURFACE7_INFO),
REGLIST(SURFACE7_LOWER_BOUND),
REGLIST(SURFACE7_UPPER_BOUND),
REGLIST(SW_SEMAPHORE),
REGLIST(TEST_DEBUG_CNTL),
REGLIST(TEST_DEBUG_MUX),
REGLIST(TEST_DEBUG_OUT),
REGLIST(TMDS_CRC),
REGLIST(TRAIL_BRES_DEC),
REGLIST(TRAIL_BRES_ERR),
REGLIST(TRAIL_BRES_INC),
REGLIST(TRAIL_X),
REGLIST(TRAIL_X_SUB),
REGLIST(PIXCLKS_CNTL),
REGLIST(VENDOR_ID),
REGLIST(VGA_DDA_CONFIG),
REGLIST(VGA_DDA_ON_OFF),
REGLIST(VID_BUFFER_CONTROL),
REGLIST(VIDEOMUX_CNTL),
REGLIST(VIPH_CONTROL),
REGLIST(WAIT_UNTIL),
REGLIST(RB3D_BLENDCNTL),
REGLIST(RB3D_CNTL),
REGLIST(RB3D_COLOROFFSET),
REGLIST(RB3D_COLORPITCH),
REGLIST(RB3D_DEPTHOFFSET),
REGLIST(RB3D_DEPTHPITCH),
REGLIST(RB3D_PLANEMASK),
REGLIST(RB3D_ROPCNTL),
REGLIST(RB3D_STENCILREFMASK),
REGLIST(RB3D_ZSTENCILCNTL),
REGLIST(RE_LINE_PATTERN),
REGLIST(RE_LINE_STATE),
REGLIST(RE_MISC),
REGLIST(RE_SOLID_COLOR),
REGLIST(RE_TOP_LEFT),
	REGLIST(LVDS_PLL_CNTL),
	REGLIST(TMDS_PLL_CNTL),
	REGLIST(TMDS_TRANSMITTER_CNTL),
REGLIST(RE_WIDTH_HEIGHT)
};

void radeon_reg_match(const char *pattern)
{
	int i;
	for (i=0;i<sizeof(reg_list)/sizeof(reg_list[0]);i++) {
		if (fnmatch(pattern, reg_list[i].name, 0) == 0) {
			printf("%s\t0x%08lx\n", 
			       reg_list[i].name, 
			       radeon_get(reg_list[i].address, reg_list[i].name));
		}
	}
}

void radeon_reg_set(const char *name, unsigned value)
{
	int i;
	for (i=0;i<sizeof(reg_list)/sizeof(reg_list[0]);i++) {
		if (fnmatch(name, reg_list[i].name, 0) == 0) {
			const char *name = reg_list[i].name;
			unsigned address = reg_list[i].address;
			printf("OLD: %s\t0x%08lx\n", name, radeon_get(address, name));
			radeon_set(address, name, value);
			printf("NEW: %s\t0x%08lx\n", name, radeon_get(address, name));
		}
	}
}

void radeon_cmd_bits(void)
{
    unsigned long dac_cntl;

    dac_cntl = radeon_get(RADEON_DAC_CNTL,"RADEON_DAC_CNTL");  
    printf("RADEON_DAC_CNTL=%08lx (",dac_cntl);  
    if(dac_cntl & RADEON_DAC_RANGE_CNTL)
        printf("range_cntl ");  
    if(dac_cntl & RADEON_DAC_BLANKING)
        printf("blanking ");  
    if(dac_cntl & RADEON_DAC_8BIT_EN)
        printf("8bit_en ");  
    if(dac_cntl & RADEON_DAC_VGA_ADR_EN)
        printf("vga_adr_en ");  
    if(dac_cntl & RADEON_DAC_PDWN)
        printf("pdwn ");  
    printf(")\n");  
}

void radeon_cmd_dac(char *param)
{
    unsigned long dac_cntl;

    dac_cntl = radeon_get(RADEON_DAC_CNTL,"RADEON_DAC_CNTL");
    if(param == NULL) {
        printf("The radeon external DAC looks %s\n",(dac_cntl&(RADEON_DAC_PDWN))?"off":"on");
        exit (-1);
    } else if(strcmp(param,"off") == 0) {
        dac_cntl |= RADEON_DAC_PDWN;
    } else if(strcmp(param,"on") == 0) {
        dac_cntl &= ~ RADEON_DAC_PDWN;
    } else {
        usage();	    
    };
    radeon_set(RADEON_DAC_CNTL,"RADEON_DAC_CNTL",dac_cntl);
}

void radeon_cmd_light(char *param)
{
    unsigned long lvds_gen_cntl;

    lvds_gen_cntl = radeon_get(RADEON_LVDS_GEN_CNTL,"RADEON_LVDS_GEN_CNTL");
    if(param == NULL) {
        printf("The radeon backlight looks %s\n",(lvds_gen_cntl&(RADEON_LVDS_ON))?"on":"off");
        exit (-1);
    } else if(strcmp(param,"on") == 0) {
        lvds_gen_cntl |= RADEON_LVDS_ON;
    } else if(strcmp(param,"off") == 0) {
        lvds_gen_cntl &= ~ RADEON_LVDS_ON;
    } else {
        usage();	    
    };
    radeon_set(RADEON_LVDS_GEN_CNTL,"RADEON_LVDS_GEN_CNTL",lvds_gen_cntl);
}

void radeon_cmd_stretch(char *param)
{
    unsigned long fp_vert_stretch,fp_horz_stretch;

    fp_vert_stretch = radeon_get(RADEON_FP_VERT_STRETCH,"RADEON_FP_VERT_STRETCH");
    fp_horz_stretch = radeon_get(RADEON_FP_HORZ_STRETCH,"RADEON_FP_HORZ_STRETCH");
    if(param == NULL) {
        printf("The horizontal stretching looks %s\n",(fp_horz_stretch&(RADEON_HORZ_STRETCH_ENABLE))?"on":"off");
        printf("The vertical stretching looks %s\n",(fp_vert_stretch&(RADEON_VERT_STRETCH_ENABLE))?"on":"off");
        exit (-1);
    } else if(strncmp(param,"ver",3) == 0) {
        fp_horz_stretch &= ~ RADEON_HORZ_STRETCH_ENABLE;
        fp_vert_stretch |= RADEON_VERT_STRETCH_ENABLE;
    } else if(strncmp(param,"hor",3) == 0) {
        fp_horz_stretch |= RADEON_HORZ_STRETCH_ENABLE;
        fp_vert_stretch &= ~ RADEON_VERT_STRETCH_ENABLE;
    } else if(strcmp(param,"on") == 0) {
        fp_horz_stretch |= RADEON_HORZ_STRETCH_ENABLE;
        fp_vert_stretch |= RADEON_VERT_STRETCH_ENABLE;
    } else if(strcmp(param,"auto") == 0) {
        fp_horz_stretch |= RADEON_HORZ_AUTO_RATIO;
        fp_horz_stretch |= RADEON_HORZ_AUTO_RATIO_INC;
        fp_horz_stretch |= RADEON_HORZ_STRETCH_ENABLE;
        fp_vert_stretch |= RADEON_VERT_AUTO_RATIO_EN;
        fp_vert_stretch |= RADEON_VERT_STRETCH_ENABLE;
    } else if(strcmp(param,"manual") == 0) {
        fp_horz_stretch &= ~ RADEON_HORZ_AUTO_RATIO;
        fp_horz_stretch &= ~ RADEON_HORZ_AUTO_RATIO_INC;
        fp_horz_stretch |= RADEON_HORZ_STRETCH_ENABLE;
        fp_vert_stretch &= ~ RADEON_VERT_AUTO_RATIO_EN;
        fp_vert_stretch |= RADEON_VERT_STRETCH_ENABLE;
    } else if(strcmp(param,"off") == 0) {
        fp_horz_stretch &= ~ RADEON_HORZ_STRETCH_ENABLE;
        fp_vert_stretch &= ~ RADEON_VERT_STRETCH_ENABLE;
    } else {
        usage();	    
    };
    radeon_set(RADEON_FP_HORZ_STRETCH,"RADEON_FP_HORZ_STRETCH",fp_horz_stretch);
    radeon_set(RADEON_FP_VERT_STRETCH,"RADEON_FP_VERT_STRETCH",fp_vert_stretch);
}


/* Here we fork() and exec() the lspci command to look for the Radeon hardware address. */
static void map_radeon_cntl_mem(void)
{
    int pipefd[2];
    int forkrc;
    FILE *fp;
    char line[1000];
    int base;

    if(pipe(pipefd)) {
        fatal("pipe failure\n");
    }
    forkrc = fork();
    if(forkrc == -1) {
        fatal("fork failure\n");
    } else if(forkrc == 0) { /* if child */
        close(pipefd[0]);
        dup2(pipefd[1],1);  /* stdout */
        setenv("PATH","/sbin:/usr/sbin:/bin:/usr/bin",1);
        execlp("lspci","lspci","-v",NULL);
        fatal("exec lspci failure\n");
    }
    close(pipefd[1]);
    fp = fdopen(pipefd[0],"r");
    if(fp == NULL) {
        fatal("fdopen error\n");
    }
#if 0
  This is an example output of "lspci -v" ...

00:1f.6 Modem: Intel Corp. 82801CA/CAM AC 97 Modem (rev 01) (prog-if 00 [Generic])
	Subsystem: PCTel Inc: Unknown device 4c21
	Flags: bus master, medium devsel, latency 0, IRQ 11
	I/O ports at d400 [size=256]
	I/O ports at dc00 [size=128]

01:00.0 VGA compatible controller: ATI Technologies Inc Radeon Mobility M6 LY (prog-if 00 [VGA])
	Subsystem: Dell Computer Corporation: Unknown device 00e3
	Flags: bus master, VGA palette snoop, stepping, 66Mhz, medium devsel, latency 32, IRQ 11
	Memory at e0000000 (32-bit, prefetchable) [size=128M]
	I/O ports at c000 [size=256]
	Memory at fcff0000 (32-bit, non-prefetchable) [size=64K]
	Expansion ROM at <unassigned> [disabled] [size=128K]
	Capabilities: <available only to root>

02:00.0 Ethernet controller: 3Com Corporation 3c905C-TX/TX-M [Tornado] (rev 78)
	Subsystem: Dell Computer Corporation: Unknown device 00e3
	Flags: bus master, medium devsel, latency 32, IRQ 11
	I/O ports at ec80 [size=128]
	Memory at f8fffc00 (32-bit, non-prefetchable) [size=128]
	Expansion ROM at f9000000 [disabled] [size=128K]
	Capabilities: <available only to root>

We need to look through it to find the smaller region base address f8fffc00.

#endif
    while(1) { /* for every line up to the "Radeon" string */
       if(fgets(line,sizeof(line),fp) == NULL) {  /* if end of file */
          fatal("Radeon hardware not found in lspci output.\n");
       }
       if(strstr(line,"Radeon") || strstr(line,"ATI Tech")) {  /* if line contains a "radeon" string */
          if(skip-- < 1) {
             break;
          }
       }
    };
    if(debug) 
       printf("%s",line);
    while(1) { /* for every line up till memory statement */
       if(fgets(line,sizeof(line),fp) == NULL || line[0] != '\t') {  /* if end of file */
          fatal("Radeon control memory not found.\n");
       }
       if(debug) 
          printf("%s",line);
       if(strstr(line,"emory") && strstr(line,"K")) {  /* if line contains a "Memory" and "K" string */
          break;
       }
    };
    if(sscanf(line,"%*s%*s%x",&base) == 0) { /* third token as hex number */
       fatal("parse error of lspci output (control memory not found)\n");
    }
    if(debug)
        printf("Radeon found. Base control address is %x.\n",base);
    radeon_cntl_mem = map_devince_memory(base,0x2000);
}

int main(int argc,char *argv[]) 
{
    if(argc == 1) {
        map_radeon_cntl_mem();
	usage();
    }
    if(strcmp(argv[1],"--debug") == 0) {
        debug=1;
        argv++; argc--;
    };
    if(strcmp(argv[1],"--skip=") == 0) {
        skip=atoi(argv[1]+7);
        argv++; argc--;
    };
    map_radeon_cntl_mem();
    if(argc == 2) {
        if(strcmp(argv[1],"regs") == 0) {
            radeon_cmd_regs();
            return 0;
        } else if(strcmp(argv[1],"bits") == 0) {
            radeon_cmd_bits();
            return 0;
        } else if(strcmp(argv[1],"dac") == 0) {
            radeon_cmd_dac(NULL);
            return 0;
        } else if(strcmp(argv[1],"light") == 0) {
            radeon_cmd_light(NULL);
            return 0;
        } else if(strcmp(argv[1],"stretch") == 0) {
            radeon_cmd_stretch(NULL);
            return 0;
        };
    } else if(argc == 3) {
        if(strcmp(argv[1],"dac") == 0) {
            radeon_cmd_dac(argv[2]);
            return 0;
        } else if(strcmp(argv[1],"light") == 0) {
            radeon_cmd_light(argv[2]);
            return 0;
        } else if(strcmp(argv[1],"stretch") == 0) {
            radeon_cmd_stretch(argv[2]);
            return 0;
        } else if(strcmp(argv[1],"regmatch") == 0) {
            radeon_reg_match(argv[2]);
            return 0;
        };
    } else if(argc == 4) {
	    if(strcmp(argv[1],"regset") == 0) {
		    radeon_reg_set(argv[2], strtoul(argv[3], NULL, 0));
		    return 0;
	    }
    }

    usage();
    return 1;
}



