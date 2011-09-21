/*********************************************************************
   ATR/XFD File handling library
   (C) Copyright 1997 Ken Siders    

   This file can be used in any freeware or public domain software
   as long as credit is given.


History

00.000  6/16/97    Initial version
00.001  6/24/97    Fixed AtariFileSize, Added EofAtariFile and
                   ExtractAtariFile functions
00.002  6/26/97    Added SortDirectory Function
00.003  7/14/97    Fixed Double density to treat first 3 sectors as  
                   Single Density.
00.004  7/16/97    Added CreateBootAtr
00.005  8/22/97    Added ExtractExeFromBootAtr
00.006  9/04/97    Fix signature check

*********************************************************************

To do:

1 Clean up warnings + make more portable
2 Allow opening write-protected ATRs
3 Allow more than one reference to an ATR file to be opened so more 
   than one atari file can be opened at once. (Keep a count)
4 More specific error returns
5 Implement XFD handling
6 Create documentation
7 Optimize if necessary
8 Implement DCM images (maybe)


*********************************************************************/

#define wide 1


/* compile with /Zp option */


#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include "atr.h"
#include "atdos.h"
#include "kboot.h"

int lastAtariError = 0;

static unsigned char sectorBuffer[256];
static int CompareName( const void *a, const void *b );
static int verbose = 0;

/******************************************************************
SetVerbose - Sets verbose flag, returns last value of flag
******************************************************************/

int SetVerbose( int verb )
   {
   int last;

   last = verbose;
   verbose = verb;
   return(last);
   }



/******************************************************************
 OpenAtr - opens ATR file specified for read/write (if writeable).
           Returns Atr Pointer if successful, 0 if not
******************************************************************/

AtrFilePtr OpenAtr(char *file )
   {
   AtrFilePtr atr;
   int bytes;
   unsigned short signature;

   atr = malloc(sizeof(AtrFile));

   atr->dosType = DOS_ATARI; /* assume atari dos disk */


   atr->atrIn = fopen(file, "rb+");
   if ( !atr->atrIn )
      {
      free(atr);
      return 0;
      }
   bytes = fread(&hdr, 1, 16, atr->atrIn);
   if ( !bytes )
      {
      free(atr);
      return 0;
      }
   
   signature = hdr.idLow | hdr.idHigh << 8;
   atr->imageSize = 16L * (hdr.paraLow | hdr.paraHigh * 256L | hdr.paraHigher * 65536L);
   atr->secSize = hdr.secSizeLow | hdr.secSizeHigh << 8; 
   atr->crc = hdr.crc1 | hdr.crc2 * 256L | hdr.crc3 * 65536L | hdr.crc4 *256L * 65536L ;
   atr->sectorCount = atr->imageSize / atr->secSize;
   atr->flags = hdr.flags;
   atr->writeProtect = atr->flags&1;
   atr->authenticated = (atr->flags >> 1) & 1;
   
   if ( atr->sectorCount > 721 ) 
      atr->dosType = DOS_MYDOS;

   if ( signature == 0x296 )
      return atr;
   else
      {
      free(atr);
      return 0;
      }
   return atr;
   }


/******************************************************************
 CloseAtr - closes ATR file specified in an Atr Pointer from 
            and Atr Open.  Returns 0 if successful
******************************************************************/

int CloseAtr( AtrFilePtr atr )
   {
   if ( atr )
      return(fclose(atr->atrIn));
   else
      return 1;
    }


/******************************************************************
 ReadSector - Reads specified sector from the ATR file specified
              info buffer which must be big enough for the sector
              size of teh file.  Returns number of bytes read or
              0 if error.
******************************************************************/

int ReadSector(AtrFilePtr atr, unsigned short sector, char *buffer)
   {
   unsigned long pos;
   size_t bytes;
                                                             
   if ( !atr )                                               
      {                                                      
      lastAtariError = 12;                                   
      return 0;                                              
      }            
/* calculate offset into file */                                          
   if ( atr->secSize > 128 && sector > 3 )
      pos = (unsigned long)(sector-4) * atr->secSize + 400L;
   else
      pos = (unsigned long)(sector-1) * 128L + 16;

/* position file pointer at that offset */
   if ( fseek(atr->atrIn, pos, SEEK_SET) )
      {
      lastAtariError = 13;
      return 0;
      }

/* read the data */
   bytes = fread(buffer, 1, atr->secSize, atr->atrIn);
   if ( bytes & 127 )
      {
      lastAtariError = 14;
      return 0;
      }
   return bytes;
   }

