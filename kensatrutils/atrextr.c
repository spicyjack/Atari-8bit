/* Copyright 1997 Ken Siders */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atr.h"
#include "atdos.h"

int main( int argc, char **argv)
   {
   AtrFilePtr atr;
   AtariFileInfo info;

   static char buffer[1024];
   int bytes;
   long size;
   int count;
   int stat = 0;


   printf("AtrExtr Version 0.9  (c)1997 Ken Siders\n");
   printf("This program may be freely distributed\n\n");


   if (argc != 3)
      {
      printf("usage: atrextr atrname.atr file\nWildcards may be used for file\n\n");
      }
   else
      {
      atr = OpenAtr(argv[1]);
      if (atr == 0)
         {
         printf("Error reading ATR file: %s\n", argv[1]);
         CloseAtr(atr);
         exit(2);
         }
      CloseAtr(atr);

      while ( !stat )
         {
         SetVerbose(1);
         stat = ExtractAtariFile( argv[1], argv[2], NULL );
         SetVerbose(0);
         }
      if ( stat < 0 )
         {
         printf("\nError encountered when extracting file(s)\n");
         printf("%d file(s) were extracted successfully\n");

         }      

      printf("\n%d file(s) extracted\n", stat);

      }


}


