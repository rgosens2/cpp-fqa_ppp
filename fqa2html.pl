#!/usr/bin/perl
use strict;
use warnings;
use feature 'say';
use POSIX qw(strftime);

# load h2toc module containing h2toc_gentoc()
require "./toc.pl"; 

# Zet standaard tijdzone op UTC
$ENV{TZ} = "UTC";

our $title;

our $fqa; # file handle
our $fqa_page = "";
our $section = "";

our $sp = "";

our $esc2mark = {
    '/' => ['<i>', '</i>'],
    '|' => ['<code>', '</code>'],
    '@' => ['<pre>', '</pre>'],
};

our $plain2html = {
    '"' => "&quot;",
    "'" => "&#39;",
    "&" => "&amp;",
    "<" => "&lt;",
    ">" => "&gt;",
};


our $out = "";   # output buffer
our $sp2 = "";   # single page indicator

our $site = $ENV{"FQA_SITE"} // "";

# in oktober 2007 verdween parashift.com uit de DNS
our $faq_site = "http://www.parashift.com/c++-faq-lite";

our $style2 = '<link rel="stylesheet" type="text/css" href="style.css">';

our $ga = '';   # eventueel je GA-code of extra content

our $end_of_doc = sprintf(
    '<hr>
<small class="part">Copyright &copy; 2007-%d <a href="http://yosefk.com">Yossi Kreinin</a><br>
<code>revised %s</code></small>
%s
</body>
</html>',
    (strftime("%Y", gmtime)),
    (strftime("%d %B %Y", gmtime)),
    $ga
);

# reguliere expressies
our $re_link = qr{\[http:([^ ]+) ([^\]]+)\]};
our $re_int  = qr{\[(\d+)\.(\d+) ([^\]]+)\]};
our $re_corr = qr{\[corr\.(\d+) ([^\]]+)\]};

# hash: nummer naar sectienaam
my %num2sec = (
    6  => "picture",
    7  => "class",
    8  => "ref",
    9  => "inline",
    10 => "ctors",
    11 => "dtor",
    12 => "assign",
    13 => "operator",
    14 => "friend",
    15 => "io",
    16 => "heap",
    17 => "exceptions",
    18 => "const",
    19 => "inheritance-basics",
    20 => "inheritance-virtual",
    21 => "inheritance-proper",
    22 => "inheritance-abstract",
    23 => "inheritance-mother",
    25 => "inheritance-multiple",
    27 => "smalltalk",
    32 => "mixing",
    33 => "function",
    35 => "templates"
);


sub main {
    my ($argv) = @_;
    if (@$argv != 2) {
        die "usage: $argv->[0] <input C++ FQA text>\n";
    }
    run($argv->[1]);
}

sub run_to {
    my ($arg, $stream, $sp2) = @_;
    our $sp = $sp2;   # globale single-page indicator

    # TEST:
    # print "In run_to sp = $sp\n";
    # NOTE: Python schreef direct naar stream via fwrite
    my $content = run($arg, $sp);  # run retourneert string
    print $stream $content;
}

sub f2h_doit {
    my ($arg) = @_;
    open(my $htmlFile, ">", "$arg.html") or die "Cannot open file: $arg.html";
    run_to("$arg.fqa", $htmlFile);
    close($htmlFile);
}

#######################################
# These were nested in run()
# Vervang HTML-entities
sub replace_html_ent {
    my ($s) = @_;
    our $plain2html;
    my $o = "";
    for (my $i = 0; $i < length($s); $i++) {
        my $c = substr($s, $i, 1);
        $o .= exists $plain2html->{$c} ? $plain2html->{$c} : $c;
    }
    return $o;
}