/******************************************************************
 WriteSector - Writes specified sector from the ATR file specified
               from buffer specified.  Returns number of bytes
               written or 0 if error.  Image must be writeable.
******************************************************************/

int WriteSector(AtrFilePtr atr, unsigned short sector, char *buffer)
   {
   unsigned long pos;
   size_t bytes;

   if ( !atr )
      {
      lastAtariError = 12;
      return 0;
      }

/* calculate offset into file */
   if ( atr->secSize > 128 && sector > 3 )
      pos = (unsigned long)(sector-4) * atr->secSize + 400L;
   else
      pos = (unsigned long)(sector-1) * 128L + 16;

/* set file pointer to that position */
   if ( fseek(atr->atrIn, pos, SEEK_SET) )
      {
      lastAtariError = 13;
      return 0;
      }

/* sector # to high? */
   if ( pos + atr->secSize > atr->imageSize )
      {
      lastAtariError = 15;
      return 0;
      }

/* write the data */
   bytes = fwrite(buffer, 1, atr->secSize, atr->atrIn);
   if ( bytes & 127 )
      {
      lastAtariError = 14;
      return 0;
      }
   return bytes;
}

/******************************************************************
 CreateAtr - Creates an ATR file with parameters specified.  Sector
             size must be a multiple of 128 bytes.  Return 0 for
             success
******************************************************************/

int CreateAtr( char *file, unsigned short sectors, 
               unsigned short sectorSize )
   {
   FILE *fp;
   AtrHeader hdr = {0};
   unsigned long imageSize;
   int bytes;

/* sector size must be a multiple of 128 */
   if ( sectorSize & 127 )
      return 1;
/* determine the file size for the image */

   if ( sectorSize > 128 && sectors > 2)
      imageSize = (unsigned long)(sectorSize-3) * sectors + 384;
   else
      imageSize = (unsigned long)sectorSize * sectors;

/* create the file */
   fp = fopen(file, "wb");
   if ( !fp )
      return 1;

/* set up the ATR header */
   hdr.idHigh = 0x02;
   hdr.idLow = 0x96;
   hdr.paraLow = (imageSize >> 4) & 255;
   hdr.paraHigh = (imageSize >> 12) & 255; 
   hdr.paraHigher = imageSize >> 20;
   hdr.secSizeLow = sectorSize & 255;
   hdr.secSizeHigh = sectorSize >> 8;
   bytes = fwrite(&hdr, 1, 16, fp);
   if ( bytes != 16 )
      return 1;

/* seek to last position needed in file - 1 */
   if ( fseek(fp,(unsigned long)sectors * sectorSize - 1 , SEEK_SET) )
      return 1;
/* write one null byte */
   if ( fputc( 0, fp ) == EOF )
      return 1;
   if ( fclose(fp) )
      return 1;
   return 0;
   }
   
/******************************************************************
 GetAtrInfo - returns info for an open ATR image via pointers.
              non 0 returned is error. 
******************************************************************/

int GetAtrInfo( AtrFilePtr atr, unsigned short *sectorSize,
                unsigned short *sectorCount, byte  *protected)
   {
   if ( !atr )
      return 1;
/* duh */
   *sectorSize = atr->secSize;
   *sectorCount = atr->sectorCount;
   *protected = atr->writeProtect;
   return 0;
   }





/*-----------------------------------------------------------------*/
/*   ATARI 8-bit File IO routines                                  */
/*-----------------------------------------------------------------*/

/******************************************************************
 MakeFileName - Creates a filename.ext string from a zero padded
                raw fileName and extender.  Result is stored in
                string pointed to be result.  There is no return
                value.
******************************************************************/

void MakeFileName( char *result, char *fileName, char *extender )
   {
   int i;
   
   for(i=0; i<8; i++)
      {
      if (fileName[i] == ' ' || !fileName[i] )
         break;
      *(result++) = fileName[i];
      }      
   *(result++) = '.';
   for(i=0; i<3; i++)
      {
      if (extender[i] == ' ' || !extender[i] )
         break;
      *(result++) = extender[i];
      }      
   *(result++) = 0;
   }


