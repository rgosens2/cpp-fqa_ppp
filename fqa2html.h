#pragma once
#include <string>

void f2h_doit(const std::string& arg);
void f2h_singlepage(const std::string& outname);

std::string run(const std::string& arg, const std::string& sp = "");

std::string create_end_of_doc();

class Question;

