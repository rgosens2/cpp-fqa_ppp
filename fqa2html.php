<?php

// load h2toc module containing h2toc_gentoc()
require_once "toc.php";

date_default_timezone_set('UTC');

$out = ""; // output buffer
$sp2 = ""; // single page indicator

$site = getenv("FQA_SITE") ?: "";

// in October 2007, parashift.com disappeared from the DNS
$faq_site = "http://www.parashift.com/c++-faq-lite";

$style2 = '<link rel="stylesheet" type="text/css" href="style.css">';

$ga = ''; // eventueel je GA-code of extra content

$end_of_doc = sprintf(
    '<hr>
<small class="part">Copyright &copy; 2007-%d <a href="http://yosefk.com">Yossi Kreinin</a><br>
<code>revised %s</code></small>
%s
</body>
</html>',
    date("Y"),
    date("d F Y"),
    $ga
);

$re_link = '/\[http:([^ ]+) ([^\]]+)\]/';
$re_int  = '/\[(\d+)\.(\d+) ([^\]]+)\]/';
$re_corr = '/\[corr\.(\d+) ([^\]]+)\]/';

$num2sec = [
    6 => "picture",
    7 => "class",
    8 => "ref",
    9 => "inline",
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
];


function main($argv) {
    if (count($argv) != 2) {
        throw new Exception("usage: {$argv[0]} <input C++ FQA text>");
    }
    run($argv[1]);
}

function run_to($arg, $stream, $sp2 = false) {
    global $sp;
    $sp = $sp2; // indicate single page
    // TEST:
    //echo "In run_to sp = $sp\n";
    // NOTE: PHP schrijft direct naar de stream via fwrite, geen stdout override nodig
    $content = run($arg, $sp);  // run retourneert nu de output als string
    fwrite($stream, $content);
}

function f2h_doit($arg) {
    $htmlFile = fopen($arg . ".html", "w");
    if (!$htmlFile) {
        throw new Exception("Cannot open file: " . $arg . ".html");
    }
    run_to($arg . ".fqa", $htmlFile);
    fclose($htmlFile);
}

///////////////////////////////////////
// These were nested in run()
function replace_html_ent($s) {
    global $plain2html;
    $o = "";
    for ($i = 0; $i < strlen($s); $i++) {
        $c = $s[$i];
        $o .= $plain2html[$c] ?? $c;
    }
    return $o;
}

function replace_links($s) {
    global $re_link, $re_int, $re_corr;
    $s = preg_replace_callback($re_link, function($m) {
        return '`<a href="http:' . $m[1] . '">' . $m[2] . '</a>`';
    }, $s);
    $s = preg_replace_callback($re_int, function($m) {
        global $num2sec, $sp, $site;
        $snum = intval($m[1]);
        $sec = $num2sec[$snum];
        $num = intval($m[2]);
        $cap = $m[3];
        $sec = $sp ?: $sec . ".html";
        return '`<a href="' . $site . $sec . '#fqa-' . $snum . '.' . $num . '">' . $cap . '</a>`';
    }, $s);
    $s = preg_replace_callback($re_corr, function($m) {
        global $sp;
        $num = intval($m[1]);
        $cap = $m[2];
        $sec = $sp ?: "web-vs-fqa.html";
        return '`<a class="corr" href="' . $sec . '#correction-' . $num . '">' . $cap . '</a>`';
    }, $s);
    return $s;
}

function str2html($p) {
    global $esc2mark, $plain2html;
    $p = replace_links($p);
    $op = "";
    $i = 0;
    $esc2state = array_fill_keys(array_keys($esc2mark), 0);
    $pk = array_keys($plain2html);
    $asis = false;
    while ($i < strlen($p)) {
        $c = $p[$i];
        if ($c == "`") $asis = !$asis;
        elseif (!$asis && $c == "\\") {
            $i++;
            if ($i < strlen($p)) $op .= $p[$i];
        } elseif (!$asis && isset($esc2mark[$c])) {
            $op .= $esc2mark[$c][$esc2state[$c]];
            $esc2state[$c] = 1 - $esc2state[$c];
        } elseif (!$asis && isset($plain2html[$c])) $op .= $plain2html[$c];
        else $op .= $c;
        $i++;
    }
    return $op;
}

function read_paragraph() {
    global $fqa;
    $p = "";
    $line = fgets($fqa);
    while (trim($line) != "") {
        $p .= $line;
        $line = fgets($fqa);
    }
    if (trim($p) == "") return null;
    return str2html(trim($p));
}

