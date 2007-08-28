#!/usr/bin/perl
use strict;

my $ver;
my $newver;

my $rewrite = 0;

open(IN, "configure.ac");
($ver) = grep {$_=~/^AC_INIT\(.*\[(\d+\.\d+)\]/ and $_= $1} (<IN>);

my ($maj,$min) = split /\./, $ver; 
my ($newmaj,$newmin); 

while (1) {
    print "Old release $ver, enter new relase ($maj.".($min+1)."): ";
    $newver = <>;
    chomp $newver;
    if ($newver =~ /^\s*$/) {
    	$newmaj = $maj;
    	$newmin = $min+1;
    	last;
    }
    elsif ($newver =~ /^(\d+)\.(\d+)$/) {
    	$newmaj = $1; $newmin = $2;
        if ($newmaj == $maj and $newmin == $min) {
            print "You are about to rewite release old($ver) = new($newmaj.$newmin)\n";
            print "Are you sure (N/y)?: ";
            my $a = <>;
            next unless $a =~ /^y/i;
            $rewrite = 1;
            last;
        }
    	elsif ($newmaj < $maj or ($newmaj == $maj and $newmin <= $min)) {
            print "Error, new version ($newmaj.$newmin) <= old version ($maj,$min)\n";
    		next;
    	}
    	else {
    		last;
    	}
    }
    print "Release syntax error, expected \"d.d\", try egain\n";
}

$newver = "$newmaj.$newmin";

print "$ver -> $newver\n";

my @date = localtime();
my $date = sprintf("%04d-%02d-%02d",$date[5]+1900, $date[4], $date[3]);

print "Enter date ($date):";

while (1) {
    my $newdate = <>;

    chomp $newdate;

    unless ($newdate =~ /^\s*$/) {
	   unless ($newdate =~ /^\s*$/) {
	       print "data syntax error, expected dddd-dd-dd, try again\n";
	       next;
	   }
	   $date = $newdate;
	   last;
    }
    else {
        last;
    }
}

print "Date = $date\n";

my ($V) = $ver;

$V =~ s/\./\\./g;

unless ($rewrite) {
    _system("cvs commit -m. configure.ac");
    _system("sed '/^AC_INIT/ s/\\[$V\\]/[$newver]/' < configure.ac > configure.ac.new");
    _system("mv configure.ac.new configure.ac");
    _system("cp NEWS NEWS.sav");
    _system("echo '$date Version $newver\n\t*\n\n'> NEWS");
    _system("cat NEWS.sav >> NEWS");
}
_system("vi NEWS");

_system("autoconf");
_system("automake");

my $REWRITED = $rewrite ? "(rewrited)":"";
_system("cvs commit -m 'Release $newver$REWRITED'");
_system("cvs tag rel-$newmaj-$newmin");

_system("make dist");

sub _system {
	print join "", @_;
	print "\n";
	system(@_) == 0 or die "\ncommand failed\n";
	
}

