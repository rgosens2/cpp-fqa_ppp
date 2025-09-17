#include "fqa2html.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <regex>
#include <unordered_map>

#include "toc.h"
#include "tidy.h"



// -------------------- Globals --------------------
std::string out; // Output buffer
std::string sp;  // Single page

std::string title;
std::ifstream fqa;
std::string fqa_page;
std::string faq_page;

int section;

std::string site = std::getenv("FQA_SITE") ? std::getenv("FQA_SITE") : "";

// in October 2007, parashift.com disappeared from the DNS
std::string faq_site = "http://www.parashift.com/c++-faq-lite";

std::string style2 = "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">";

std::string ga = "";

const std::string end_of_doc = create_end_of_doc();

std::regex re_link(R"(\[http:([^ ]+) ([^\]]+)\])");
std::regex re_int(R"(\[(\d+)\.(\d+) ([^\]]+)\])");
std::regex re_corr(R"(\[corr\.(\d+) ([^\]]+)\])");

std::map<char, std::vector<std::string>> esc2mark = {
    {'/', {"<i>", "</i>"}},
    {'|', {"<code>", "</code>"}},
    {'@', {"<pre>", "</pre>"}}
};

std::map<char, std::string> plain2html = {
    {'"', "&quot;"},
    {'\'', "&#39;"},
    {'&', "&amp;"},
    {'<', "&lt;"},
    {'>', "&gt;"}
};

std::map<int, std::string> num2sec = {
    {6, "picture"},
    {7, "class"},
    {8, "ref"},
    {9, "inline"},
    {10, "ctors"},
    {11, "dtor"},
    {12, "assign"},
    {13, "operator"},
    {14, "friend"},
    {15, "io"},
    {16, "heap"},
    {17, "exceptions"},
    {18, "const"},
    {19, "inheritance-basics"},
    {20, "inheritance-virtual"},
    {21, "inheritance-proper"},
    {22, "inheritance-abstract"},
    {23, "inheritance-mother"},
    {25, "inheritance-multiple"},
    {27, "smalltalk"},
    {32, "mixing"},
    {33, "function"},
    {35, "templates"}
};


// -------------------- Helper functions --------------------
std::string create_end_of_doc() {
    //setenv("TZ", "UTC", 1);  // set TZ environment variable
    //tzset();                 // apply new timezone setting

    // get current time
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::gmtime(&t);  // UTC like date_default_timezone_set('UTC')

    // format year
    int year = tm->tm_year + 1900;

    // format date as "d F Y" (example: "20 August 2025")
    std::ostringstream date_stream;
    date_stream << std::put_time(tm, "%d %B %Y");

    // build the HTML
    std::ostringstream out;
    out <<
        "<hr>\n"
        "<small class=\"part\">Copyright &copy; 2007-" << year <<
        " <a href=\"http://yosefk.com\">Yossi Kreinin</a><br>\n"
        "<code>revised " << date_stream.str() << "</code></small>\n"
        << ga << "\n"
        "</body>\n"
        "</html>";

    return out.str();
}

std::map<std::string, std::string> parseLine(const std::string& line) {
    std::map<std::string, std::string> result;

    // Match 'key' : 'value' OR 'key' : number
    // E.g.: {'section':12,'faq-page':'assignment-operators.html'}
    std::regex re(R"('([^']+)'\s*:\s*'([^']*)'|'([^']+)'\s*:\s*([0-9]+))");
    auto begin = std::sregex_iterator(line.begin(), line.end(), re);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch m = *it;
        if (m[1].matched) {        // capture group 1 matched
            result[m[1]] = m[2];   // string value
        } else {
            result[m[3]] = m[4];   // number as string
        }
    }
    return result;
}


void run_to(const std::string& arg, std::ofstream& stream, const std::string& sp2 = "") {
    sp = sp2; // indicate single page
    // TEST: std::cout << "In run_to sp = " << sp << "\n";
    // run returns the output as string
    std::string content = run(arg, sp);
    stream << content;
}


void f2h_doit(const std::string& arg) {
    std::ofstream htmlFile(arg + ".html");
    if (!htmlFile.is_open()) {
        throw std::runtime_error("Cannot open file: " + arg + ".html");
    }
    run_to(arg + ".fqa", htmlFile);
    htmlFile.close();
}




///////////////////////////////////////
// These were nested in run()
std::string replace_html_ent(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s) {
        auto it = plain2html.find(c);
        if (it != plain2html.end()) {
            o += it->second;
        } else {
            o += c;
        }
    }
    return o;
}

