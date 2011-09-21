/* Copyright 1997 Ken Siders */

/* dos compatability types */
#define DOS_ATARI 1
#define DOS_MYDOS 2
#define DOS_SPARTADOS 3
#define DOS_LJK 4

#define MAX_ATARI_FILES 64

/* disk format types */
#define FORMAT_SD 1
#define FORMAT_ED 2
#define FORMAT_DD 3
#define FORMAT_SD_CUSTOM 4
#define FORMAT_DD_CUSTOM 5
#define FORMAT_DEFAULT 6

/* open modes */
#define ATARI_OPEN_READ 4
#define ATARI_OPEN_WRITE 8
#define ATARI_OPEN_DIR 6

/* these will be in variables later */
#define firstDirSector 361
#define lastDirSector 368
#define vtocSector 360
#define vtocSector2 0
#define dirEntrySize 16
#define dirEntriesPerSector 8

#define DELETED_FLAG    0x80
#define INUSE_FLAG      0x40
#define LOCKED_FLAG     0x20
#define OPENOUTPUT_FLAG 0x01

struct S_AtariDosDirEntry
   {
   char flag;
   unsigned short sectorCount;
   unsigned short startSector;
   char fileName[8];
   char extender[3];
   };

typedef struct S_AtariDosDirEntry AtariDosDirEntry;
typedef AtariDosDirEntry *AtariDosDirEntryPtr;

struct S_AtariDosLink
   {
   int fileNo:6;
   int nextSector:10;
   int shortSector:1;
   int bytesInSector:7;

   };

typedef struct S_AtariDosLink AtariDosLink;
typedef AtariDosLink *AtariDosLinkPtr;

struct S_AtariVtoc
   {
   byte reserved[10];
   byte vtoc[118];
   };




struct S_AtariFile
   {
   AtrFilePtr atr;
   unsigned short startSector;
   unsigned short currentSector;
   unsigned short sectorSize;
   unsigned short numberOfSectors;  /* no. of sectors in file */
   unsigned long fileSize ; /* not used yet */
   byte openFlag;
   byte eofFlag;
   short currentOffset;
   short bytesData;
   short sectorLinkOffset;
   short fileNo;
   unsigned char sectorBuffer[256];

   };

typedef struct S_AtariFile AtariFile;
typedef AtariFile * AtariFilePtr;




struct S_AtariFileInfo
   {
   unsigned long fileSize; /* not implemented */
   unsigned short startSector;
   unsigned short sectorCount;
   char fileName[13];
   int locked;
   int attrib;        /* internal */
   int dirSector;     /* internal */
   int dirEntry;      /* internal */
   char *pattern;     /* internal */
   char *atrName;     /* internal */
   int dosType;
   short fileNo;
   int flag;    /* from dos */
   };

typedef struct S_AtariFileInfo AtariFileInfo;
typedef AtariFileInfo *AtariFileInfoPtr;



/* function prototypes */

void MakeFileName( char *result, char *fileName, char *extender );
static int PatternMatch( char *pattern, char *fileName, char *extender);
int AtariFindFirst( char *atrName, unsigned attrib,
    char *pattern, AtariFileInfoPtr fileInfo );
int AtariFindNext( AtariFileInfoPtr fileInfo );
AtariFilePtr OpenAtariFile( char *atrName, char *fileName, byte mode);
int AtariDirectory( char *atrName, char *pattern);
long ReadAtariFile( AtariFilePtr atFile, char *buffer, long bytes );
int CloseAtariFile( AtariFilePtr atFile );
int EofAtariFile( AtariFilePtr atFile );
long AtariFileSize( char *atrFile, char *fileName );
int FixAtariFileNo( char *atrName, char *fileName, int fileNo );

