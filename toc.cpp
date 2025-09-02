#include "toc.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>

void h2toc_gentoc(const std::string& filename, const std::string& name, const std::string& visname) {
    std::string vis = visname.empty() ? filename : visname;

    // open file en lees alles
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Cannot open file: " << filename << "\n";
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(infile, line)) {
        lines.push_back(line);
    }
    infile.close();

    std::vector<std::string> headings;
    std::vector<std::string> changed;
    size_t toc_pos = std::string::npos;

    std::regex h2re("<h2>(.*)</h2>");
    int c = 1;

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string l = lines[i];

        if (l.find("<!-- h2toc -->") != std::string::npos) {
            toc_pos = changed.size();  // markeer waar TOC komt
        }

        std::smatch m;
        if (std::regex_search(l, m, h2re)) {
            headings.push_back(m[1]);
            l = "<a id=\"" + name + "-" + std::to_string(c) + "\"></a>" + l;
            ++c;
        }

        changed.push_back(l);
    }

    if (toc_pos == std::string::npos) {
        std::cerr << "No <!-- h2toc --> comment found in " << filename << " - table of contents not generated\n";
        return;
    }

    // genereer TOC
    std::string toc = "<ul>\n";
    for (size_t i = 0; i < headings.size(); ++i) {
        toc += "<li><a href=\"" + vis + "#" + name + "-" + std::to_string(i + 1) + "\">" + headings[i] + "</a></li>\n";
    }
    toc += "</ul>";

    changed[toc_pos] = toc;

    // schrijf terug naar bestand
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Cannot write to file: " << filename << "\n";
        return;
    }

    for (const auto& l : changed) {
        outfile << l << "\n";
    }
    outfile.close();
}