std::string replace_links(const std::string& input) {
    std::string s = input;

    // [http:URL TEXT]
    {
        //std::regex re_link(R"(\[http:([^ ]+) ([^\]]+)\])");
        std::string result;
        auto begin = std::sregex_iterator(s.begin(), s.end(), re_link);
        auto end = std::sregex_iterator();
        size_t last_pos = 0;
        for (auto it = begin; it != end; ++it) {
            const std::smatch& m = *it;
            result += s.substr(last_pos, m.position() - last_pos);
            result += "`<a href=\"http:" + m[1].str() + "\">" + m[2].str() + "</a>`";
            last_pos = m.position() + m.length();
        }
        result += s.substr(last_pos);
        s = result;
    }

    // [section.num CAP]
    {
        //std::regex re_int(R"(\[(\d+)\.(\d+) ([^\]]+)\])");
        std::string result;
        auto begin = std::sregex_iterator(s.begin(), s.end(), re_int);
        auto end = std::sregex_iterator();
        size_t last_pos = 0;
        for (auto it = begin; it != end; ++it) {
            const std::smatch& m = *it;
            result += s.substr(last_pos, m.position() - last_pos);
            int snum = std::stoi(m[1].str());
            int num = std::stoi(m[2].str());
            std::string cap = m[3].str();
            std::string sec = sp.empty() ? num2sec[snum] + ".html" : sp;
            result += "`<a href=\"" + site + sec + "#fqa-" + std::to_string(snum) + "." + std::to_string(num) + "\">" + cap + "</a>`";
            last_pos = m.position() + m.length();
        }
        result += s.substr(last_pos);
        s = result;
    }

    // [corr.num CAP]
    {
        //std::regex re_corr(R"(\[corr\.(\d+) ([^\]]+)\])");
        std::string result;
        auto begin = std::sregex_iterator(s.begin(), s.end(), re_corr);
        auto end = std::sregex_iterator();
        size_t last_pos = 0;
        for (auto it = begin; it != end; ++it) {
            const std::smatch& m = *it;
            result += s.substr(last_pos, m.position() - last_pos);
            int num = std::stoi(m[1].str());
            std::string cap = m[2].str();
            std::string sec = sp.empty() ? "web-vs-fqa.html" : sp;
            result += "`<a class=\"corr\" href=\"" + sec + "#correction-" + std::to_string(num) + "\">" + cap + "</a>`";
            last_pos = m.position() + m.length();
        }
        result += s.substr(last_pos);
        s = result;
    }

    return s;
}

std::string str2html(const std::string& p) {
    std::string s = replace_links(p); // your previously converted replace_links
    std::string op;
    bool asis = false;

    // Initialize esc2state map
    std::unordered_map<char, int> esc2state;
    for (const auto& kv : esc2mark) {
        esc2state[kv.first] = 0;
    }

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '`') {
            // TEST:
            //std::cout << "backtick\n";
            asis = !asis;
        }
        else if (!asis && c == '\\') {
            ++i;
            if (i < s.size()) op += s[i];
        }
        else if (!asis && esc2mark.count(c)) {
            op += esc2mark[c][esc2state[c]];
            esc2state[c] = 1 - esc2state[c];
        }
        else if (!asis && plain2html.count(c)) {
            op += plain2html[c];
        }
        else {
            op += c;
        }
    }

    return op;
}

// Helper to trim whitespace from both ends
std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    auto end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

// Read one paragraph from the file
std::string read_paragraph() {
    std::string p, line;
    if (!std::getline(fqa, line)) return ""; // EOF
    while (trim(line) != "") {
        p += line + "\n";
        if (!std::getline(fqa, line)) break;
    }
    if (trim(p).empty()) return ""; // equivalent to null in PHP
    return str2html(trim(p));
}

// Append a paragraph to the output
void print_paragraph(const std::string& p) {
    out += "<p>\n" + p + "\n</p>\n\n";
}

