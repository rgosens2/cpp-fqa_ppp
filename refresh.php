<?php
// load modules (PHP versions of fqa2html.py and toc.py)
include_once "fqa2html.php";
include_once "toc.php";
include_once "tidy.php";


function doit() {
    // find all .fqa files in current directory
    $fqaFiles = array_filter(scandir("."), function($f) {
        return substr($f, -4) === ".fqa";
    });

    // convert each .fqa file to HTML
    foreach ($fqaFiles as $f) {
        echo $f . "...\n";
        f2h_doit(substr($f, 0, -4)); // assumption: f2h_doit exists in fqa2html.php
    }

    // build single-page version
    f2h_singlepage("fqa.html");

    // generate table of contents for selected files
    $tocs = [
        "defective.html" => "defect",
        "linking.html" => "link",
        "faq.html" => "faq",
        "web-vs-fqa.html" => "correction",
        "web-vs-c++.html" => "misfeature",
    ];

    foreach ($tocs as $k => $v) {
        h2toc_gentoc($k, $v); // assumption: h2toc_gentoc exists in toc.php
    }
}

// run conversion
doit();

// load tidy.php (PHP version of tidy.py)
do_tidy();


// move generated .html files into "html2" subdirectory
$htmlFiles = array_filter(scandir("."), function($f) {
    return substr($f, -5) === ".html";
});

$cwd = getcwd() . "/";
if (!is_dir($cwd . "html2")) {
    mkdir($cwd . "html2");
}

foreach ($htmlFiles as $f) {
    rename($cwd . $f, $cwd . "html2/" . $f);
}
?>
