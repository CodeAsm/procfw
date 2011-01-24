#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pspsdk.h>
#include <pspthreadman_kernel.h>
#include "systemctrl.h"
#include "systemctrl_se.h"
#include "systemctrl_private.h"
#include "isoreader.h"
#include "printk.h"
#include "utils.h"
#include "strsafe.h"
#include "virtual_pbp.h"

typedef struct __attribute__((packed))
{
	u32 signature;
	u32 version;
	u32 fields_table_offs;
	u32 values_table_offs;
	int nitems;
} SFOHeader;

typedef struct __attribute__((packed))
{
	u16 field_offs;
	u8  unk;
	u8  type; // 0x2 -> string, 0x4 -> number
	u32 unk2;
	u32 unk3;
	u16 val_offs;
	u16 unk4;
} SFODir;

typedef struct _PBPEntry {
	u32 enabled;
	char *name;
} PBPEntry;

static PBPEntry pbp_entries[8] = {
	{ 1, "PARAM.SFO" },
	{ 1, "ICON0.PNG" },
	{ 1, "ICON1.PMF" },
	{ 1, "PIC0.PNG"  },
	{ 1, "PIC1.PNG"  },
	{ 1, "SND0.AT3"  },
	{ 0, "DATA.PSP"  }, // never enable it
	{ 0, "DATA.PSAR" }, // never enable it
};