/******************************************************************
 PatternMatch - Returns 1 if fileName+extender matches pattern in
                pattern. Wildcards are the standard '?' and '*'
                as supported by all Atari Dos's.  Returns 0 if
                it does not match.
******************************************************************/
int PatternMatch( char *pattern, char *fileName, char *extender)
   {
   int i=0;
   char file[13];

   MakeFileName(file, fileName, extender);

   while (*pattern && file[i] )
      {
      if ( !file[i] && *pattern )
         return 0;
      if ( file[i] && !*pattern )
         return 0;

      if ( *pattern == '*')
         {
         while ( file[i] && file[i] != '.')
            i++;
         while ( *pattern && *pattern != '.')
            pattern++;
         if ( file[i] == '.' && *pattern == '.' )
            {
            pattern++;
            i++;
            continue;
            }
         continue;
         }

      if ( *pattern == '?' && file[i] != '.' )
         {
         i++;
         pattern++;
         continue;
         }

      if ( *pattern != file[i] )
         return 0;

      i++; 
      pattern++;
      }

   if ( !*pattern && !file[i] )
      return 1;
   else
      return 0;
   }


/******************************************************************
 AtariFindFirst - Finds first match for pattern and sets struct
                  with file information.  returns 0 for success,
                  -1 if not found, other for error.  This is 
                  similiar to _dosfindfist in the DOS world.
******************************************************************/

int AtariFindFirst( char *atrName, unsigned attrib,
    char *pattern, AtariFileInfoPtr fileInfo )
   {
   char buffer[256];
   AtrFilePtr atr;
   int i,j;
   AtariDosDirEntryPtr dirEntry;
   unsigned short sectorSize, sectorCount;
   byte protected;

/* open the ATR image */
   atr = OpenAtr(atrName);
   if ( atr == NULL )
      return 2;

/* Get some info about the ATR image and save */
   if ( GetAtrInfo( atr, &sectorSize, &sectorCount, &protected) )
      {
      free(atr);
      return 3;
      }

/* look for the file in the directory, if found initilize the fileInfo
   structure with data from the directory sector */

   for( i = firstDirSector, fileInfo->fileNo = 0; i <= lastDirSector; i++ )
      {
      if (! ReadSector(atr, (unsigned short) i, buffer) )
         return 4;
      for( j=0; j< dirEntriesPerSector; j++, fileInfo->fileNo++ )
         {
         dirEntry = (AtariDosDirEntryPtr)(buffer + dirEntrySize * j );
         fileInfo->locked = (dirEntry->flag & LOCKED_FLAG) ? 1 : 0;
         if (dirEntry->flag & DELETED_FLAG )
            continue; 
         if ( (dirEntry->flag & INUSE_FLAG) && PatternMatch(pattern,
             dirEntry->fileName, dirEntry->extender) )
            {
            fileInfo->flag = dirEntry->flag;
            fileInfo->startSector = dirEntry->startSector;
            fileInfo->sectorCount = dirEntry->sectorCount;
            fileInfo->dirSector = i;
            fileInfo->dirEntry = j;
            fileInfo->attrib = attrib;
            fileInfo->pattern = pattern;
            fileInfo->atrName = atrName;
            MakeFileName( fileInfo->fileName, dirEntry->fileName,
                dirEntry->extender);
            if ( CloseAtr(atr) )
                return 5;
            return( 0 ); /* success */
            }
         }
      }
   if ( CloseAtr(atr) )
      return 6;

   return -1;
   }

/******************************************************************
 AtariFindNext - Returns next matching file after previous 
                 AtariFindFirst or AtariFindNext call.  The fileinfo
                 structure passed should not be altered from the
                 previous call.  Also the ATR file name and pattern
                 from the initial AtariFindFirst call must still be
                 in scope. Similiar to _dosfindnext in DOS world.
******************************************************************/

int AtariFindNext( AtariFileInfoPtr fileInfo )
   {
   char buffer[256];
   AtrFilePtr atr;
   int i,j;
   AtariDosDirEntryPtr dirEntry;
   unsigned short sectorSize, sectorCount;
   byte protected;

   atr = OpenAtr(fileInfo->atrName);
   if ( atr == NULL )
      return 1;

   if ( GetAtrInfo( atr, &sectorSize, &sectorCount, &protected) )
      {
      free(atr);
      return 2;
      }
   i = fileInfo->dirSector;
   j = fileInfo->dirEntry;

   j++;
   if ( j >= dirEntriesPerSector )
      {
      j=0;
      i++;
      }

   for( ; i <= lastDirSector; i++ , j = 0)
      {
      if (! ReadSector(atr, (unsigned short) i, buffer) )
         return 3;
      for( ; j< dirEntriesPerSector; j++, fileInfo->fileNo++ )
         {
         dirEntry = (AtariDosDirEntryPtr)(buffer + dirEntrySize * j );
         fileInfo->locked = (dirEntry->flag & LOCKED_FLAG) ? 1 : 0;
         if (dirEntry->flag & DELETED_FLAG )
            continue; 
         if ( (dirEntry->flag & INUSE_FLAG) && PatternMatch(fileInfo->pattern,
             dirEntry->fileName, dirEntry->extender) )
            {
            fileInfo->flag = dirEntry->flag;
            fileInfo->startSector = dirEntry->startSector;
            fileInfo->sectorCount = dirEntry->sectorCount;
            fileInfo->dirSector = i;
            fileInfo->dirEntry = j;
            MakeFileName( fileInfo->fileName, dirEntry->fileName,
                dirEntry->extender);
            if ( CloseAtr(atr) )
               return 4;
            return( 0 );
            }
         }
      }
   if ( CloseAtr(atr) )
      return 5;

   return -1;
   }