function print_paragraph($p) {
    global $out;
    //echo "<p>\n$p\n</p>\n\n";
    $out .= "<p>\n$p\n</p>\n\n";
}

function print_heading($faq_page) {
    global $sp, $title, $faq_site, $style2, $out;
    // NOTE: for the single page (sp == fqa.html) we only print h1, not a new html body etc
    // So we should return here.
    if ($sp) {
        // TEST:
        echo "We get here with sp = $sp\n";
        //echo "<h1>" . replace_html_ent($title) . "</h1>";
        $out .= "<h1>" . replace_html_ent($title) . "</h1>";
        return $out;
    }
    if ($faq_page) {
        $below_heading = '<small class="part">Part of <a href="index.html">C++ FQA Lite</a>.
            To see the original answers, follow the </small><b class="FAQ"><a href="' . $faq_site . '/' . $faq_page . '">FAQ</a></b><small class="part"> links.</small><hr>';
    } elseif (strpos($title, "Main page") === false) {
        $below_heading = '<small class="part">Part of <a href="index.html">C++ FQA Lite</a></small><hr>';
    } else $below_heading = "";
    $title_titag = "C++ FQA Lite: " . $title;
    $title_h1tag = $title;
    if (strpos($title, "Main page") !== false) {
        $title_titag = "C++ Frequently Questioned Answers";
        $title_h1tag = "C++ FQA Lite: Main page";
    }
    //echo "<!DOCTYPE html>
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

class Question {
    public $title;
    public $faq;
    public $fqa;
    function __construct($title, $faq, $fqa) {
        $this->title = $title;
        $this->faq = $faq;
        $this->fqa = $fqa;
    }
    function toc_line($num) {
        global $fqa_page, $section, $sp;
        $page = $sp ?: $fqa_page;
        return '<li><a href="' . $page . '#fqa-' . $section . '.' . $num . '">[' . $section . '.' . $num . '] ' . $this->title . '</a></li>';
    }
    function title_lines($num) {
        global $section;
        return '<a id="fqa-' . $section . '.' . $num . '"></a>' . "\n" . '<h2>[' . $section . '.' . $num . '] ' . $this->title . '</h2>' . "\n";
    }
    function replace_faq($num) {
        global $faq_site, $faq_page, $section;
        $repstr = '<b class="FAQ"><a href="' . $faq_site . '/' . $faq_page . '#faq-' . $section . '.' . $num . '">FAQ:</a></b>';
        $this->faq[0] = str_replace("FAQ:", $repstr, $this->faq[0]);
    }
    function replace_fqa() {
        $repstr = '<b class="FQA">FQA:</b>';
        $this->fqa[0] = str_replace("FQA:", $repstr, $this->fqa[0]);
    }
}

function read_question() {
    $title = read_paragraph();
    if ($title === null) return null;
    $faqps = [$p = read_paragraph()];
    while (strpos(end($faqps), "FQA:") === false) {
        $faqps[] = read_paragraph();
    }
    $fqaps = [array_pop($faqps)];
    while (strpos(end($fqaps), "-END") === false) {
        $fqaps[] = read_paragraph();
    }
    array_pop($fqaps);
    return new Question($title, $faqps, $fqaps);
}
///////////////////////////////////////


function run($arg, $sp = false) {
    global $out, $title, $end_of_doc, $fqa, $fqa_page, $section, $esc2mark, $plain2html;
    $out = ""; // Output buffer

    $fqa = fopen($arg, "r");
    $fqa_page = str_replace(".fqa", ".html", $arg);

    $esc2mark = [
        "/" => ["<i>", "</i>"],
        "|" => ["<code>", "</code>"],
        "@" => ["<pre>", "</pre>"]
    ];

    $plain2html = [
        '"' => "&quot;",
        "'" => "&#39;",
        "&" => "&amp;",
        "<" => "&lt;",
        ">" => "&gt;"
    ];

    // if (!function_exists('replace_html_ent')) {
    
    // }

    // if (!function_exists('replace_links')) {
    
    // }

    // if (!function_exists('str2html')) {
    
    // }

    // if (!function_exists('read_paragraph')) {
    
    // }

    // if (!function_exists('print_paragraph')) {
   
    // }

    // first line: page title
    $title = trim(fgets($fqa));

    // TEST:
    echo "Title: $title\n";

    // if (!function_exists('print_heading')) {
    
    // }

    // second line: attributes
    $fqa_line = fgets($fqa);
    // TODO: change python:
    // {'section':12,'faq-page':'assignment-operators.html'}
    // to php:
    // ['section'=>12,'faq-page'=>'assignment-operators.html'];
    //
    $fqa_line = str_replace(
        ["{", "}", ":"],
        ["[", "];", "=>"],
        $fqa_line);

    // TEST:
    //echo $fqa_line;

    // NOTE: need return in PHP eval
    $attrs = eval("return " . $fqa_line);   // careful: eval in PHP executes code

    if (count($attrs)) {
        $section  = $attrs["section"];
        $faq_page = $attrs["faq-page"];
        print_heading($faq_page);
    } else {
        print_heading(null);
        // this isn't a FAQ section - read and print all paragraphs
        while (true) {
            $p = read_paragraph();
            if ($p) {
                print_paragraph($p);
            } else {
                if (!$sp) {
                    //echo $end_of_doc;
                    $out .= $end_of_doc;
                }
                // and close file
                fclose($fqa);
                return $out; // Return the output buffer
            }
        }
    }
    // echo sprintf("formatting C++ FQA page %s (FAQ page: %s, section number %d)\n", $title, $faq_page, $section);

    // we get here if this is a FAQ section
    // if (!class_exists('Question')) {
    
    // }

    // if (!function_exists('read_question')) {
   
    // }

    $p = read_paragraph();
    print_paragraph($p);

    $qs = [];
    $q = read_question();
    while ($q) {
        $qs[] = $q;
        $q = read_question();
    }

    //echo "<ul>\n";
    //foreach ($qs as $i => $q) echo $q->toc_line($i + 1) . "\n";
    //echo "</ul>\n";
    $out .=  "<ul>\n";
    foreach ($qs as $i => $q) $out .= $q->toc_line($i + 1) . "\n";
    $out .=  "</ul>\n";
    

    foreach ($qs as $i => $q) {
        $n = $i + 1;
        //echo "\n";
        //echo $q->title_lines($n);
        $out .= "\n";
        $out .= $q->title_lines($n);
        $q->replace_faq($n);
        foreach ($q->faq as $p) print_paragraph($p);
        $q->replace_fqa();
        foreach ($q->fqa as $p) print_paragraph($p);
    }

    if (!$sp) $out .= $end_of_doc;
    fclose($fqa);

    return $out; // Return the output buffer
}


$secindex = [
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
    ["templates", "Templates"]
];

$singlepageindex = <<<EOT
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
EOT;
// RG: the closing shitter on a new line was preventing correct removal of
// a closing </p> in fqa.html

function f2h_singlepage($outname) {
    global $style2, $singlepageindex, $secindex, $end_of_doc;

    // open output file
    $outf = fopen($outname, "w");

    // print HTML header
    fwrite($outf, sprintf(
"<!DOCTYPE html>
<html>
<head>
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>
<title>C++ Frequently Questioned Answers</title>
%s
</head>
<body>
", $style2));

    // write temporary index file
    $tmpfile = "sp-index.tmp.fqa";
    $tf = fopen($tmpfile, "w");
    fwrite($tf, str_replace("%(sp)s", $outname, $singlepageindex));
    fclose($tf);

    // TEST:
    echo "In singlepage.\n";
    run_to($tmpfile, $outf, $outname);
    unlink($tmpfile);

    // define helper functions (nested in Python, top-level or closures in PHP)
    $sec_ancor = function($secfname) use ($outf) {
        fwrite($outf, sprintf('<a id="fqa-%s"></a>', substr($secfname, 0, -4)));
    };

    $sec_with_toc = function($filename, $name) use ($outf, $outname, $sec_ancor) {
        $sec_ancor($filename);
        $tmpfile = "sec-with-toc.tmp.html";
        $tmp = fopen($tmpfile, "w");
        run_to($filename, $tmp, $outname);
        fclose($tmp);
        h2toc_gentoc($tmpfile, $name, $outname); // aan te passen aan jouw toc.php
        $tmp2 = fopen($tmpfile, "r");
        fwrite($outf, stream_get_contents($tmp2));
        fclose($tmp2);
        unlink($tmpfile);
    };

    // now build the content
    $sec_with_toc("defective.fqa", "defect");

    foreach ($secindex as $pair) {
        list($sec, $title) = $pair;
        $sec_ancor($sec . ".fqa");
        run_to($sec . ".fqa", $outf, $outname);
    }

    $sec_with_toc("web-vs-fqa.fqa", "correction");

    fwrite($outf, $end_of_doc);
    fclose($outf);
}

?>