# Links vervangen (http, interne verwijzingen, correcties)
sub replace_links {
    my ($s) = @_;
    our ($re_link, $re_int, $re_corr, %num2sec, $sp, $site);

    # [http:... ...]  →  `<a href="http:...">...</a>`
    $s =~ s{$re_link}{
        sprintf '`<a href="http:%s">%s</a>`', $1, $2
    }ge;

    # [S.N Caption] (bijv. [12.34 Caption])
    $s =~ s{$re_int}{
        my ($snum, $num, $cap) = (int($1), int($2), $3);
        my $sec    = $num2sec{$snum} // '';
        my $target = $sp ? $sp : "$sec.html";
        sprintf '`<a href="%s%s#fqa-%d.%d">%s</a>`',
                $site, $target, $snum, $num, $cap
    }ge;

    # [corr.N Caption]
    $s =~ s{$re_corr}{
        my ($num, $cap) = (int($1), $2);
        my $target = $sp ? $sp : "web-vs-fqa.html";
        sprintf '`<a class="corr" href="%s#correction-%d">%s</a>`',
                $target, $num, $cap
    }ge;

    return $s;
}

# Tekst naar HTML
sub str2html {
    my ($p) = @_;
    our ($esc2mark, $plain2html);

    $p = replace_links($p) if defined &replace_links;

    my $op = '';
    my %esc2state = map { $_ => 0 } keys %$esc2mark;
    my $asis = 0;

    for (my $i = 0; $i < length($p); $i++) {
        my $c = substr($p, $i, 1);

        if ($c eq '`') {
            $asis = !$asis;
        }
        elsif (!$asis && $c eq '\\') {
            $i++;
            $op .= substr($p, $i, 1) if $i < length($p);
        }
        elsif (!$asis && exists $esc2mark->{$c}) {
            my ($open, $close) = @{$esc2mark->{$c}};
            $op .= $esc2mark->{$c}->[$esc2state{$c}];
            $esc2state{$c} = 1 - $esc2state{$c};
        }
        elsif (!$asis && exists $plain2html->{$c}) {
            $op .= $plain2html->{$c};
        }
        else {
            $op .= $c;
        }
    }

    return $op;
}

