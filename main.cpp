#include <string>
#include <vector>
#include <unordered_map>
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

static std::string full_path(const std::string &dir, const std::string &path)
{
	std::string fpath = path;
	if (fpath.c_str()[0] != '/') {
		if (dir.length()) {
			if (dir.c_str()[dir.length() - 1] == '/') {
				fpath = dir + fpath;
			} else {
				fpath = dir + "/" + fpath;
			}
		}
	}
	return fpath;
}

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
	int max_func_depth;
	bool read_namespace;
	int prev_wrap_index;	

	max_func_depth = 0;
	read_namespace = false;
	prev_wrap_index = 0;

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

		/* end of command */
		if (*ctr == ';') {
			read_namespace = false;
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
			
			if (token.compare("namespace") == 0) {
				read_namespace = true;
			}

			if (nested <= max_func_depth) {
				/* check functions name */
				const char *tctr;
				bool ok;
				bool is_checked;

				tctr = ctr;
				ok = false;
				is_checked = false;
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
							is_checked = true;	
							break;
						}
					}
				}
				if (ok) {
					ok = false;
					tctr = ctr;
					while (tctr > head) {
						tctr--;
						if (tctr == head) {
							first = tctr - head;
							ok = true;
							break;
						}	
						if (*tctr == '\n') {
							first = tctr - head + 1;
							ok = true;
							break;
						}
					}
				}
				if (ok) {
					/* search near tag existed  */
					std::string sub = std::string(head + prev_wrap_index, first - prev_wrap_index);
					size_t pos = sub.find("LCOV_EXCL_BR_START");
					if (pos != std::string::npos) {
						first = -1;
						ok = false;
					}	
				}
				if (is_checked) {
					if (ok) {
						std::cout << "process: " << token << std::endl;
					} else {
						std::cout << "ignore: " << token << std::endl;
					}
				}
			}

			token.clear();
		}

		if (*ctr == '{') {
			nested++;
			if (read_namespace) {
				max_func_depth++;
				read_namespace = false;
			}
		} else if (*ctr == '}') {
			prev_wrap_index = ctr - head;
			if (nested == max_func_depth) {
				max_func_depth--;
			}
			nested--;
			if (nested <= max_func_depth && first >= 0) {
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
		fw.open(src.path);
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

	std::string root;
	std::string xlsx_file;
	std::string sheet;
	std::string col_path;
	std::string col_func;
	std::string col_check;

	sheet = "Sheet1";

	for (int i = 1, j = 2; i < argc && j < argc; i += 2, j += 2) {
		if (strcmp(argv[i], "-xlsx") == 0) {
			xlsx_file = std::string((const char *)argv[j]);			
		} else if (strcmp(argv[i], "-root") == 0) {
			root = std::string((const char *)argv[j]);
		} else if (strcmp(argv[i], "-sheet") == 0) {
			sheet = std::string((const char *)argv[j]);
		} else if (strcmp(argv[i], "-col_path") == 0) {
			col_path = std::string((const char *)argv[j]);
		} else if (strcmp(argv[i], "-col_check") == 0) {
			col_check = std::string((const char *)argv[j]);
		} else if (strcmp(argv[i], "-col_func") == 0) {
			col_func = std::string((const char *)argv[j]);
		}
	}
	
	if (xlsx_file.length() == 0) {
		std::cerr << "xlsx file not found!" << std::endl;
		return 1;
	}
	if (col_path.length() == 0) {
		std::cerr << "col path missing!" << std::endl;
		return 1;	
	}
	if (col_check.length() == 0) {
		std::cerr << "col check missing!" << std::endl;
		return 1;
	}
	if (col_func.length() == 0) {
		std::cerr << "col func missing!" << std::endl;
		return 1;
	}	
	
	int path_index = -1;
	int func_index = -1;
	int check_index = -1;
	OpenXLSX::XLDocument doc;
	doc.open(xlsx_file);
	auto wks = doc.workbook().worksheet(sheet);
	std::vector<OpenXLSX::XLCellValue> cols;
	std::unordered_map<std::string, source> sources;
	for (auto &row : wks.rows()) {
		cols = row.values();
		
		if (path_index == -1 && func_index == -1 && check_index == -1) {
			for (int i = 0; i < cols.size(); ++i) {
				if (col_path.compare(cols[i].get<std::string>()) == 0) {
					path_index = i;
				} else if (col_func.compare(cols[i].get<std::string>()) == 0) {
					func_index = i;
				} else if (col_check.compare(cols[i].get<std::string>()) == 0) {
					check_index = i;
				}
			}
			if (path_index < 0) {
				std::cerr << "col path not found!" << std::endl;
				return 1;
			}
			if (func_index < 0) {
				std::cerr << "col func not found!" << std::endl;
				return 1;
			}
			if (check_index < 0) {
				std::cerr << "col check not found!" << std::endl;
				return 1;
			} 
		} else {
			std::string t_path = cols[path_index];
			std::string t_func = cols[func_index];
			long t_check = cols[check_index];
			if (t_path.length() == 0) continue;
			if (t_func.length() == 0) continue;
			if (t_check >= 4) continue;

			t_path = full_path(root, t_path);
		    /* fix path */
			const char *head = t_path.c_str();
			const char *end = head + t_path.length() - 1;
			while (end > head) {
				if (*end == '(') {
					t_path = std::string(head, end - head);
					break;
				}	
				end--;
			}
    		
			std::unordered_map<std::string, source>::iterator it;
			it = sources.find(t_path);
			if (it == sources.end()) {
				source t;
				t.path = t_path;
				sources[t_path] = t;
				it = sources.find(t_path);
			}
					
			it->second.functions.push_back(t_func);
		}

	}
	doc.close();

	for (auto &it : sources) {
		std::cout << "sources: " << it.first << std::endl;
		std::cout << "-------------------------------------------" << std::endl;
		walk(it.second);
		std::cout << "-------------------------------------------" << std::endl << std::endl;
	}

	//source test;
	//test.path = argv[1];
	//test.functions.push_back("GetRsiData");
	//walk(test);
	return 0;
}
