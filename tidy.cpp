#include "tidy.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;

// globale lijst van HTML-bestanden
std::vector<std::string> files;

std::vector<std::string> getFiles() {
    std::vector<std::string> fqas;
    std::vector<std::string> htmls;

    // scan current directory for *.fqa
    for (auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string fname = entry.path().filename().string();
            if (fname.size() >= 4 && fname.substr(fname.size() - 4) == ".fqa") {
                fqas.push_back(fname);
            }
        }
    }

    // convert to .html names
    for (auto& f : fqas) {
        htmls.push_back(f.substr(0, f.size() - 4) + ".html");
    }

    // add single-page fqa.html
    htmls.push_back("fqa.html");

    return htmls;
}



// helper: run command en vang output op
std::string getoutput(const std::string& cmd) {
    std::string tmp = "_out_.txt";
    int retval = system((cmd + " > " + tmp + " 2>&1").c_str());

    if (retval > 1) {
        throw std::runtime_error(cmd + " FAILED");
    }

    std::ifstream infile(tmp);
    std::stringstream buffer;
    buffer << infile.rdbuf();
    return buffer.str();
}

// check file met tidy
void tidy_file(const std::string& f) {
    // TEST:
    //std::cout << "In tidy_file\n";

    std::string cmd = "tidy -e -utf8 " + f + " 2>&1 | grep \"errors\"";
    std::string o = getoutput(cmd);

    if (o.find(" 0 errors") != std::string::npos ||
        o.find("No warnings or errors were found") != std::string::npos) {
        std::cout << f << ": " << o;
    } else {
        throw std::runtime_error("ERRORS FOUND IN " + f + ": " + o);
    }
}

// vervang <p>\n<tag> ... </tag>\n</p> door <tag> ... </tag>
void do_tidy() {
    // TEST:
    //std::cout << "In do_tidy\n";

    files = getFiles();

    std::vector<std::string> tidyisms = {"ul", "pre", "h2"};

    for (const auto& f : files) {
        std::ifstream infile(f);
        if (!infile.is_open()) throw std::runtime_error("Cannot open file " + f);
        std::stringstream buffer;
        buffer << infile.rdbuf();
        std::string contents = buffer.str();
        infile.close();

        for (const auto& t : tidyisms) {
            std::string open_tag = "<p>\n<" + t + ">";
            std::string close_tag = "</" + t + ">\n</p>\n";
            std::string replacement_open = "<" + t + ">";
            std::string replacement_close = "</" + t + ">\n";

            size_t pos = 0;
            while ((pos = contents.find(open_tag, pos)) != std::string::npos) {
                contents.replace(pos, open_tag.size(), replacement_open);
            }

            pos = 0;
            while ((pos = contents.find(close_tag, pos)) != std::string::npos) {
                contents.replace(pos, close_tag.size(), replacement_close);
            }
        }

        // TEST:
        //std::cout << "QQQ\n";

        std::ofstream outfile(f);
        if (!outfile.is_open()) throw std::runtime_error("Cannot write file " + f);
        outfile << contents;
        outfile.close();

        tidy_file(f);
    }
}

