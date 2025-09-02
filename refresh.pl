#!/usr/bin/perl
use strict;
use warnings;
use File::Copy qw(move);
use File::Path qw(make_path);
use Cwd;

# load Perl versions of modules
require "./fqa2html.pl";
require "./toc.pl";
require "./tidy.pl";


sub doit {
    # find all .fqa files in current directory
    opendir(my $dh, ".") or die "Cannot open current directory: $!";
    my @fqaFiles = grep { /\.fqa$/ && -f $_ } readdir($dh);
    closedir $dh;

    # convert each .fqa file to HTML
    foreach my $f (@fqaFiles) {
        print "$f...\n";
        f2h_doit($f =~ s/\.fqa$//r);  # remove extension and call function
    }

    # build single-page version
    f2h_singlepage("fqa.html");

    # generate table of contents for selected files
    my %tocs = (
        "defective.html"   => "defect",
        "linking.html"     => "link",
        "faq.html"         => "faq",
        "web-vs-fqa.html"  => "correction",
        "web-vs-c++.html"  => "misfeature",
    );

    while (my ($k, $v) = each %tocs) {
        h2toc_gentoc($k, $v);
    }
}

# run conversion
doit();

# TEST:
#print "Before do_tidy()\n";

# tidy HTML
do_tidy();

# TEST:
#print "After do_tidy()\n";

# move generated .html files into "html3" subdirectory
opendir(my $dh, ".") or die "Cannot open current directory: $!";
my @htmlFiles = grep { /\.html$/ && -f $_ } readdir($dh);
closedir $dh;

my $cwd = getcwd();
my $dest = "$cwd/html3";
make_path($dest) unless -d $dest;

foreach my $f (@htmlFiles) {
    move($f, "$dest/$f") or warn "Cannot move $f: $!";
}