/******************************************************************
 OpenAtariFile - Opens file in an ATR image in the mode specified
                 (ATARI_OPEN_READ, ATARI_OPEN_WRITE, or
                 ATARI_OPEN_DIR.  Returns pointer to atari file
                 structure or 0 on error. 
******************************************************************/

AtariFilePtr OpenAtariFile( char *atrName, char *fileName, byte mode)
   {
   AtariFilePtr atFile;
   byte protected;
   unsigned short sectorSize;
   unsigned short sectorCount;
   AtariFileInfo fileInfo;

/* bad open mode? */
   if ( mode != ATARI_OPEN_READ && mode != ATARI_OPEN_WRITE &&
        mode != ATARI_OPEN_DIR )
      {
      lastAtariError = 2;
      return NULL;
      }

   atFile = malloc(sizeof(AtariFile));
/* open the atr image */
   atFile->atr = OpenAtr(atrName);
   if ( atFile->atr == NULL )
      {
      free(atFile);
      lastAtariError = 1;
      return NULL;
      }
/* get some info on the ATR file and store */
   if ( GetAtrInfo( atFile->atr, &sectorSize, &sectorCount, &protected) )
      {
      CloseAtr(atFile->atr);
      free(atFile);
      lastAtariError = 3;
      return NULL;
      }

/* set file parameters */
   atFile->sectorSize = sectorSize;
   atFile->openFlag = mode;
   atFile->eofFlag = 0;

/* is ATR write protected? (APE extension?) */
   if ( protected && (mode & ATARI_OPEN_WRITE) )
      {
      CloseAtr(atFile->atr);
      free(atFile);
      lastAtariError = 4;
      return NULL;
      }

/* read directory, find start sector and number of sectors and set
   in atFile.  Initialize current sector to start sector also */

   if ( AtariFindFirst(atrName, 0, fileName, &fileInfo) )
      {
      lastAtariError = 5;
      return NULL;
      }

/* is the file the ATR is lcoated in write protected? */   
   if ( fileInfo.locked && (mode & ATARI_OPEN_WRITE) )
      {
      lastAtariError = 6;
      return NULL;
      }

/* set some file info data in the structure */
   atFile->startSector = atFile->currentSector = fileInfo.startSector;
   atFile->fileNo = fileInfo.fileNo;
   atFile->numberOfSectors = fileInfo.sectorCount;
   atFile->openFlag = mode;
   atFile->currentOffset = 0;
   if (sectorSize == 128)
      atFile->sectorLinkOffset = 125;
   else if (sectorSize == 256 )
      atFile->sectorLinkOffset = 253;
   else
      {
      lastAtariError = 7;
      return NULL;
      }
   return atFile;
   }


/******************************************************************
 ReadAtariFile - reads bytes bytes from the open atari file specified
   in atFile and stores them in buffer.  buffer must be big enough.
   Returns bytes actually read. 
******************************************************************/

