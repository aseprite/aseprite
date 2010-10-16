#! /usr/bin/perl
#
#  Perl script for comparing the output of two test.exe profile logs,
#  converting the time values into a percentage difference.
#
#  Usage: cmplog profile1.log profile2.log


if (not (($f1 = shift) && ($f2 = shift))) {
   print "Usage: cmplog file1.log file2.log\n";
   exit
}

open A, $f1 or die "Can't open first input file ($f1)\n";
open B, $f2 or die "Can't open second input file ($f2)\n";

print "Comparing test profile logs $f1 and $f2\n";

while ($l1 = <A> and $l2 = <B>) {
   if ($l1 =~ /Hardware acceleration/) {
      $l1 = <A>;
      do { $l1 = <A>; } until ($l1 =~ /^\r?$/);
   }

   if ($l2 =~ /Hardware acceleration/) {
      $l2 = <B>;
      do { $l2 = <B>; } until ($l2 =~ /^\r?$/);
   }

   if ($l1 =~ /\s+(.+)\s+- ([0-9]+|N\/A)/) {
      $name = $1;
      $v1 = $2;
      $l2 =~ /- ([0-9]+|N\/A)/;
      $v2 = $1;
      $p = ($v1 == 0) || ($v2 == 0) ? "N/A" : int($v2*100/$v1) . "%";
      $name .= " " x (32 - length($name));
      print "\t$name = $p\n";
   }
   elsif ($l1 =~ /^\r?(.*):\r?$/) {
      print "\n\n$1:\n\n";
   }
}
