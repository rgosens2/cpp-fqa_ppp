// Build command: g++ -std=c++17 refresh.cpp fqa2html.cpp toc.cpp tidy.cpp -o refresh
// Debug build: -g -O0
// NOTE: Spinning Locals circle during debugging?
// See: https://github.com/microsoft/vscode/issues/206493
// Solution: https://stackoverflow.com/questions/70245851/how-to-debug-in-vs-code-using-lldb
// Use "type": "lldb" in launch.json

#include "refresh.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm> // voor std::copy_if
#include <fstream>

#include "fqa2html.h"
#include "toc.h"
#include "tidy.h"

namespace fs = std::filesystem;

void doit() {
    // zoek alle .fqa bestanden in de huidige directory
    std::vector<std::string> fqaFiles;
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file() && entry.path().extension() == ".fqa") {
            fqaFiles.push_back(entry.path().filename().string());
        }
    }

    // converteer elk .fqa bestand naar HTML
    for (const auto& f : fqaFiles) {
        std::cout << f << "...\n";
        f2h_doit(f.substr(0, f.size() - 4));
    }

    // bouw single-page versie
    f2h_singlepage("fqa.html");

    // genereer table of contents voor geselecteerde bestanden
    std::vector<std::pair<std::string, std::string>> tocs = {
        {"defective.html", "defect"},
        {"linking.html", "link"},
        {"faq.html", "faq"},
        {"web-vs-fqa.html", "correction"},
        {"web-vs-c++.html", "misfeature"}
    };

    for (const auto& kv : tocs) {
        h2toc_gentoc(kv.first, kv.second);
    }
}

int main() {
    // voer conversie uit
    doit();

    // tidy verwerking
    do_tidy();

    // verplaats gegenereerde .html bestanden naar "html4" subdirectory
    fs::path cwd = fs::current_path();
    fs::path html4Dir = cwd / "html4";
    if (!fs::exists(html4Dir)) {
        fs::create_directory(html4Dir);
    }

    for (const auto& entry : fs::directory_iterator(cwd)) {
        if (entry.is_regular_file() && entry.path().extension() == ".html") {
            fs::rename(entry.path(), html4Dir / entry.path().filename());
        }
    }

    return 0;
}
