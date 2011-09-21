/* Copyright 1997 Ken Siders */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atr.h"
#include "atdos.h"

/* SortAtr */


int main( int argc, char **argv)
   {
   AtrFilePtr atr;
   int stat;

   printf("SortATR v0.9  (C)1997 Ken Siders\n");
   printf("This program may be freely distributed\n\n");

   if ( argc != 2 )
      {
      printf("usage: SortAtr atrname\n");
      exit(2);
      }
   stat = SortAtariDir( argv[1] );
   if ( stat )
      {
      printf("Error sorting directory\n");
      exit(3);
      }
   else
      {
      printf("no errors\n");
      exit(0);
      }


}


