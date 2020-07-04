#!/usr/bin/perl -w
#
# Adds Atari800 emulator cartridge header if missing,
# otherwise it merely updates the header contents.
#
# Written by Nathan Hartwell
#
# 1.0 - 12/3/2001  - Initial release
#
# 1.1 - 12/3/2001  - Improved error handling if file does not exist.
#                    Added '@' to cartridge type selection.
#                    Added auto big-/little-endian check for checksum byte swap.
#
# 1.2 - 12/13/2001 - Rewritten based on the rewrite from Jindroush.  I don't like
#                    inability of having to wait until the user presses [ENTER]
#                    before processing the input given, but until I find a fully
#                    portable single character input solution, this will have to
#                    do.
#
# 1.3 - 12/14/2001 - I couldn't stand having to hit the [ENTER] key everytime.
#                    So, I dropped in some code to use the Term::ReadKey module.
#                    If your perl does't have this module, I'm sorry.  It works
#                    with the perl distribution for cygwin (perl 5.6.1), but the
#                    perl on my Linux router doesn't have that module.  The perl
#                    version on my home Linux system is 5.005_03 and will be
#                    upgraded shortly.  :)
#
# 1.4 - 12/17/2001 - Code cleaned up to remove warnings while "use strict;" is in
#                    place.
#
# 1.5 - 12/18/2001 - More code tweak suggestions from Jindroush added.
#
# 1.6 - 6/19/2012  - A much needed update. Added new cartridge types and changed
#                    input method.  Term::ReadKey is no longer used.
#

use strict;
use integer;
use Fcntl;
$| = 1;

my @TypeNames =
(
  "No type",
  "Standard 8K (800/XL/XE)",
  "Standard 16K (800/XL/XE)",
  "OSS '034M' 16K (800/XL/XE)",
  "Standard 32K (5200)",
  "DB 32K (800/XL/XE)",
  "2 chip 32K (5200)",
  "Bounty Bob Strikes Back (5200)",
  "8x8 D50x 64K (800/XL/XE)",
  "Express 64K (800/XL/XE)",
  "Diamond 64K (800/XL/XE)",
  "SpartaDOS X 64K (800/XL/XE)",
  "XEGS 32K (800/XL/XE)",
  "XEGS 64K (800/XL/XE)",
  "XEGS 128K (800/XL/XE)",
  "OSS 'M091' 16K (800/XL/XE)",
  "1 chip 16K (5200)",
  "ATRAX 128K (800/XL/XE)",
  "Bounty Bob Strikes Back (800/XL/XE)",
  "Standard 8K (5200)",
  "Standard 4K (5200)",
  "Right Slot 8K (800)",
  "8x8 D50x 32K?",
  "XEGS 256K",
  "XEGS 512K",
  "XEGS 1024K",
  "Mega 16K",
  "Mega 32K",
  "Mega 64K",
  "Mega 128K",
  "Mega 256K",
  "Mega 512K",
  "Mega 1024K",
  "SWXEGS 32K",
  "SWXEGS 64K",
  "SWXEGS 128K",
  "SWXEGS 256K",
  "SWXEGS 512K",
  "SWXEGS 1024K",
  "Phoenix 8K",
  "Blizzard 16K",
  "ATMAX 128K",
  "ATMAX 1024K",
  "SDX 128K",
);

my %Types =
(
  4096 =>   [ 20 ],
  8192 =>   [ 1, 19, 21, 39 ],
  16384 =>  [ 2, 3, 15, 16, 26, 40 ],
  32768 =>  [ 4, 5, 6, 12, 22, 27, 33 ],
  40960 =>  [ 7, 18 ],
  65536 =>  [ 8, 9, 10, 11, 13. 28, 34 ],
  131072 => [ 14, 17, 29, 35, 41, 43 ],
  262144 => [ 23, 30, 36 ],
  524288 => [ 24, 31, 37 ],
  1048576 => [ 25, 32, 38, 42 ]
);

print "\nAtari800 Cartridge Header Creator/Updater v1.6\n";
print "With major code improvements inspired by Jindroush\n";
print "By Nathan Hartwell - June 19th, 2012\n";

while (@ARGV)
{
  my $file = shift @ARGV;

  if (! -f $file)
  {
    print "File '$file' does not exist\n";
    next;
  }
  sysopen(ROM, $file, O_RDWR)
    or die "Error opening '$file'\n";
  my $size = -s $file;

  binmode(ROM);

  my $magic;
  my $ct;
  my $csum0 = -1;
  my $dirty = 1;

  if (($size & 0x1F) == 0x10)
  {
    #possibly valid header found
    $size -= 16;
    my $curhdr;
    sysread(ROM, $curhdr, 16);
    ($magic, $ct, $csum0, undef) = unpack("A4NNN", $curhdr);
    $dirty = 0;
  }
  else
  {
    #check for valid image size
    if (!exists($Types{$size}))
    {
      printf(STDERR "Unsupported image size: '$file' size=$size\n");
      next;
    }
    print "\nMissing header, adding...\n";
  }

  print "\n     ROM: $file\n";

  my $body;
  sysread(ROM, $body, $size);

  my $csum = 0;
  foreach my $bv (unpack('C*', $body))
  {
    $csum += $bv;
  }

  print "Checksum: ";
  if ($csum != $csum0)
  {
    if ($dirty)
    {
      print "New Header - Computing\n";
    }
    else
    {
      print "Bad\n";
    }
  }
  else
  {
    print "OK\n";
  }

  if (defined $ct)
  {
    printf "    Type: %2d - %s\n", $ct, $TypeNames[$ct];
  }
  print "\nSelect cartridge type:\n\n";

  my $vt = $Types{$size};
  foreach my $_t (@$vt)
  {
    printf " %2d) %s\n", $_t, $TypeNames[$_t];
  }
  print "\n  Just press [Enter] to keep current header value.\n\n";
  print "    Choose: ";

  while (1)
  {
    my $key;

    $key = <STDIN>;

    if ($dirty)
    {
      next unless ($key ge 0);
    }
    else
    {
      next unless ($key eq chr(10) or $key eq chr(13) or ($key ge 'A' and $key le 'Z'));
    }

    if ($key eq chr(10) or $key eq chr(13))
    {
      last;
    }

    my $found;
    map($found |= ($_ == $key) ? 1 : 0, @$vt);

    if ($found)
    {
      $ct = $key;
      last;
    }

  }

  #rewrite image file with new/valid header
  seek(ROM, 0, 0);
  my $hdr = pack('A4NNN', 'CART', $ct, $csum, 0);
  syswrite(ROM, $hdr, 16);
  if ($dirty)
  {
    syswrite(ROM, $body, $size);
  }
  close(ROM);

}