long ReadAtariFile( AtariFilePtr atFile, char *buffer, long bytes )
{
   int lastSector = 0;
   long bytesRead = 0;

   if ( !bytes || atFile->eofFlag)
      return 0;
   if ( !(atFile->openFlag & ATARI_OPEN_READ) )
      return 0;
   if ( !atFile->currentOffset )
      {
      /* read sector */
      if (ReadSector(atFile->atr, atFile->currentSector, atFile->sectorBuffer) != atFile->sectorSize )
         {
         if ( !lastAtariError )
            lastAtariError = 19;
         return 0;
         }
      if ( atFile->sectorSize == 128 )
         atFile->bytesData = atFile->sectorBuffer[atFile->sectorLinkOffset+2]
         & 127;
      else
         atFile->bytesData = atFile->sectorBuffer[atFile->sectorLinkOffset+2]; 
      }
   while( bytes )
      {
      while ( atFile->currentOffset < atFile->bytesData && bytes)
         {
         *(buffer++) = atFile->sectorBuffer[atFile->currentOffset++];
         bytes--;
         bytesRead++;
         }

      if ( bytes )
         {
         /* read next sector */
         atFile->currentOffset = 0;
         if (atFile->atr->dosType == DOS_MYDOS )
            atFile->currentSector =
                (atFile->sectorBuffer[atFile->sectorLinkOffset] << 8) |
                atFile->sectorBuffer[atFile->sectorLinkOffset+1];
         else /* assume atari dos */
            atFile->currentSector =
                ((atFile->sectorBuffer[atFile->sectorLinkOffset] & 3) << 8) |
                (atFile->sectorBuffer[atFile->sectorLinkOffset+1]);

         if (!atFile->currentSector )
            {
            atFile->eofFlag = 1;
            return bytesRead;
            }

         ReadSector(atFile->atr, atFile->currentSector, atFile->sectorBuffer);
         if ( atFile->sectorSize == 128 )
            atFile->bytesData = atFile->sectorBuffer[atFile->sectorLinkOffset+2] & 127;
         else
            atFile->bytesData = atFile->sectorBuffer[atFile->sectorLinkOffset+2]; 
         }

      }
   return bytesRead;
   }


/******************************************************************
 CloseAtariFile - Closes Atari File 
******************************************************************/

int CloseAtariFile( AtariFilePtr atFile )
   {
   int stat;
   /* simple enough */
   stat = CloseAtr( atFile->atr );
   free( atFile );
   return stat;
   }


/******************************************************************
 EofAtariFile - Returns 1 if at EOF of atari file, 0 if not 
******************************************************************/

int EofAtariFile( AtariFilePtr atFile )
   {
   /* simple enough */
   return atFile->eofFlag;
   }

/******************************************************************
 AtariDirectory - Displays atari directory of disk image to screen
                  return 0 for success.  atrName is the ATR file
                  name, pattern is mask to use.  use "*.*" for all
                  files.  Wide if non zero displays actual file
                  length instead of just a sector count.
******************************************************************/

int AtariDirectory( char *atrName, char *pattern)
   {
   char buffer[256];
   char fileName[14];
   byte protected;
   unsigned short sectorSize;
   unsigned short sectorCount;
   AtrFilePtr atr;
   int i,j,cnt = 0;
   char locked;
   unsigned long fileSize;
   AtariDosDirEntryPtr dirEntry;

   atr = OpenAtr(atrName);
   if ( atr == NULL )
      return 1;

   if ( GetAtrInfo( atr, &sectorSize, &sectorCount, &protected) )
      {
      free(atr);
      return 1;
      }
   
   printf("sector size = %hu    sector count = %hu\n\n", sectorSize,
       sectorCount);


   printf("\nDirectory of '%s':\n\n", atrName);

   if ( !wide )
      {
      printf("no f filename ext  secs  startSec\n");
      printf("-- - -------- ---  ----  --------\n");
      }
   else
      {
      printf("no f filename ext  secs  length  startSec\n");
      printf("-- - -------- ---  ----  ------  --------\n");
      }
   for( i = firstDirSector; i <= lastDirSector; i++ )
      {
      if (! ReadSector(atr, (unsigned short) i, buffer) )
         return 1;
      for( j=0; j< dirEntriesPerSector; j++ )
         {
         dirEntry = (AtariDosDirEntryPtr)(buffer + dirEntrySize * j );
         locked = (dirEntry->flag & LOCKED_FLAG) ? '*' : ' ';
         if (dirEntry->flag & DELETED_FLAG )
            locked = 'D'; 
         if ( (dirEntry->flag & INUSE_FLAG) && PatternMatch(pattern,
             dirEntry->fileName, dirEntry->extender) )
            {
            if ( wide )
               {
               CloseAtr(atr);
               MakeFileName(fileName, dirEntry->fileName, dirEntry->extender);
               fileSize = AtariFileSize(atrName, fileName);
               atr = OpenAtr(atrName);
               if ( !atr )
                  return 1;
               printf("%2u %c %-8.8s %-3.3s  %4.3hu  %-6lu  %hu\n", cnt, locked,
                  dirEntry->fileName, dirEntry->extender,
                  dirEntry->sectorCount, fileSize, dirEntry->startSector);
               }
            else
               printf("%2u %c %-8.8s %-3.3s  %4.3hu  %hu\n", cnt, locked,
                  dirEntry->fileName, dirEntry->extender,
                  dirEntry->sectorCount, dirEntry->startSector);
            }
         cnt++;
         }
      }

   if ( CloseAtr(atr) )
      return 1;

   return 0;
   }


