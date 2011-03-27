#include <pspkernel.h>
#include <pspreg.h>
#include <stdio.h>
#include <string.h>
#include <systemctrl.h>
#include <systemctrl_se.h>
#include <pspsysmem_kernel.h>
#include <psprtc.h>
#include "utils.h"
#include "printk.h"
#include "libs.h"
#include "utils.h"
#include "systemctrl.h"
#include "systemctrl_se.h"
#include "systemctrl_private.h"
#include "inferno.h"

PSP_MODULE_INFO("PRO_Inferno_Driver", 0x1000, 1, 1);

u32 psp_model;
u32 psp_fw_version;

extern int SysMemForKernel_C7E57B9C(void *unk0);
extern char *GetUmdFile();

// 00002790
const char *g_iso_fn = NULL;

// 0x000027A8
SceUID g_mediaman_semaid = -1;

// 0x00002248
u8 g_umddata[16] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

// 00000090
int setup_umd_device(void)
{
	int ret;

	g_iso_fn = GetUmdFile();
	ret = sceIoAddDrv(&g_iodrv);

	if(ret < 0) {
		return ret;
	}

	SysMemForKernel_C7E57B9C(g_umddata);
	ret = 0;

	return ret;
}

// 00001514
int init_march33(void)
{
	g_drive_status = 50;
	g_umd_cbid = -1;
	g_umd_error_status = 0;
	g_mediaman_semaid = sceKernelCreateSema("MediaManSema", 0, 0, 1, 0);

	return MIN(g_mediaman_semaid, 0);
}

// 0x00000000
int module_start(SceSize args, void* argp)
{
	int ret;

	psp_model = sceKernelGetModel();
	psp_fw_version = sceKernelDevkitVersion();
	setup_patch_offset_table(psp_fw_version);
	printk_init("ms0:/march33_reverse.txt");
	printk("March33 reversed started FW=0x%08X %02dg\n", psp_fw_version, psp_model+1);

	ret = setup_umd_device();

	if(ret < 0) {
		return ret;
	}

	ret = init_march33();

	return MIN(ret, 0);
}

// 0x0000006C
int module_stop(SceSize args, void *argp)
{
	sceIoDelDrv("umd");

	return 0;
}