void print_heading(const std::string& faq_page) {
    // single page: just print h1
    if (!sp.empty()) {
        // TEST:
        std::cout << "We get here with sp=" << sp << " and faq_page=" << faq_page << "\n";
        out += "<h1>" + replace_html_ent(title) + "</h1>\n";
        return;
    }

    std::string below_heading;

    if (!faq_page.empty()) {
        below_heading = "<small class=\"part\">Part of <a href=\"index.html\">C++ FQA Lite</a>. "
                        "To see the original answers, follow the </small><b class=\"FAQ\"><a href=\"" +
                        faq_site + "/" + faq_page + "\">FAQ</a></b><small class=\"part\"> links.</small><hr>";
    }
    else if (title.find("Main page") == std::string::npos) {
        below_heading = "<small class=\"part\">Part of <a href=\"index.html\">C++ FQA Lite</a></small><hr>";
    }
    else {
        below_heading = "";
    }

    std::string title_titag = "C++ FQA Lite: " + title;
    std::string title_h1tag = title;

    if (title.find("Main page") != std::string::npos) {
        title_titag = "C++ Frequently Questioned Answers";
        title_h1tag = "C++ FQA Lite: Main page";
    }

    out += "<!DOCTYPE html>\n<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n"
           "<title>" + replace_html_ent(title_titag) + "</title>\n" +
           style2 +
           "\n</head>\n<body>\n<h1>" + replace_html_ent(title_h1tag) + "</h1>\n" +
           below_heading + "\n\n";
}

class Question {
public:
    std::string title;
    std::vector<std::string> faq;
    std::vector<std::string> fqa;

    Question(const std::string& t, const std::vector<std::string>& f, const std::vector<std::string>& fq)
        : title(t), faq(f), fqa(fq) {}

    std::string toc_line(int num) {
        std::string page = sp.empty() ? fqa_page : sp;
        std::ostringstream oss;
        oss << "<li><a href=\"" << page << "#fqa-" << section << "." << num << "\">[" << section << "." << num << "] " << title << "</a></li>";
        return oss.str();
    }

    std::string title_lines(int num) {
        std::ostringstream oss;
        oss << "<a id=\"fqa-" << section << "." << num << "\"></a>\n";
        oss << "<h2>[" << section << "." << num << "] " << title << "</h2>\n";
        return oss.str();
    }

    void replace_faq(int num) {
        // TEST:
        //std::cout << "Replace faq\n";
        if (!faq.empty()) {
            std::string repstr = "<b class=\"FAQ\"><a href=\"" + faq_site + "/" + faq_page + "#faq-" + std::to_string(section) + "." + std::to_string(num) + "\">FAQ:</a></b>";
            size_t pos = faq[0].find("FAQ:");
            if (pos != std::string::npos) {
                // TEST:
                //std::cout << "Replace\n";
                faq[0].replace(pos, 4, repstr);
            }
        }
    }

    void replace_fqa() {
        if (!fqa.empty()) {
            std::string repstr = "<b class=\"FQA\">FQA:</b>";
            size_t pos = fqa[0].find("FQA:");
            if (pos != std::string::npos) {
                fqa[0].replace(pos, 4, repstr);
            }
        }
    }
};

Question* read_question() {
    std::string title = read_paragraph();
    if (title.empty()) return nullptr;

    std::vector<std::string> faqps;
    faqps.push_back(read_paragraph());

    while (!faqps.back().empty() && faqps.back().find("FQA:") == std::string::npos) {
        faqps.push_back(read_paragraph());
    }

    std::vector<std::string> fqaps;
    fqaps.push_back(faqps.back());
    faqps.pop_back();

    while (!fqaps.back().empty() && fqaps.back().find("-END") == std::string::npos) {
        fqaps.push_back(read_paragraph());
    }

    if (!fqaps.empty()) fqaps.pop_back(); // remove the -END paragraph

    return new Question(title, faqps, fqaps);
}
///////////////////////////////////////


