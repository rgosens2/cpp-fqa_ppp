<?php

$fqas = array_filter(scandir("."), function($f) {
    return substr($f, -4) === ".fqa";
});
$htmls = array_map(function($f) { return substr($f, 0, -4) . ".html"; }, $fqas);
$htmls[] = "fqa.html";
$files = $htmls;


function getoutput($cmd) {
    $tmp = "_out_.txt";
    $exit = system($cmd . " > " . $tmp, $retval);
    // TEST: We get 1. Why???
    // AHAH:
    // grep returns 0 if it finds a matching line
    //      returns 1 if it finds no matching line
    //.     returns >1 on other errors
    //echo $retval;
    if ($retval > 1) {
        throw new Exception("$cmd FAILED");
    }
    $r = file_get_contents($tmp);
    return $r;
}


///////////////////////////////////////
// We are getting a clean bill of health: 
// No warnings or errors were found.
// Changed <tt> to <code>
// Changed <a name to <a id
// Added missing </li>
// Deleted border=0 from img
///////////////////////////////////////
function tidy_file($f) {
    $o = getoutput(sprintf('tidy -e -utf8 %s 2>&1 | grep "errors"', $f));
    if (strpos($o, " 0 errors") !== false ||
        strpos($o, "No warnings or errors were found") !== false) {
        echo $f . ": " . rtrim($o) . "\n";
    } else {
        throw new Exception("ERRORS FOUND IN $f: " . rtrim($o));
    }
}


function do_tidy() {
    global $files;
    foreach ($files as $f) {
        $contents = file_get_contents($f);

        $tidyisms = ["ul", "pre", "h2"];
        foreach ($tidyisms as $t) {
            // Python: contents.replace("<p>\n<%s>" % t, "<%s>" % t)
            $contents = str_replace("<p>\n<$t>", "<$t>", $contents);
            $contents = str_replace("</$t>\n</p>\n", "</$t>\n", $contents);
        }

        file_put_contents($f, $contents);

        tidy_file($f);
    }
}

// echo "WARNING!! i'm not tidying post-me files for bitexact.py!! FIXME!!\n";
?>
