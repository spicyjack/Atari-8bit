/* Copyright 1997 Ken Siders */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atr.h"
#include "atdos.h"


/********************************************************************
 Testing 
********************************************************************/


int main( int argc, char **argv)
   {
   static char buffer[1024];
   int bytes;
   long size = 0;
   int count;

   AtariFileInfo info;
   AtariFilePtr input;

   printf("MakeAtr Version 0.9  (c)1997 Ken Siders\n");
   printf("This program may be freely distributed\n\n");


   if (argc != 3)
      {
      printf("usage: unmakatr atrname.atr file\n\n");
      }
   else
      {
      size = ExtractExeFromBootAtr( argv[1], argv[2] );
      if ( size <= 0 )
         printf("Error encountered in extracting\n\n", -size);
      else
         printf("No errors, %s extracted successfully (%ld bytes)\n\n", argv[2], size);
      }


}


