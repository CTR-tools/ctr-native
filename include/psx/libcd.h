/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/include/psx/libcd.h
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#ifndef LIBCD_H
#define LIBCD_H

#include <macros.h>

#define DECODE_BCD(x)    (((x) >> 4) * 10 + ((x) & 0xF))
#define ENCODE_BCD(x)    ((((x) / 10) << 4) | ((x) % 10))

#define CdlModeStream    0x100 /* Normal Streaming                     */
#define CdlModeStream2   0x120 /* SUB HEADER information includes      */
#define CdlModeSpeed     0x80  /* 0: normal speed	1: double speed	*/
#define CdlModeRT        0x40  /* 0: ADPCM off		1: ADPCM on	*/
#define CdlModeSize1     0x20  /* 0: 2048 byte		1: 2340byte	*/
#define CdlModeSize0     0x10  /* 0: -			1: 2328byte	*/
#define CdlModeSF        0x08  /* 0: Channel off	1: Channel on	*/
#define CdlModeRept      0x04  /* 0: Report off	1: Report on	*/
#define CdlModeAP        0x02  /* 0: AutoPause off	1: AutoPause on */
#define CdlModeDA        0x01  /* 0: CD-DA off		1: CD-DA on	*/

/*
 * Status Contents
 */
#define CdlStatPlay      0x80 /* playing CD-DA */
#define CdlStatSeek      0x40 /* seeking */
#define CdlStatRead      0x20 /* reading data sectors */
#define CdlStatShellOpen 0x10 /* once shell open */
#define CdlStatSeekError 0x04 /* seek error detected */
#define CdlStatStandby   0x02 /* spindle motor rotating */
#define CdlStatError     0x01 /* command error detected */

/*
 * Macros for CdGetDiskType()
 */
#define CdlStatNoDisk    0
#define CdlOtherFormat   1
#define CdlCdromFormat   2

/*
 * CD-ROM Primitive Commands
 */
#define CdlNop           0x01
#define CdlSetloc        0x02
#define CdlPlay          0x03
#define CdlForward       0x04
#define CdlBackward      0x05
#define CdlReadN         0x06
#define CdlStandby       0x07
#define CdlStop          0x08
#define CdlPause         0x09
#define CdlMute          0x0b
#define CdlDemute        0x0c
#define CdlSetfilter     0x0d
#define CdlSetmode       0x0e
#define CdlGetparam      0x0f
#define CdlGetlocL       0x10
#define CdlGetlocP       0x11
#define CdlGetTN         0x13
#define CdlGetTD         0x14
#define CdlSeekL         0x15
#define CdlSeekP         0x16
#define CdlReadS         0x1B

/*
 * Interrupts
 */
#define CdlNoIntr        0x00 /* No interrupt */
#define CdlDataReady     0x01 /* Data Ready */
#define CdlComplete      0x02 /* Command Complete */
#define CdlAcknowledge   0x03 /* Acknowledge (reserved) */
#define CdlDataEnd       0x04 /* End of Data Detected */
#define CdlDiskError     0x05 /* Error Detected */

/*
 * Library Macros
 */
#ifndef btoi
#define btoi(b) ((b) / 16 * 10 + (b) % 16) /* BCD to u8 */
#endif
#ifndef itob
#define itob(i) ((i) / 10 * 16 + (i) % 10) /* u8 to BCD */
#endif

#define CdSeekL(p)   CdControl(CdlSeekL, (u8 *)p, 0)
#define CdSeekP(p)   CdControl(CdlSeekP, (u8 *)p, 0)
#define CdStandby()  CdControl(CdlStandby, 0, 0)
#define CdPause()    CdControl(CdlPause, 0, 0)
#define CdStop()     CdControl(CdlStop, 0, 0)
#define CdMute()     CdControl(CdlMute, 0, 0)
#define CdDeMute()   CdControl(CdlDemute, 0, 0)
#define CdForward()  CdControl(CdlForward, 0, 0)
#define CdBackward() CdControl(CdlBackward, 0, 0)

/*
 *	Position
 */
#define CdlMAXTOC    100

/*
 *	Callback
 */

typedef void (*CdlCB)(u8, u8 *);

/*
 *	Location
 */
typedef struct
{
	u8 minute; /* minute (BCD) */
	u8 second; /* second (BCD) */
	u8 sector; /* sector (BCD) */
	u8 track;  /* track (void) */
} CdlLOC;

CTR_STATIC_ASSERT(sizeof(CdlLOC) == 4);

/*
 *	ADPCM Filter
 */