/******************************************************************
 AtariFileSize - Returns size of atari file or -1 on error
******************************************************************/

long AtariFileSize( char *atrFile, char *fileName )
   {
   long count = 0;
   long bytes;
   static char buffer[16];
   AtariFilePtr input;

   /* open the atari file on the ATR image */
   input = OpenAtariFile(atrFile, fileName, ATARI_OPEN_READ);
   if ( input == NULL )
      return -1;  
   /* count how many bytes we can actually read */
   while( (bytes=ReadAtariFile(input, buffer, sizeof(buffer))) > 0 )
      count+= bytes; 
   CloseAtariFile(input);
   return(count);
   }


/******************************************************************
 ExtractAtariFile - returns no. files extracted, -no for error.
                    file is stored with same name in dosPath
                    directory.  (don't add the trailing '\').
                    Wildcards are allowed for atari file. Use NULL
                    for dosPath to extract to current directory
******************************************************************/

int ExtractAtariFile( char *atrFile, char *fileName, char *dosPath )
{
   int count = 0;
   long bytes, bytesOut;
   static char buffer[16];
   AtariFilePtr input;
   char outName[128];
   FILE *output;
   AtariFileInfo info;
   

   if ( !AtariFindFirst(atrFile, 0, fileName, &info) )
      {
      do {
         if ( dosPath != NULL)
            {
            strcpy(outName, dosPath);
            strcat(outName,"\\");
            }
         else
            outName[0] = 0;
         strcat(outName, info.fileName);
         
         output = fopen(outName, "wb");
         if (output == NULL )
            {
            lastAtariError = 30;
            return -count-1;
            }
         input = OpenAtariFile(atrFile, info.fileName, ATARI_OPEN_READ);
         if ( input == NULL )
            {
            return -count-1;  
            }
         if ( verbose )
            printf("Extracting '%s'...", info.fileName);
         while( (bytes=ReadAtariFile(input, buffer, sizeof(buffer))) > 0 )
            {
            bytesOut = fwrite(buffer, 1, (int)bytes, output);
            if ( bytes != bytesOut )
               {
               fclose( output );
               CloseAtariFile(input);
               lastAtariError = 31;
               if (verbose )
                  printf("\n");
               return -count-1;
               }
            }
         fclose( output );
         CloseAtariFile(input);
         if ( verbose )
            printf(" done\n");
         count ++;
         } while ( !AtariFindNext(&info) );
      }
   else
      {
      return 0;
      }
   return(count);
}


/******************************************************************
 UpdateAtariFileNo - For atari dos, will fix the file no in each
                     sector within the file.  For use after a
                     directory is sorted.  returns 0 for success
******************************************************************/

int FixAtariFileNo( char *atrName, char *fileName, int fileNo )
{
   int cnt=0;
   AtariFilePtr atFile;

   atFile = OpenAtariFile( atrName, fileName, ATARI_OPEN_READ);
   if ( atFile == NULL )
      return 1;

   if ( atFile->atr->dosType != DOS_ATARI )
      {
      CloseAtariFile(atFile);
      return 0;
      }


   while (atFile->currentSector) 
      {   
      if (ReadSector(atFile->atr, atFile->currentSector, sectorBuffer) != atFile->sectorSize )
         {
         if ( !lastAtariError )
            lastAtariError = 19;
         CloseAtariFile(atFile);
         return 1;
         }

      /* set the file no in the file */
      sectorBuffer[atFile->sectorLinkOffset] &= 3;
      sectorBuffer[atFile->sectorLinkOffset] |= (atFile->fileNo<<2);

      /* write the sector */
      if (WriteSector(atFile->atr, atFile->currentSector, sectorBuffer) != atFile->sectorSize )
         {
         if ( !lastAtariError )
            lastAtariError = 19;
         CloseAtariFile(atFile);
         return 1;
         }

      /* get next sector in link */
      atFile->currentSector =
             ((sectorBuffer[atFile->sectorLinkOffset] & 3) << 8) |
             sectorBuffer[atFile->sectorLinkOffset+1];
      }
   CloseAtariFile(atFile);
   return 0;
   }









/******************************************************************
 SortAtariDir - 
******************************************************************/