# Paragraaf lezen
sub read_paragraph {
    our $fqa;
    my $p = "";
    my $line = <$fqa>;
    while (defined $line && $line =~ /\S/) {
        $p .= $line;
        $line = <$fqa>;
    }
    return undef if $p =~ /^\s*$/;
    return str2html($p =~ s/^\s+|\s+$//gr);
}

# Paragraaf printen
sub print_paragraph {
    my ($p) = @_;
    our $out;

    $p ||= "";

    # echo "<p>\n$p\n</p>\n\n";
    $out .= "<p>\n$p\n</p>\n\n";
}

# Heading printen
sub print_heading {
    my ($faq_page) = @_;
    our ($sp, $title, $faq_site, $style2, $out);

    # Perl klaagt over uninitialized variable
    # geef standaardwaarde als $faq_page leeg is of undef
    $faq_page ||= "";

    # NOTE: for the single page (sp == fqa.html) we only print h1, not a new html body etc
    if ($sp) {
        # TEST:
        print "We get here with sp = $sp\n";
        $out .= "<h1>" . replace_html_ent($title) . "</h1>";
        return $out;
    }

    my $below_heading;
    if ($faq_page) {
        $below_heading = '<small class="part">Part of <a href="index.html">C++ FQA Lite</a>.
            To see the original answers, follow the </small><b class="FAQ"><a href="' . $faq_site . '/' . $faq_page . '">FAQ</a></b><small class="part"> links.</small><hr>';
    } elsif (index($title, "Main page") == -1) {
        $below_heading = '<small class="part">Part of <a href="index.html">C++ FQA Lite</a></small><hr>';
    } else {
        $below_heading = "";
    }

    my $title_titag = "C++ FQA Lite: " . $title;
    my $title_h1tag = $title;
    if (index($title, "Main page") != -1) {
        $title_titag = "C++ Frequently Questioned Answers";
        $title_h1tag = "C++ FQA Lite: Main page";
    }

    $out .= "<!DOCTYPE html>
<html>
<head>
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>
<title>" . replace_html_ent($title_titag) . "</title>\n" .
$style2 .
"\n</head>
<body>
<h1>" . replace_html_ent($title_h1tag) . "</h1>\n" .
$below_heading .
"\n\n";
}

# Class Question
{
    package Question;
    sub new {
        my ($class, $title, $faq, $fqa) = @_;
        bless {
            title => $title,
            faq   => $faq || [],   # zorg dat het een arrayref is
            fqa   => $fqa || [],
        }, $class;
    }

    sub faq { shift->{faq} }
    sub fqa { shift->{fqa} }

    sub toc_line {
        my ($self, $num) = @_;
        our ($fqa_page, $sp);
        my $page = $sp ? $sp : $fqa_page;
        $page ||= "";
        #$section ||= "";
        # TEST:
        #print "Section: $section\n";
        return '<li><a href="' . $page . '#fqa-' . $section . '.' . $num . '">[' . $section . '.' . $num . '] ' . $self->{title} . '</a></li>';
    }

    sub title_lines {
        my ($self, $num) = @_;
        # NOTE: Do not use the global here. It will be empty!!!
        # NONO: if we declare our $section here it will be of the package: Question::$section
        # And that will be empty. We expressly need the global $section here: main::$section
        # That will contain the correct section number.
        #our $section; 
        # TEST:
        #print "Section: $section\n";
        return '<a id="fqa-' . $section . '.' . $num . '"></a>' . "\n" .
               '<h2>[' . $section . '.' . $num . '] ' . $self->{title} . '</h2>' . "\n";
    }

    sub replace_faq {
        my ($self, $num) = @_;
        our ($faq_site, $faq_page);
        $faq_site ||= "";
        $faq_page ||= "";
        #$section ||= "";
        my $repstr = '<b class="FAQ"><a href="' . $faq_site . '/' . $faq_page . '#faq-' . $section . '.' . $num . '">FAQ:</a></b>';
        $self->{faq}->[0] =~ s/FAQ:/$repstr/;
    }

    sub replace_fqa {
        my ($self) = @_;
        my $repstr = '<b class="FQA">FQA:</b>';
        $self->{fqa}->[0] =~ s/FQA:/$repstr/;
    }
}

# Vraag lezen
sub read_question {
    my $title = read_paragraph();
    return undef unless defined $title;

    my @faqps = (read_paragraph());
    while (defined $faqps[-1] && index($faqps[-1], "FQA:") == -1) {
        push @faqps, read_paragraph();
    }

    my @fqaps = (pop @faqps);
    while (defined $fqaps[-1] && index($fqaps[-1], "-END") == -1) {
        push @fqaps, read_paragraph();
    }
    pop @fqaps;

    return Question->new($title, \@faqps, \@fqaps);
}
#######################################


sub run {
    my ($arg, $sp) = @_;
    our $fqa;
    our $fqa_page;
    our $section;

    $sp //= 0;  # default to false

    $out = "";  # output buffer

    open $fqa, '<', $arg or die "Cannot open $arg: $!";
    $fqa_page = $arg;
    $fqa_page =~ s/\.fqa$/.html/;

    # NOTE: needed in str2html() so make it global
    # $esc2mark = {
    #     '/' => ['<i>', '</i>'],
    #     '|' => ['<code>', '</code>'],
    #     '@' => ['<pre>', '</pre>'],
    # };

    # $plain2html = {
    #     '"' => "&quot;",
    #     "'" => "&#39;",
    #     "&" => "&amp;",
    #     "<" => "&lt;",
    #     ">" => "&gt;",
    # };

    # first line: page title
    $title = <$fqa>;
    chomp $title;

    # second line: attributes
    my $fqa_line = <$fqa>;
    # TODO: change python:
    # {'section':12,'faq-page':'assignment-operators.html'}
    # to perl hash reference:
    # {'section'=>12,'faq-page'=>'assignment-operators.html'};
    #
    # NOTE: we need a hash reference {} not a normal hash ()
    #$fqa_line =~ s/\{/(/g;
    #$fqa_line =~ s/\}/);/g;
    $fqa_line =~ s/:/=>/g;

    # $fqa_line = "my %hash = " . $fqa_line;

    # TEST:
    #print $fqa_line; 

    # NOTE: dit kan ook met decode_json() ipv eval
    my $attrs = eval $fqa_line;  # eval to get hash ref
    die "Error parsing attributes: $@" if $@;

    if (%$attrs) {
        $section  = $attrs->{section};
        my $faq_page = $attrs->{'faq-page'};
        # TEST:
        #print "Section: $section\n";
        print_heading($faq_page);
    } else {
        print_heading(undef);
        while (1) {
            my $p = read_paragraph($fqa);
            if ($p) {
                print_paragraph($p);
            } else {
                $out .= $end_of_doc unless $sp;
                close $fqa;
                return $out;
            }
        }
    }

    # read first paragraph
    my $p = read_paragraph($fqa);
    print_paragraph($p);

    # read all questions
    my @qs;
    my $q = read_question($fqa);
    while ($q) {
        push @qs, $q;
        $q = read_question($fqa);
    }

    $out .= "<ul>\n";
    for my $i (0..$#qs) {
        $out .= $qs[$i]->toc_line($i + 1) . "\n";
    }
    $out .= "</ul>\n";

    for my $i (0..$#qs) {
        my $n = $i + 1;
        $out .= "\n";
        # TEST:
        #print "Section: $section\n";
        $out .= $qs[$i]->title_lines($n);
        $qs[$i]->replace_faq($n);
        for my $p (@{$qs[$i]->faq}) {
            print_paragraph($p);
        }
        $qs[$i]->replace_fqa();
        for my $p (@{$qs[$i]->fqa}) {
            print_paragraph($p);
        }
    }

    $out .= $end_of_doc unless $sp;
    close $fqa;
    return $out;
}


my $secindex = [
    ["picture", "Big Picture Issues"],
    ["class", "Classes and objects"],
    ["ref", "References"],
    ["inline", "Inline functions"],
    ["ctors", "Constructors"],
    ["dtor", "Destructors"],
    ["assign", "Assignment operators"],
    ["operator", "Operator overloading"],
    ["friend", "Friends"],
    ["io", "Input/output via <code>&lt;iostream&gt;</code> and <code>&lt;cstdio&gt;</code>"],
    ["heap", "Freestore management"],
    ["exceptions", "Exceptions"],
    ["const", "Const correctness"],
    ["inheritance-basics", "Inheritance - basics"],
    ["inheritance-virtual", "Inheritance - <code>virtual</code> functions"],
    ["inheritance-proper", "Inheritance - proper inheritance and substitutability"],
    ["inheritance-abstract", "Inheritance - abstract base classes"],
    ["inheritance-mother", "Inheritance - what your mother never told you"],
    ["inheritance-multiple", "Inheritance - multiple and <code>virtual</code> inheritance"],
    ["mixing", "How to mix C and C++"],
    ["function", "Pointers to member functions"],
    ["templates", "Templates"],
];

my $singlepageindex = <<'EOT';
C++ Frequently Questioned Answers
{}
This is a single page version of [http://yosefk.com/c++fqa C++ FQA Lite]. C++ is a general-purpose programming language, not necessarily suitable for your special purpose. [6.18 FQA] stands for "frequently
questioned answers". This FQA is called
"lite" because it questions the answers found in `<a href="http://www.parashift.com/c++-faq-lite/index.html">C++ FAQ Lite</a>`.

The single page version does not include most "metadata" sections such as [http://yosefk.com/c++fqa/faq.html the FQA FAQ].

`<h2>Table of contents</h2>`

`<ul>
<li><a href="%(sp)s#fqa-defective">Defective C++</a> - a list of major language defects</li>
<li>C++ Q&amp;A, structured similarly to C++ FAQ Lite, with links to the original FAQ answers</li>
<ul>
<li><a href="%(sp)s#fqa-picture">Big Picture Issues</a></li>
<li><a href="%(sp)s#fqa-class">Classes and objects</a></li>
<li><a href="%(sp)s#fqa-ref">References</a></li>
<li><a href="%(sp)s#fqa-inline">Inline functions</a></li>
<li><a href="%(sp)s#fqa-ctors">Constructors</a></li>
<li><a href="%(sp)s#fqa-dtor">Destructors</a></li>
<li><a href="%(sp)s#fqa-assign">Assignment operators</a></li>
<li><a href="%(sp)s#fqa-operator">Operator overloading</a></li>
<li><a href="%(sp)s#fqa-friend">Friends</a></li>
<li><a href="%(sp)s#fqa-io">Input/output via <code>&lt;iostream&gt;</code> and <code>&lt;cstdio&gt;</code></a></li>
<li><a href="%(sp)s#fqa-heap">Freestore management</a></li>
<li><a href="%(sp)s#fqa-exceptions">Exceptions</a></li>
<li><a href="%(sp)s#fqa-const">Const correctness</a></li>
<li><a href="%(sp)s#fqa-inheritance-basics">Inheritance - basics</a></li>
<li><a href="%(sp)s#fqa-inheritance-virtual">Inheritance - <code>virtual</code> functions</a></li>
<li><a href="%(sp)s#fqa-inheritance-proper">Inheritance - proper inheritance and substitutability</a></li>
<li><a href="%(sp)s#fqa-inheritance-abstract">Inheritance - abstract base classes</a></li>
<li><a href="%(sp)s#fqa-inheritance-mother">Inheritance - what your mother never told you</a></li>
<li><a href="%(sp)s#fqa-inheritance-multiple">Inheritance - multiple and <code>virtual</code> inheritance</a></li>
<li><a href="%(sp)s#fqa-mixing">How to mix C and C++</a></li>
<li><a href="%(sp)s#fqa-function">Pointers to member functions</a></li>
<li><a href="%(sp)s#fqa-templates">Templates</a></li>
</ul>
<li><a href="%(sp)s#fqa-web-vs-fqa">`FQA errors`</a> found by readers</li>
</ul>`
EOT
# RG: the closing shitter on a new line was preventing correct removal of
# a closing </p> in fqa.html

sub f2h_singlepage {
    my ($outname) = @_;
    our ($style2, $end_of_doc); # like PHP 'global'

    # open output file
    open my $outf, ">", $outname or die "Cannot open $outname: $!";

    # print HTML header
    print $outf sprintf <<"HDR", $style2;
<!DOCTYPE html>  
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
<title>C++ Frequently Questioned Answers</title>
%s
</head>
<body>
HDR

    # write temporary index file
    my $tmpfile = "sp-index.tmp.fqa";
    open my $tf, ">", $tmpfile or die "Cannot open $tmpfile: $!";
    (my $spindex = $singlepageindex) =~ s/%\(sp\)s/$outname/g;
    print $tf $spindex;
    close $tf;

    # TEST:
    print "In singlepage.\n";
    run_to($tmpfile, $outf, $outname);
    unlink $tmpfile;

    # define helper functions (nested in Python, top-level or closures in PHP)
    my $sec_ancor = sub {
        my ($secfname) = @_;
        print $outf sprintf('<a id="fqa-%s"></a>', substr($secfname, 0, -4));
    };

    my $sec_with_toc = sub {
        my ($filename, $name) = @_;
        $sec_ancor->($filename);
        my $tmpfile = "sec-with-toc.tmp.html";
        open my $tmp, ">", $tmpfile or die $!;
        run_to($filename, $tmp, $outname);
        close $tmp;
        h2toc_gentoc($tmpfile, $name, $outname); # aan te passen aan jouw toc.pl
        open my $tmp2, "<", $tmpfile or die $!;
        local $/;
        print $outf do { local $/; <$tmp2> };
        close $tmp2;
        unlink $tmpfile;
    };

    # now build the content
    $sec_with_toc->("defective.fqa", "defect");

    foreach my $pair (@$secindex) {
        my ($sec, $title) = @$pair;
        $sec_ancor->($sec . ".fqa");
        run_to($sec . ".fqa", $outf, $outname);
    }

    $sec_with_toc->("web-vs-fqa.fqa", "correction");

    print $outf $end_of_doc;
    close $outf;
}