std::string run(const std::string& arg, const std::string& sp) {
    out.clear();

    fqa.open(arg);
    //fqa.open(arg, std::ios::binary);
    if (!fqa.is_open()) return "";

    fqa_page = arg;
    size_t pos = fqa_page.find(".fqa");
    if (pos != std::string::npos) fqa_page.replace(pos, 4, ".html");

    // helper
    // end_of_doc is now a const and initialized at startup

    // TEST:
    //std::cout << "In run\n";

    // NOTE: We were not getting the title of the singlepage.
    // This check code was returning: a 43 2b 2b
    // Which is Oa 43 2b 2b with 0a being a newline.
    // NOTE: std::cout might format small integers <10 as hex digits without the leading 0,
    // so 0x0A becomes just a.
    // The first character of the string is \n (newline, hex 0A), not 'C'.
    // Then the next characters are 'C' (0x43), '+' (0x2B), '+' (0x2B)
    // char c;
    // for(int i=0;i<4;++i) {
    //     if(fqa.get(c)) std::cout << std::hex << (unsigned int)(unsigned char)c << " ";
    // }
    // So the cause was our raw string literal was starting with a newline:
    //std::string singlepageindex = R"(
    //C++ Frequently Questioned Answers
    //{}
    


    // first line: page title
    std::getline(fqa, title);

    // TEST:
    // SHIT: In singlepage we are not getting the title. Why?
    // AHAH: Because our string literal R started with a newline
    //std::cout << "Title: " << title << "\n";

    // second line: attributes
    std::string fqa_line;
    std::getline(fqa, fqa_line);

    /* 
    // SHIT: This code is incorrect. It was searching for "\section\": instead of 'section':
    // And it also fails at section: 666 instead of section:666
    // Simple parsing: {'section':12,'faq-page':'assignment-operators.html'} -> C++ map
    size_t sec_pos = fqa_line.find("'section':");
    if (sec_pos != std::string::npos) {
        // SHIT: This needs 'section':666 but fails at 'section': 666
        // We could correct all .fqa's but let's find the C++ eval equivalent.
        // WELL: Most .fqa were correctly formatted without spaces so we just corrected the wrong ones.
        // We can now keep using this naive parser.
        section = std::stoi(fqa_line.substr(sec_pos + 10)); // 'section': is 10 chars
    }
    std::string faq_page_attr;
    size_t page_pos = fqa_line.find("'faq-page':'");
    if (page_pos != std::string::npos) {
        size_t start = page_pos + 12;
        size_t end = fqa_line.find("'", start);
        faq_page_attr = fqa_line.substr(start, end - start);
    }
    */

    // Simple parser above works OK but let's use some nifty regex parser
    // that can handle multiple spaces.
    auto parsed = parseLine(fqa_line);
    if (!parsed["section"].empty())
      section = std::stoi(parsed["section"]);
    std::string faq_page_attr = parsed["faq-page"];

    // TEST:
    //std::cout << "Section: " << section << "\nFAQ page: " << faq_page_attr << "\n";

    // TEST:
    //std::cout << "In run\n";

    if (!faq_page_attr.empty()) {
        print_heading(faq_page_attr);
    } else {
        print_heading("");
        while (true) {
            std::string p = read_paragraph();
            if (!p.empty()) {
                print_paragraph(p);
            } else {
                if (sp.empty()) out += end_of_doc;
                fqa.close();
                // SHIT: We return here for every page.
                // TEST:
                //std::cout << "Return\n";
                return out;
            }
        }
    }

    // TEST:
    //std::cout << "In run\n";

    // first paragraph
    std::string p = read_paragraph();
    print_paragraph(p);

    // read all questions
    std::vector<std::shared_ptr<Question>> qs;
    auto q = read_question();
    while (q) {
        qs.push_back(std::shared_ptr<Question>(q)); // wrap raw pointer
        q = read_question();
    }

    // TEST:
    //std::cout << "qs.size = " << qs.size() << "\n";

    out += "<ul>\n";
    for (size_t i = 0; i < qs.size(); ++i) out += qs[i]->toc_line(i + 1) + "\n";
    out += "</ul>\n";

    for (size_t i = 0; i < qs.size(); ++i) {
        int n = i + 1;
        out += "\n";
        out += qs[i]->title_lines(n);
        // TEST:
        //std::cout << "Calling replace_faq\n";
        qs[i]->replace_faq(n);
        for (auto& para : qs[i]->faq) print_paragraph(para);
        qs[i]->replace_fqa();
        for (auto& para : qs[i]->fqa) print_paragraph(para);
    }

    if (sp.empty()) out += end_of_doc;
    fqa.close();

    return out; // Return the output buffer
}