int SortAtariDir( char *atrName )
   {
   AtrFilePtr atr;
   char *pos;
   AtariFileInfoPtr files[64];
   AtariFileInfo info;
   AtariDosDirEntryPtr entry;
   int offset;
   unsigned short sector;   

   int i, cnt = 0;

   /* read file info for all files, allocate memory and store in array */

   if ( !AtariFindFirst(atrName, 0, "*.*", &info) )
      {
      do {
         files[cnt] = malloc( sizeof(info) );
         memcpy( files[cnt], &info, sizeof(info) );
         cnt ++;
         } while ( !AtariFindNext(&info) );
      }
   else
      {
      return 1;
      }

   /* sort the files by name */

   qsort( (void *)files, (size_t) cnt, (size_t) sizeof(AtariFileInfoPtr), CompareName );

   /* clear out the directory */
   if ( ClearAtariDirectory(atrName) )
      return 1;

   /* write the entries in sorted order to the directory sectors */

   offset = 0;
   sector = firstDirSector;

   atr = OpenAtr( atrName );

   for( i=0; i<cnt; i++ )
      {
      entry = (AtariDosDirEntryPtr) (sectorBuffer + offset );
      entry->startSector = files[i]->startSector;
      entry->sectorCount = files[i]->sectorCount;
      entry->flag = files[i]->flag;
      pos = strchr( files[i]->fileName, '.' );
      if ( pos != NULL)
         {
         strncpy( entry->fileName, files[i]->fileName, 
                  min(pos-files[i]->fileName,8) );
         strncpy( entry->extender, pos+1, 3 );     
         }
      else
         {
         strncpy( entry->fileName, files[i]->fileName, 8);
         }
      offset += 16;
      if (offset >= 128)
         {
         if ( WriteSector( atr, sector, sectorBuffer) != atr->secSize )
            {
            CloseAtr( atr );
            return 1;
            }
         sector ++;
         offset = 0;
         memset(sectorBuffer, 0, sizeof(sectorBuffer));
         }      
      }
   if ( offset > 0 )
      if ( WriteSector( atr, sector, sectorBuffer) != atr->secSize )
         {
         CloseAtr( atr );
         return 1;
         }

   CloseAtr( atr );


   /* This should have no effect on Mydos extended format disks */
   for( i=0; i<cnt; i++ )
      {
      if ( FixAtariFileNo( atrName, files[i]->fileName, i) )
         return 1;
      free( files[i] );
      }
   return 0;
   }

/* function for above used as arg to qsort */
static int CompareName( const void *a, const void *b )
   {
   AtariFileInfoPtr aa = *(AtariFileInfoPtr *)a,
                    bb = *(AtariFileInfoPtr *)b;

   return strcmp( aa->fileName, bb->fileName );
   }

/*******************************************************************
ClearAtariDirectory - internal routine
*******************************************************************/

static ClearAtariDirectory( char *file )
   {
   AtrFilePtr atr;
   int i; 
  
   memset(sectorBuffer, 0, sizeof(sectorBuffer) );
   atr = OpenAtr( file );
   
   for( i = firstDirSector; i <= lastDirSector; i++ )
      if ( WriteSector(atr, i, sectorBuffer) != atr->secSize )
         return 1;
   return 0;
   CloseAtr(atr);
   }


/********************************************************************
 CreateBootAtr - creates a minimally sized bootable ATR image from
                 an atari executable.  The executable must not need
                 DOS to run.
********************************************************************/

