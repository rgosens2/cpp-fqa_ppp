#!/usr/bin/perl
use strict;
use warnings;
use File::Slurp;     # voor file_get_contents / put_contents equivalent
use File::Basename;

# Verzamel alle .fqa-bestanden in de huidige directory
opendir(my $dh, ".") or die "Cannot open current directory: $!";
my @fqas = grep { /\.fqa$/ } readdir($dh);
closedir $dh;

# print "\n\n\nMatched fqas:\n";
# print join("\n", @fqas), "\n";


# Converteer naar .html
my @htmls = map { (my $h = $_) =~ s/\.fqa$/.html/; $h } @fqas;
push @htmls, "fqa.html";
my @files = @htmls;

# print "\n\n\nMatched htmls:\n";
# print join("\n", @htmls), "\n";


# Voer een shellcommando uit, vang stdout in een tmp-bestand,
# en retourneer de inhoud als string
sub getoutput {
    my ($cmd) = @_;
    my $tmp = "_out_.txt";

    # system in Perl retourneert exitstatus << 8
    my $retval = system("$cmd > $tmp");
    $retval = $retval >> 8;

    # TEST: We get 1. Why???
    # AHAH:
    # grep returns 0 if it finds a matching line
    #      returns 1 if it finds no matching line
    #      returns >1 on other errors
    if ($retval > 1) {
        die "$cmd FAILED with exit=$retval\n";
    }

    my $r = read_file($tmp);
    return $r;
}


#########################################
# We are getting a clean bill of health:
# No warnings or errors were found.
# Changed <tt> to <code>
# Changed <a name to <a id
# Added missing </li>
# Deleted border=0 from img
#########################################
sub tidy_file {
    my ($f) = @_;

    # NOTE: do not forget 2>&1
    # It sends tidy stderr to stdout which we read
    my $o = getoutput(sprintf('tidy -e -utf8 %s 2>&1 | grep "errors"', $f));

    if (index($o, " 0 errors") != -1 ||
        index($o, "No warnings or errors were found") != -1) {
        print "$f: " . $o;
    } else {
        die "ERRORS FOUND IN $f: $o";
    }
}


sub do_tidy {
    foreach my $f (@files) {
        # TEST:
        #print join("\n", @files), "\n";
        #print "$f\n";

        my $contents = read_file($f);

        my @tidyisms = ("ul", "pre", "h2");
        foreach my $t (@tidyisms) {
            # Python: contents.replace("<p>\n<%s>" % t, "<%s>" % t)
            $contents =~ s/<p>\n<$t>/<$t>/g;
            $contents =~ s{</$t>\n</p>\n}{</$t>\n}g;
        }

        write_file($f, $contents);

        tidy_file($f);
    }
}

# echo "WARNING!! i'm not tidying post-me files for bitexact.py!! FIXME!!\n";
