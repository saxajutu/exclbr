#include <string>
#include <vector>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <OpenXLSX.hpp>

struct block
{
	int first;
	int second;
};

class source
{
public:
	std::string path;
	std::vector<std::string> functions;	
};

static void read_file(std::string &s, const std::string &path)
{
	std::ifstream t(path.c_str());

	t.seekg(0, std::ios::end);
	s.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	s.assign(std::istreambuf_iterator<char>(t), std::istreambuf_iterator<char>());
}

static bool compare(const std::string &a, const std::string &b) 
{
	return a.length() >= b.length();
}

static void walk(source& src)
{
	std::vector<block> blocks;
	std::string s;
	std::string ns;
	std::string token;
	std::string buf;
	const char *ctr, *head;
	int nested;	
	int first, second;

	/* sort functions by length */
	std::sort(src.functions.begin(), src.functions.end(), compare);

	read_file(s, src.path);
	nested = 0;
	head = s.c_str();
	ctr = head;
	first = -1;
	second = -1;	
	while (*ctr) {
		
		/* directives */
		if (*ctr == '#') {
			while (*ctr) {
				if (*ctr == '\n') {
					if (buf.length()) {
						token = buf;
						buf.clear();
					}
					goto next;
				}
				ctr++;
			}
		}

		/* single line comment */
		if ((*ctr == '/') && (*(ctr + 1) == '/')) {
			while (*ctr) {
				if (*ctr == '\n') {
					if (buf.length()) {
						token = buf;
						buf.clear();
					}
					goto next;
				}
				ctr++;
			}
		}
		
		/* block comment */
		if ((*ctr == '/') && (*(ctr + 1) == '*')) {
			ctr++;
			ctr++;
			while (*ctr) {
				if ((*ctr =='/') && (*(ctr - 1) == '*')) {
					if (buf.length()) {
						token = buf;
						buf.clear();
					}
					goto next;
				}
				ctr++;
			}
		}		

		/* string */
		if (*ctr == '\"') {
			ctr++;
			while (*ctr) {
				if ((*ctr == '\"') && (*(ctr - 1) != '\\')) {
					if (buf.length()) {
						token = buf;
						buf.clear();
					}
					goto next;	
				}
				ctr++;
			}
			
		}
		
		if (isspace(*ctr)) {
			if (buf.length()) {
				token = buf;
				buf.clear();
			}
		} else {
			if (isalnum(*ctr) || (*ctr == '_')) {
				buf += *ctr;
			} else {
				if (buf.length()) {
					token = buf;
					buf.clear();
				}
			}		
		}
	next:
		if (token.length()) {
			
			if (nested == 0) {
				/* check functions name */
				const char *tctr;
				bool ok;

				tctr = ctr;
				ok = false;
				while (*tctr) {
					if (isspace(*tctr)) {
						
					} else if (*tctr == '(') {
						ok = true;
						break;
					} else {
						break;
					}
					tctr++;
				}
				if (ok) {
					ok = false;
					for (const std::string &f : src.functions) {
						if (token.compare(f) == 0) {
							ok = true;	
							break;
						}
					}
				}
				if (ok) {
					tctr = ctr;
					while (tctr > head) {
						tctr--;
						if (tctr == head) {
							first = tctr - head;
							break;
						}	
						if (*tctr == '\n') {
							first = tctr - head + 1;
							break;
						}
					}
				}
			}

			token.clear();
		}

		if (*ctr == '{') {
			nested++;
		} else if (*ctr == '}') {
			nested--;
			if (nested == 0 && first >= 0) {
				second = ctr - head;
				
				blocks.push_back((block){first, second});

				first = -1;
				second = -1;
			}
		}

		ctr++;
	}
	
	//write to file
	if (blocks.size() > 0) {
		int start = 0;

		std::ofstream fw;
		fw.open("changed.cpp");
		for (int i = 0; i < blocks.size(); ++i) {
			const block &b = blocks[i];
			if (b.first > 0) {
				fw << std::string(head + start, b.first - start);
			} 
			fw << "\\\\LCOV_EXCL_BR_START\n";
			fw << std::string(head + b.first, b.second - b.first + 1);
			fw << "\n\\\\LCOV_EXCL_BR_STOP";
			if (head[b.second + 1] != '\n') {
				fw << "\n";
			}
			start = b.second + 1;
		}
		fw << std::string(head + start, s.length() - start);
		fw.close();
	}

		
}

int main(int argc, char **argv)
{
	/*
	OpenXLSX::XLDocument doc;
	doc.create("Spreadsheet.xlsx");
	auto wks = doc.workbook().worksheet("Sheet1");
	wks.cell("A1").value() = "Hello world XLSX";
	doc.save();
	*/

	OpenXLSX::XLDocument doc;
	doc.open("Spreadsheet.xlsx");
	auto wks = doc.workbook().worksheet("Sheet1");
	std::vector<OpenXLSX::XLCellValue> cols;
	for (auto &row : wks.rows()) {
		cols = row.values();
		for (auto it = cols.begin(); it != cols.end(); it++) {
			std::cout << *it << std::endl;
		}
	}
	doc.close();

	source test;
	test.path = argv[1];
	test.functions.push_back("GetRsiData");
	walk(test);
	return 0;
}