int CreateBootAtr( char *atrName, char *fileName)
   {
   unsigned long fileSize;
   unsigned long sectorCnt;
   AtrHeader hdr;
   unsigned long paras;
   FILE * atrFile, *inFile;
   size_t padding, bytes, bytes2;
   struct find_t fileInfo;
   int stat;

/* get file's size */

   stat = _dos_findfirst(fileName, _A_NORMAL, &fileInfo);
   if ( stat )
      return 11;
   fileSize = (unsigned long) fileInfo.size;
   if ( !fileSize )
      return 12;

/* determine number of sectors required  */

   sectorCnt = (unsigned short) ((fileSize + 127L) / 128L + 3L);
   paras = sectorCnt * 16;

/* create ATR header */
   memset(&hdr, 0, sizeof(hdr));
   hdr.idLow      = (byte) 0x96;
   hdr.idHigh     = (byte) 0x2;
   hdr.paraLow    = (byte) (paras & 0xFF);
   hdr.paraHigh   = (byte) ((paras >> 8) & 0xFF);
   hdr.paraHigher = (byte) ((paras >> 16) & 0xFF);
   hdr.secSizeLow = (byte) 128;

/* open output file */
   atrFile = fopen(atrName, "wb");
   if ( atrFile == NULL )
      return 1;

/* Write the ATR Header */
   bytes = fwrite(&hdr, 1, sizeof(hdr), atrFile);
   if ( bytes != sizeof(hdr) )
      {
      close(atrFile);
      return 2;
      }

/* plug the file size into the boot sectors at offset 9 (4 bytes)*/
   bootData[9]  = (byte)(fileSize & 255);
   bootData[10] = (byte)((fileSize >> 8) & 255);
   bootData[11] = (byte)((fileSize >> 16) & 255);
   bootData[12] = 0;

/* write the three boot sectors */
   bytes = fwrite(bootData, 1, 384, atrFile);
   if ( bytes != 384 )
      {
      fclose(atrFile);
      return 6;
      }

/* open the input file and copy/append the file's data to output file */

   inFile = fopen(fileName, "rb");
   if ( inFile == NULL )
      {
      close(atrFile);
      return 13;
      }

   bytes = 384;
   while (bytes == 384)
      {
      bytes = fread(bootData, 1, 384, inFile);
      if ( !bytes )
         break;
      bytes2 = fwrite(bootData, 1, bytes, atrFile);
      if ( bytes != bytes2 )
         {
         fclose(inFile);
         fclose(atrFile);
         return 19;
         }
      }
   if ( !feof(inFile) )
      {
      fclose(inFile);
      fclose(atrFile);
      return 19;
      }

  fclose(inFile); 


/* pad to even sector size (data has no meaning) */
   padding = (size_t) ((sectorCnt-3) * 128 - fileSize ); 
   if ( padding )
      {
      bytes = fwrite(bootData, 1, padding, atrFile);
      if ( bytes != padding )
         {
         close(atrFile);
         return 7;
         }
      }  

/* close output */
   fclose(atrFile);

   return 0;
   }

/********************************************************************
ExtractExeFromBootAtr - undoes a CreateBootAtr by extracting the
                        original executable
returns 0 for error, or file length in bytes of file extracted
!!This function needs to have code added to distinguish error types.
********************************************************************/

long ExtractExeFromBootAtr( char *atrName, char *fileName)
   {
   FILE *atrFile, *exeFile;
   unsigned char *buffer;
   AtrHeader hdr = {0};
   long fileSize;
   size_t bytes,bytes2,readCnt;
   unsigned long  fSize, size;

/* get some memory */
   buffer = malloc(384);
   if ( !buffer)
      return 0;

/* open the atr file */
   atrFile = fopen(atrName, "rb");
   if ( !atrFile )
      {
      free(buffer);
      return 0;
      }

/* read Atr header */

   if ( fread(&hdr, 1, 16, atrFile) != 16 )
      {
      free(buffer);
      fclose(atrFile);
      return 0;
      }

/* verify it is an ATR file */

   if ( hdr.idHigh != 0x02 || hdr.idLow != 0x96 )
      {
      free(buffer);
      fclose(atrFile);
      return 0;
      }

/* read first 3 (boot) sectors */

   if ( fread(buffer, 1, 384, atrFile) != 384 )
      {
      free(buffer);
      fclose(atrFile);
      return 0;
      }

/* set the file size in bootData from the file so we can compare*/
   bootData[9]   = buffer[9];
   bootData[10]  = buffer[10];
   bootData[11]  = buffer[11];
   bootData[12]  = buffer[12];

/* check if ATR was created by MakeBootAtr */
   if ( memcmp(buffer, bootData, 384) )
      {
      free(buffer);
      fclose(atrFile);
      return 0;
      }
/* Get size of file to extract */
   fSize = size = ((unsigned long)buffer[9]|(((unsigned long)buffer[10])<<8)|(((unsigned long)buffer[11])<<16));

/* Open output file */
   exeFile = fopen(fileName, "wb");
   if ( !exeFile )
      {
      close(atrFile);
      free(buffer);
      return 0;
      }
/* copy 'size' bytes from the Atr file to the exe file */   
   bytes = 384;
   while (bytes == 384 && fSize)
      {
      readCnt = min(384, fSize);
      bytes = fread(buffer, 1, readCnt, atrFile);
      if ( !bytes )
         break;
      bytes2 = fwrite(buffer, 1, bytes, exeFile);
      if ( bytes != readCnt )
         {
         fclose(exeFile);
         fclose(atrFile);
         free(buffer);
         return 0;
         }
      fSize -= bytes;
      }

/* clean up and get out of here */
   fclose(exeFile);
   fclose(atrFile);
   free(buffer);

   return size;
   }