typedef struct
{
	u8 file; /* file ID (always 1) */
	u8 chan; /* channel ID */
	u16 pad;
} CdlFILTER;

/*
 *	Attenuator
 */
typedef struct
{
	u8 val0; /* volume for CD(L) -> SPU (L) */
	u8 val1; /* volume for CD(L) -> SPU (R) */
	u8 val2; /* volume for CD(R) -> SPU (L) */
	u8 val3; /* volume for CD(R) -> SPU (R) */
} CdlATV;

/*
 *	Low Level File System for CdSearchFile()
 */
#define CdlMAXFILE  64  /* max number of files in a diRECT16ory */
#define CdlMAXDIR   128 /* max number of total diRECT16ories */
#define CdlMAXLEVEL 8   /* max levels of diRECT16ories */

typedef struct
{
	CdlLOC pos;    /* file location */
	u32 size;      /* file size */
	char name[16]; /* file name (body) */
} CdlFILE;


/*#define MULTI_INTERRUPT */
#ifndef MULTI_INTERRUPT
#define pauseMULI()
#define restartMULI()
#endif

/*
 *	Streaming Structures
 */
typedef struct
{
	u16 id;
	u16 type;
	u16 secCount;
	u16 nSectors;
	u32 frameCount;
	u32 frameSize;

	u16 width;
	u16 height;
	u32 dummy1;
	u32 dummy2;
	CdlLOC loc;
} StHEADER; /* CD-ROM STR structure */

CTR_STATIC_ASSERT(sizeof(StHEADER) == 32);

#define StFREE          0x0000
#define StREWIND        0x0001
#define StCOMPLETE      0x0002
#define StBUSY          0x0003
#define StLOCK          0x0004

#define EDC             0
#define SECTOR_SIZE     (512) /* Sector Size (word) */
#define HEADER_SIZE     (8)   /* Header Size (word) */

#define StSTATUS        0x00
#define StVER           0x00
#define StTYPE          0x01
#define StSECTOR_OFFSET 0x02
#define StSECTOR_SIZE   0x03
#define StFRAME_NO      0x04
#define StFRAME_SIZE    0x06

#define StMOVIE_WIDTH   0x08
#define StMOVIE_HEIGHT  0x09


void StSetRing(u32 *ring_addr, u32 ring_size);
void StClearRing(void);
void StUnSetRing(void);
void StSetStream(u32 mode, u32 start_frame, u32 end_frame, void (*func1)(), void (*func2)());
void StSetEmulate(u32 *addr, u32 mode, u32 start_frame, u32 end_frame, void (*func1)(), void (*func2)());
u32 StFreeRing(u32 *base);
u32 StGetNext(u32 **addr, StHEADER **header);
u32 StGetNextS(u32 **addr, StHEADER **header);
u16 StNextStatus(u32 **addr, StHEADER **header);
void StRingStatus(short *free_sectors, short *over_sectors);
void StSetMask(u32 mask, u32 start, u32 end);
void StCdInterrupt(void);
int StGetBackloc(CdlLOC *loc);
int StSetChannel(u32 channel);

void CdFlush(void);
CdlFILE *CdSearchFile(CdlFILE *fp, char *name);
CdlLOC *CdIntToPos(int i, CdlLOC *p);
char *CdComstr(u8 com);
char *CdIntstr(u8 intr);
int CdControl(u8 com, u8 *param, u8 *result);
int CdControlB(u8 com, u8 *param, u8 *result);
int CdControlF(u8 com, u8 *param);
int CdGetSector(void *madr, int size);
int CdGetSector2(void *madr, int size);
int CdDataSync(int mode);
int CdGetToc(CdlLOC *loc);
int CdPlay(int mode, int *track, int offset);
int CdMix(CdlATV *vol);
int CdPosToInt(CdlLOC *p);
int CdRead(int sectors, u32 *buf, int mode);
int CdRead2(int mode);
int CdReadFile(char *file, u32 *addr, int nbyte);
int CdReadSync(int mode, u8 *result);
int CdReady(int mode, u8 *result);
int CdSetDebug(int level);
int CdSync(int mode, u8 *result);
void(*CdDataCallback(void (*func)()));
CdlCB CdReadCallback(CdlCB func);
CdlCB CdReadyCallback(CdlCB func);
CdlCB CdSyncCallback(CdlCB func);
int CdInit(void);
int CdReset(int mode);
int CdStatus(void);
int CdLastCom(void);
CdlLOC *CdLastPos(void);
int CdMode(void);
int CdDiskReady(int mode);
int CdGetDiskType(void);
struct EXEC *CdReadExec(char *file);
void CdReadBreak(void);


#endif
