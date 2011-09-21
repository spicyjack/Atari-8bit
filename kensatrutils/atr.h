/* Copyright 1997 Ken Siders */

#ifndef ATR_H_INCLUDED
#define ATR_H_INCLUDED

/* constants */
#define ATR_HEADER_SIZE 16


/* typedefs */
typedef unsigned char byte;


/* structure definitions */

struct S_HDR
{
byte idLow, idHigh;
byte paraLow, paraHigh;
byte secSizeLow, secSizeHigh;
byte paraHigher;
byte crc1, crc2, crc3, crc4;
byte unused1, unused2, unused3, unused4;
byte flags;
} hdr;

typedef struct S_HDR AtrHeader;

struct S_AtrFile {
   FILE *atrIn;
   unsigned long  imageSize;
   unsigned short secSize;
   unsigned long crc;
   unsigned long sectorCount;
   byte flags;
   byte writeProtect;
   byte authenticated;
   unsigned short currentSector;
   unsigned char dosType;
};

typedef struct S_AtrFile AtrFile;
typedef AtrFile *AtrFilePtr; 


/* function prototypes */
AtrFilePtr OpenAtr(char *file );

int CloseAtr( AtrFilePtr atr );
int ReadSector(AtrFilePtr atr, unsigned short sector, char *buffer);
int WriteSector(AtrFilePtr atr, unsigned short sector, char *buffer);
int CreateAtr( char *file, unsigned short sectors, 
               unsigned short sectorSize );
int GetAtrInfo( AtrFilePtr atr, unsigned short *sectorSize,
                unsigned short *sectorCount, byte  *protected);
int CreateBootAtr( char *atrName, char *fileName);
long ExtractExeFromBootAtr(char *, char *);

#endif


