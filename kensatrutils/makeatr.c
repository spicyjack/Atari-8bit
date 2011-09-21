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
   long size;
   int count;
   int stat = 0;

   AtariFileInfo info;
   AtariFilePtr input;

   printf("MakeAtr Version 0.9  (c)1997 Ken Siders\n");
   printf("This program may be freely distributed\n\n");


   if (argc != 3)
      {
      printf("usage: makeatr atrname.atr file\n\n");
      }
   else
      {
      stat = CreateBootAtr( argv[1], argv[2] );
      if ( stat )
         printf("Error #%d encountered in conversion\n\n", stat);
      else
         printf("No errors, %s created successfully\n\n", argv[1]);
      }


}