std::vector<std::pair<std::string, std::string>> secindex = {
    {"picture", "Big Picture Issues"},
    {"class", "Classes and objects"},
    {"ref", "References"},
    {"inline", "Inline functions"},
    {"ctors", "Constructors"},
    {"dtor", "Destructors"},
    {"assign", "Assignment operators"},
    {"operator", "Operator overloading"},
    {"friend", "Friends"},
    {"io", "Input/output via <code>&lt;iostream&gt;</code> and <code>&lt;cstdio&gt;</code>"},
    {"heap", "Freestore management"},
    {"exceptions", "Exceptions"},
    {"const", "Const correctness"},
    {"inheritance-basics", "Inheritance - basics"},
    {"inheritance-virtual", "Inheritance - <code>virtual</code> functions"},
    {"inheritance-proper", "Inheritance - proper inheritance and substitutability"},
    {"inheritance-abstract", "Inheritance - abstract base classes"},
    {"inheritance-mother", "Inheritance - what your mother never told you"},
    {"inheritance-multiple", "Inheritance - multiple and <code>virtual</code> inheritance"},
    {"mixing", "How to mix C and C++"},
    {"function", "Pointers to member functions"},
    {"templates", "Templates"}
};

// NOTE: Not R"(
//C++ Frequently Questioned Answers
//{}
// because that starts the file with a newline and then the singlepage title cannot be found.
std::string singlepageindex = R"(C++ Frequently Questioned Answers
{}
This is a single page version of [http://yosefk.com/c++fqa C++ FQA Lite]. C++ is a general-purpose programming language, not necessarily suitable for your special purpose. [6.18 FQA] stands for "frequently
questioned answers". This FQA is called
"lite" because it questions the answers found in `<a href="http://www.parashift.com/c++-faq-lite/index.html">C++ FAQ Lite</a>`.

The single page version does not include most "metadata" sections such as [http://yosefk.com/c++fqa/faq.html the FQA FAQ].

`<h2>Table of contents</h2>`

`<ul>
<li><a href="%(sp)s#fqa-defective">Defective C++</a> - a list of major language defects</li>
<li>C++ Q&amp;A, structured similarly to C++ FAQ Lite, with links to the original FAQ answers</li>
<li style="list-style-type: none;">
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
</li>
<li><a href="%(sp)s#fqa-web-vs-fqa">`FQA errors`</a> found by readers</li>
</ul>`
)";

void f2h_singlepage(const std::string& outname) {
    // open output file
    std::ofstream outf(outname);
    if (!outf.is_open()) {
        std::cerr << "Cannot open file " << outname << "\n";
        return;
    }

    // print HTML header
    outf << "<!DOCTYPE html>\n<html>\n<head>\n"
         << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n"
         << "<title>C++ Frequently Questioned Answers</title>\n"
         << style2 << "\n</head>\n<body>\n";

    std::string tmpfile = "sp-index.tmp.fqa";

    // replace "%(sp)s" with outname in singlepageindex
    std::string content = singlepageindex;
    std::string placeholder = "%(sp)s";
    size_t pos = 0;
    while ((pos = content.find(placeholder, pos)) != std::string::npos) {
        content.replace(pos, placeholder.length(), outname);
        pos += outname.length();
    }

    // write to temporary file
    {
        std::ofstream tf(tmpfile);
        tf << content;
    } // file is automatically closed here

    // TEST:
    std::cout << "In singlepage.\n";
    {
        //std::ifstream tmpin(tmpfile);
        run_to(tmpfile, outf, outname);
    }
    std::remove(tmpfile.c_str());

    // define helper functions
    auto sec_ancor = [&outf](const std::string& secfname) {
        outf << "<a id=\"fqa-" << secfname.substr(0, secfname.size() - 4) << "\"></a>";
    };

    auto sec_with_toc = [&outf, &outname, &sec_ancor](const std::string& filename, const std::string& name) {
        sec_ancor(filename);
        std::string tmpfile = "sec-with-toc.tmp.html";
        {
            std::ofstream tmp(tmpfile);
            run_to(filename, tmp, outname);
        }
        h2toc_gentoc(tmpfile, name, outname);
        {
            std::ifstream tmp2(tmpfile);
            outf << tmp2.rdbuf();
        }
        std::remove(tmpfile.c_str());
    };

    // now build the content
    sec_with_toc("defective.fqa", "defect");

    for (auto& pair : secindex) {
        std::string sec = pair.first;
        std::string title = pair.second;
        sec_ancor(sec + ".fqa");
        run_to(sec + ".fqa", outf, outname);
    }

    sec_with_toc("web-vs-fqa.fqa", "correction");

    outf << end_of_doc;
    outf.close();
}




// -------------------- Main --------------------
// int main(int argc, char* argv[]) {
//     try {
//         if (argc != 2) {
//             throw std::runtime_error(std::string("usage: ") + argv[0] + " <input C++ FQA text>");
//         }
//         ///////////
//         run(argv[1]);
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//         return 1;
//     }
//     return 0;
// }
