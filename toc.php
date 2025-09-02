<?php
// generate a table of contents from <h2> headers, and replace the <!-- h2toc --> comment with it
$h2re = '/<h2>(.*)<\/h2>/';

function h2toc_gentoc($filename, $name, $visname = null) {
    global $h2re;   // zelfde als Python global h2re

    if ($visname === null) {
        $visname = $filename;
    }

    $f = fopen($filename, "r");
    $orig = fread($f, filesize($filename));
    fclose($f);

    $headings = [];
    $changed = [];

    $c = 1;
    $where = null;
    foreach (explode("\n", $orig) as $line) {
        if (strpos($line, "<!-- h2toc -->") !== false) {
            $where = count($changed); // mark the pos where the toc must be generated
        }
        if (preg_match($h2re, $line, $m)) {
            $headings[] = $m[1];
            $line = sprintf('<a id="%s-%d"></a>', $name, $c) . $line;
            $c += 1;
        }
        $changed[] = $line;
    }

    if ($where === null) {
        echo "no <!-- h2toc --> comment found in $filename - table of contents not generated\n";
        return;
    }

    $toc = "<ul>\n";
    foreach ($headings as $i => $subj) {
        $toc .= sprintf(
            '<li><a href="%s#%s-%d">%s</a></li>' . "\n",
            $visname, $name, $i+1, $subj
        );
    }
    $toc .= "</ul>";
    $changed[$where] = $toc;

    $f = fopen($filename, "w");
    fwrite($f, implode("\n", $changed));
    fclose($f);
}
?>