static u8 virtualsfo[408] = {
	0x00, 0x50, 0x53, 0x46, 0x01, 0x01, 0x00, 0x00,
	0x94, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 
	0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 
	0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x04, 0x02, 
	0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x12, 0x00, 0x04, 0x02, 
	0x0A, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 
	0x08, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x04, 0x02, 
	0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
	0x18, 0x00, 0x00, 0x00, 0x27, 0x00, 0x04, 0x04, 
	0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 
	0x20, 0x00, 0x00, 0x00, 0x36, 0x00, 0x04, 0x02,
	0x05, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 
	0x24, 0x00, 0x00, 0x00, 0x45, 0x00, 0x04, 0x04, 
	0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 
	0x2C, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x04, 0x02, 
	0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
	0x30, 0x00, 0x00, 0x00, 0x42, 0x4F, 0x4F, 0x54, 
	0x41, 0x42, 0x4C, 0x45, 0x00, 0x43, 0x41, 0x54, 
	0x45, 0x47, 0x4F, 0x52, 0x59, 0x00, 0x44, 0x49, 
	0x53, 0x43, 0x5F, 0x49, 0x44, 0x00, 0x44, 0x49, 
	0x53, 0x43, 0x5F, 0x56, 0x45, 0x52, 0x53, 0x49,
	0x4F, 0x4E, 0x00, 0x50, 0x41, 0x52, 0x45, 0x4E, 
	0x54, 0x41, 0x4C, 0x5F, 0x4C, 0x45, 0x56, 0x45, 
	0x4C, 0x00, 0x50, 0x53, 0x50, 0x5F, 0x53, 0x59, 
	0x53, 0x54, 0x45, 0x4D, 0x5F, 0x56, 0x45, 0x52, 
	0x00, 0x52, 0x45, 0x47, 0x49, 0x4F, 0x4E, 0x00, 
	0x54, 0x49, 0x54, 0x4C, 0x45, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x4D, 0x47, 0x00, 0x00, 
	0x55, 0x43, 0x4A, 0x53, 0x31, 0x30, 0x30, 0x34, 
	0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x31, 0x2E, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x31, 0x2E, 0x30, 0x30, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
	0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 
	0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 
	0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 
	0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
	0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 
	0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

VirtualPBP vpbps[MAX_VPBP];
static int g_sema = -1;

static int is_iso(SceIoDirent * dir)
{
	//result
	int result = 0;

	//grab extension
	char * ext = dir->d_name + strlen(dir->d_name) - 3;

	//filename length check
	if (ext > dir->d_name) {
		//check extension
		if (stricmp(ext, "iso") == 0 /* || stricmp(ext, "cso") */) {
			//valid iso detected (more checks can be added here lateron)
			result = 1;
		}
	}

	return result;
}

typedef struct _FakePath2ISOArg {
	const char *file;
	char *newfn;
	int result;
} FakePath2ISOArg;

//translate virtual eboot to iso path
static void _fakepath_2_iso(void *parg)
{
	FakePath2ISOArg *arg = parg;
	char *p;
	u32 isoindex;
	SceUID isodfd;
	SceIoDirent dir;
	char isopath[] = "ms0:/ISO";
	const char *file = arg->file;
	char *newfn = arg->newfn;

	if (!ISOEBOOT(file)) {
		arg->result = -7;

		return;
	}

	strncpy(isopath, file, 2);
	p = strstr(file, "ISOGAME");

	if (p == NULL) {
		arg->result = -9;

		return;
	}

	p += sizeof("ISOGAME")-1;
	isoindex = strtoul(p, NULL, 16);
	isodfd = sceIoDopen(isopath);

	if (isodfd >= 0) {
		int i = -1, ret;

		do {
			memset(&dir, 0, sizeof(dir));
			ret = sceIoDread(isodfd, &dir);

			if (is_iso(&dir)) {
				i++;
			}
		} while(ret > 0 && i != isoindex);

		//close iso directory
		sceIoDclose(isodfd);

		if (i == isoindex) {
			//found iso
			sprintf(newfn, "%s/%s", isopath, dir.d_name);

			arg->result = 0;
			return;
		}
	}
	
	arg->result = -10;

	return;
}

static int fakepath_2_iso(const char * file, char newfn[128])
{
	int ret;

	FakePath2ISOArg arg;

	arg.file = file;
	arg.newfn = newfn;
	arg.result = 0;
	ret = sceKernelExtendKernelStack(0x1000, &_fakepath_2_iso, &arg);

	return arg.result;
}

static int find_enabled_vpbp(const char *file)
{
	int i, ret;
	char isopath[128];

	ret = fakepath_2_iso((char *)file, isopath);

	if (ret < 0) {
		return -11;
	}

	for(i=0; i<NELEMS(vpbps); ++i) {
		if (vpbps[i].enabled && 0 == strcmp(isopath, vpbps[i].name)) {
			return i;
		}
	}

	return -1;
}

static int find_disabled_vpbp(void)
{
	int i;

	for(i=0; i<NELEMS(vpbps); ++i) {
		if (!vpbps[i].enabled) {
			return i;
		}
	}

	return -1;
}

static inline void lock(void)
{
	sceKernelWaitSema(g_sema, 1, 0);
}

static inline void unlock(void)
{
	sceKernelSignalSema(g_sema, 1);
}

static VirtualPBP* get_vpbp_by_fd(SceUID fd)
{
	fd -= MAGIC_VPBP_FD;

	if (fd < 0 || fd >= NELEMS(vpbps)) {
		return NULL;
	}

	return &vpbps[fd];
}

static void get_sfo_title(char *title, int n, char *sfo)
{
	SFOHeader *header = (SFOHeader *)sfo;
	SFODir *entries = (SFODir *)(sfo+0x14);
	int i;

	for (i = 0; i < header->nitems; i++) {
		if (0 == strcmp(sfo+header->fields_table_offs+entries[i].field_offs, "TITLE")) {
			memset(title, 0, n);
			strncpy(title, sfo+header->values_table_offs+entries[i].val_offs, n);
		}
	}
}

static int disable_cache(int idx)
{
	VirtualPBP *vpbp;
	
	if (idx < 0 || idx >= NELEMS(vpbps)) {
		unlock();

		return -15;
	}

	vpbp = &vpbps[idx];

	if (!vpbp->opened && vpbp->enabled) {
		vpbp->enabled = 0;
	}

	return 0;
}

int vpbp_init(void)
{
	g_sema = sceKernelCreateSema("VPBPSema", 0, 1, 1, NULL);

	if (g_sema < 0)
		return g_sema;

	memset(&vpbps, 0, sizeof(vpbps));

	return 0;
}

SceUID vpbp_open(const char * file, int flags, SceMode mode)
{
	int fd, i, ret;
	VirtualPBP *vpbp;
	u32 off;
	char iso_fn[128];

	lock();
	fd = find_enabled_vpbp(file);

	if (fd >= 0) {
		vpbp = &vpbps[fd];
		vpbp->file_pointer = 0;
		unlock();

		return MAGIC_VPBP_FD+fd;
	}

	fd = find_disabled_vpbp();

	if (fd < 0) {
		unlock();

		return -2;
	}

	vpbp = &vpbps[fd];
	memset(vpbp, 0, sizeof(*vpbp));
	vpbp->opened = 1;
	vpbp->enabled = 1;
	vpbp->file_pointer = 0;

	if (flags & (PSP_O_WRONLY | PSP_O_TRUNC | PSP_O_CREAT) || !(flags & PSP_O_RDONLY)) {
		printk("%s: bad flags 0x%08X\n", __func__, flags);
		unlock();

		return -6;
	}

	ret = fakepath_2_iso((char *)file, iso_fn);

	if (ret < 0) {
		unlock();

		return -12;
	}

	printk("%s: %s\n", __func__, iso_fn);
	STRCPY_S(vpbp->name, iso_fn);
	vpbp->header[0] = 0x50425000; // PBP magic
	vpbp->header[1] = 0x10000; // version

	// fill vpbp offsets
	off = 0x28;

	isoSetFile(vpbp->name);

	for(i=0; i<NELEMS(pbp_entries); ++i) {
		vpbp->header[i+2] = off;

		if (pbp_entries[i].enabled) {
			PBPSection *sec = &vpbp->sects[i];

			sec->lba = isoGetFileInfo(pbp_entries[i].name, &sec->size);

			if (i == 0) {
				off += sizeof(virtualsfo);
			} else {
				off += sec->size;
			}
		}
	}

	vpbp->total_size = vpbp->header[9];
	unlock();

	return MAGIC_VPBP_FD+fd;
}

SceOff vpbp_lseek(SceUID fd, SceOff offset, int whence)
{
	VirtualPBP *vpbp;

	lock();
	vpbp = get_vpbp_by_fd(fd);
	
	if (vpbp == NULL) {
		printk("%s: unknown fd 0x%08X\n", __func__, fd);
		unlock();

		return -3;
	}

	switch(whence) {
		case PSP_SEEK_SET:
			vpbp->file_pointer = (int)offset;
			break;
		case PSP_SEEK_CUR:
			vpbp->file_pointer += (int)offset;
			break;
		case PSP_SEEK_END:
			vpbp->file_pointer = vpbp->total_size + (int)offset;
			break;
		default:
			break;
	}

	unlock();

	return vpbp->file_pointer;
}

int vpbp_read(SceUID fd, void * data, SceSize size)
{
	VirtualPBP *vpbp;
	int remaining;
	
	lock();
	vpbp = get_vpbp_by_fd(fd);
	
	if (vpbp == NULL) {
		printk("%s: unknown fd 0x%08X\n", __func__, fd);
		unlock();

		return -4;
	}

	isoSetFile(vpbp->name);
	remaining = size;

	while(remaining > 0) {
		if (vpbp->file_pointer >= 0 && vpbp->file_pointer < vpbp->header[2]) {
			int re;

			re = MIN(remaining, vpbp->header[2] - vpbp->file_pointer);
			memcpy(data, vpbp->header+vpbp->file_pointer, re);
			vpbp->file_pointer += re;
			data += re;
			remaining -= re;
		}

		if (vpbp->file_pointer >= vpbp->header[2] && vpbp->file_pointer < vpbp->header[3]) {
			// use own custom sfo
			int re;
			void *buf, *buf_64;
			char sfotitle[64];

			buf = oe_malloc(SECTOR_SIZE+64);

			if (buf != NULL) {
				buf_64 = PTR_ALIGN_64(buf);
				isoReadRawData(buf_64, vpbp->sects[0].lba, 0, SECTOR_SIZE);
				get_sfo_title(sfotitle, 64, buf_64);
				oe_free(buf);
				memcpy(virtualsfo+0x118, sfotitle, 64);
				re = MIN(remaining, sizeof(virtualsfo) - (vpbp->file_pointer - vpbp->header[2]));
				memcpy(data, virtualsfo+vpbp->file_pointer-vpbp->header[2], re);
				vpbp->file_pointer += re;
				data += re;
				remaining -= re;
			} else {
				unlock();

				return -13;
			}
		}

		// ignore last two sections
		int i; for(i=1; i<NELEMS(pbp_entries)-2; ++i) {
			if (vpbp->file_pointer >= vpbp->header[2+i] && vpbp->file_pointer < vpbp->header[2+1+i]) {
				void *buf, *buf_64;

				buf = oe_malloc(8*SECTOR_SIZE+64);

				if (buf != NULL) {
					int rest, pos, re, total;

					pos = vpbp->file_pointer - vpbp->header[2+i];
					rest = total = MIN(remaining, vpbp->sects[i].size - pos);
					buf_64 = PTR_ALIGN_64(buf);

					while (rest > 0) {
						re = MIN(rest, 8*SECTOR_SIZE);
						isoReadRawData(buf_64, vpbp->sects[i].lba, pos, re);
						memcpy(data, buf_64, re);
						rest -= re;
						pos += re;
						vpbp->file_pointer += re;
						data += re;
						remaining -= re;
					}

					oe_free(buf);
				} else {
					unlock();

					return -5;
				}
			}
		}

		if (vpbp->file_pointer >= vpbp->total_size)
			break;
	}

	unlock();
	return size - remaining;
}

int vpbp_close(SceUID fd)
{
	VirtualPBP *vpbp;
	
	lock();
	vpbp = get_vpbp_by_fd(fd);

	if (vpbp == NULL) {
		printk("%s: unknown fd 0x%08X\n", __func__, fd);
		unlock();

		return -7;
	}

	vpbp->opened = 0;
	unlock();

	return 0;
}

int vpbp_disable_all_caches(void)
{
	lock();

	int i; for(i=0; i<NELEMS(vpbps); ++i) {
		VirtualPBP *vpbp;
		
		vpbp = &vpbps[i];

		if (!vpbp->opened && vpbp->enabled) {
			vpbp->enabled = 0;
		}
	}

	unlock();

	return 0;
}

int vpbp_remove(const char * file)
{
	int ret;
	char iso_fn[128];

	lock();
	ret = fakepath_2_iso((char *)file, iso_fn);

	if (ret < 0) {
		unlock();

		return -14;
	}

	ret = sceIoRemove(iso_fn);

	int i; for(i=0; i<NELEMS(vpbps); ++i) {
		if (0 == strcmp(vpbps[i].name, iso_fn)) {
			disable_cache(i);
			break;
		}
	}

	unlock();

	return ret;
}

int vpbp_is_fd(SceUID fd)
{
	VirtualPBP *vpbp;
	int ret;
	
	lock();
	vpbp = get_vpbp_by_fd(fd);

	if (vpbp != NULL)
		ret = 1;
	else
		ret = 0;
	unlock();

	return ret;
}

int vpbp_getstat(const char * file, SceIoStat * stat)
{
	int ret;
	char iso_fn[128];

	lock();
	ret = fakepath_2_iso((char *)file, iso_fn);

	if (ret < 0) {
		unlock();
		return ret;
	}

	ret = sceIoGetstat(iso_fn, stat);
	unlock();

	return ret;
}

int vpbp_loadexec(char * file, struct SceKernelLoadExecVSHParam * param)
{
	int ret;
	char iso_fn[128];
	SEConfig config;

	lock();
	ret = fakepath_2_iso((char *)file, iso_fn);

	if (ret < 0) {
		unlock();
		return ret;
	}

	//set iso file for reboot
	sctrlSESetUmdFile(iso_fn);

	sctrlSEGetConfig(&config);
	//set iso mode for reboot
	sctrlSESetBootConfFileIndex(config.umdmode);

	//full memory doesn't hurt on isos
	sctrlHENSetMemory(48, 0);
	printk("%s: ISO %s, UMD mode %d\n", __func__, iso_fn, config.umdmode);
	
	//reset and configure reboot parameter
	memset(param, 0, sizeof(param));
	param->size = sizeof(param);
	param->argp = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN";
	param->args = strlen(param->argp) + 1;
	param->key = "game";

	//fix apitypes
	int apitype = 0x120;
	if (sceKernelGetModel() == PSP_GO) apitype = 0x125;

	//start game image
	return sctrlKernelLoadExecVSHWithApitype(apitype, param->argp, param);

	unlock();

	return ret;
}