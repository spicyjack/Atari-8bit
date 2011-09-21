/* Copyright 1997 Ken Siders */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atr.h"
#include "atdos.h"

void help(void)
   {
   printf("usage: atrdir atrname.atr [filespec]\n");
   printf("\n");
   printf("\n\n");
   }


int main( int argc, char **argv)
   {
   AtrFilePtr atr;
   AtariFileInfo info;

   int count = 0;
   int stat = 0;
   int minCnt;
   char option;
   static char mask[80];


   printf("AtrDir Version 0.9  (c)1997 Ken Siders\n");
   printf("This program may be freely distributed\n\n");

   if (argc < 2)
      {
      help();
      exit(2);
      }
   
   if (argv[1][0] == '-')
      {
      option = argv[1][1];
      minCnt = 3;
      }
   else
      {
      option = 0;
      minCnt = 2;
      }

   if ( argc != minCnt && argc != minCnt + 1)
      {
      help();
      exit(4);
      }

   if (option && option != 'W' && option != 'w' )
      {
      help();
      exit(5);
      }

   atr = OpenAtr(argv[minCnt-1]);
   if (atr == 0)
      {
      printf("Error reading ATR file: %s\n", argv[1]);
      exit(2);
      }
   else
      {
      CloseAtr(atr);
      }

   if (argc == minCnt)
      strcpy(mask, "*.*");
   else
      strcpy(mask, argv[minCnt]);
   printf("Directory of %s:%s:\n", argv[minCnt-1], mask);
   AtariDirectory( argv[minCnt-1], mask);
      
   





   printf("\n%d file(s)\n", count);

   }





