#!/usr/bin/perl
use strict;
use warnings;

# generate a table of contents from <h2> headers,
# and replace the <!-- h2toc --> comment with it
my $h2re = qr{<h2>(.*)</h2>};

sub h2toc_gentoc {
    my ($filename, $name, $visname) = @_;
    $visname //= $filename;   # zelfde als "if ($visname === null) $visname = $filename;" in PHP

    # lees volledig bestand in
    open my $f, "<", $filename or die "Can't open $filename: $!";
    local $/;
    my $orig = <$f>;
    close $f;

    my @headings;
    my @changed;
    my $c = 1;
    my $where;

    foreach my $line (split /\n/, $orig) {
        if (index($line, "<!-- h2toc -->") != -1) {
            $where = scalar @changed;   # markeer positie waar de TOC moet komen
        }
        if ($line =~ $h2re) {
            push @headings, $1;
            $line = sprintf('<a id="%s-%d"></a>%s', $name, $c, $line);
            $c++;
        }
        push @changed, $line;
    }

    if (!defined $where) {
        print "no <!-- h2toc --> comment found in $filename - table of contents not generated\n";
        return;
    }

    my $toc = "<ul>\n";
    for my $i (0 .. $#headings) {
        $toc .= sprintf(
            '<li><a href="%s#%s-%d">%s</a></li>' . "\n",
            $visname, $name, $i+1, $headings[$i]
        );
    }
    $toc .= "</ul>";

    $changed[$where] = $toc;

    open my $out, ">", $filename or die "Can't write $filename: $!";
    print $out join("\n", @changed);
    close $out;
}
