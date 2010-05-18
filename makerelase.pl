#!/usr/bin/perl
use strict;

my $ver;
my $newver;

my $rewrite = 0;

open(IN, "configure.ac");
($ver) = grep {$_=~/^AC_INIT\(.*\[(\d+\.\d+(-\w+\d)?)\]/ and $_= $1} (<IN>);

my $verhint = $ver;

$verhint =~ s/(\d+)$/$1+1/e;

while (1) {
    print "Old release $ver, enter new relase ($verhint): ";
    $newver = <>;
    
    chomp $newver;
    $newver =~ s/^\s*//;
    $newver =~ s/\s*$//;
    
    
    if ($newver =~ /^\s*$/) {
    	$newver = "$verhint";
    	last;
    }
    elsif ($newver =~ /^(\d+)\.(\d+)(-\w+)?$/) {
        if ("$newver" eq "$ver") {
            print "New version ($newver) is the same as old version ($ver)\n";
            print "Are you sure?: ";
            $a =<>;
            if ($a=~/^y/i) {last;}
            next;
        }
    	else {
    		last;
    	}
    }
    print "Release syntax error, expected \"d.d[-s]\", try egain\n";
}

print "$ver -> $newver\n";

my @date = localtime();
my $date = sprintf("%04d-%02d-%02d",$date[5]+1900, $date[4]+1, $date[3]);

print "Enter date ($date):";

while (1) {
    my $newdate = <>;

    chomp $newdate;

    unless ($newdate =~ /^\s*$/) {
	   unless ($newdate =~ /^\d\d\d\d-\d\d-\d\d$/) {
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

my $updatecvs;
while (1) {
    print "Update cvs (y/n)? ";
    my $a = <>;
    if ($a  =~ /y/i) {
        $updatecvs = 1;
        last;
    }
    elsif ($a  =~ /n/i) {
        $updatecvs = 0;
        last;
    }
}

my ($V) = $ver;

$V =~ s/\./\\./g;
if ($updatecvs) {
    _system("cvs commit -m. configure.ac");
}

_system("sed '/^AC_INIT/ s/\\[$V\\]/[$newver]/' < configure.ac > configure.ac.new");
_system("sed '2 {s/Ver .*/Ver $newver ($date)/}' < README > README.new");
_system("mv configure.ac.new configure.ac");
_system("mv README.new README");

foreach ('ChangeLog', 'NEWS') {
    my $F = $_;
    print "\nPlease edit $F now (type enter)\n";
    <>; 
    _system("cp $F $F.sav");
    _system("echo '$date Version $newver\n\    *\n\n'> $F");
    _system("cat $F.sav >> $F");
    _system("vi $F");
}


_system("autoconf");
_system("automake");

my $tag = $newver;
$tag =~ s/\./-/g;
print "TAG = $tag\n";
if ($updatecvs) {
    _system("cvs commit -m 'Release $newver'");
    _system("cvs tag -d rel-$tag");
    _system("cvs tag -c rel-$tag");
}

_system("make dist");

sub _system {
	print join "", @_;
	print "\n";
	system(@_) == 0 or die "\ncommand failed\n";
}


