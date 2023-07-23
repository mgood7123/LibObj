#include <cppfs/fs.h>
#include <cppfs/FileHandle.h>
#include <cppfs/FileIterator.h>

#include <sstream>
#include <fstream>

#include <memory>
#include <cstring>

#include <mmap_iterator.h>

#include <tmpfile.h>

#include <composable_list.h>

struct SearchInfo {
    std::vector<std::string> s;
    std::string r;
} search_info;

#include <regex>

void invokeMMAP(const char * path) {
    auto rep_len = search_info.r.size();
    auto search_len = search_info.s[0].size();
    bool is_searching = rep_len == 0;
    bool replace_same_length = rep_len == search_len;
    bool replace_smaller = rep_len < search_len;
    bool replace_greater = rep_len > search_len;

    MMapHelper map(path, 'r');

    if (map.is_open() && map.length() == 0) {
        std::cout << "skipping zero length file: " << path << std::endl;
        return;
    }

    MMapIterator begin(map, 0);
    MMapIterator end(map, map.length());

    if (is_searching) {
        std::match_results<MMapIterator> current, prev;

        std::regex e(search_info.s[0], std::regex::ECMAScript | std::regex::optimize);

        while(std::regex_search(begin, end, current, e)) {
            prev = current;
            auto n = current.prefix();
            if (n.length() != 0) {
                std::cout << "non match: '" << n << "'" << std::endl;
            }
            for (size_t i = 0; i < current.size(); ++i) 
                std::cout << "match " << std::to_string(i) << ": " << current[i] << std::endl;

            begin = std::next(begin, current.position() + current.length());
        }
        std::cout << "non match: '" << prev.suffix() << "'" << std::endl;
    } else {
        // https://en.cppreference.com/w/cpp/regex/regex_replace
        auto out_iter = std::ostreambuf_iterator<char>(std::cout);
        std::regex e(search_info.s[0], std::regex::ECMAScript | std::regex::optimize);
        std::regex_replace(out_iter, begin, end, e, search_info.r);
        std::cout << "\n";
    }
}

std::stringstream * toMemFile(std::istream * inHandle, unsigned int s, unsigned int buffer_length) {
    std::cout << "copying file to memory file" << std::endl;
    std::stringstream * memFile = new std::stringstream(std::ios::binary | std::ios::in | std::ios::out);
    unsigned int s_r = 0;
    unsigned int s_w = 0;

    char buffer[buffer_length];
    unsigned int s_c = s > buffer_length ? buffer_length : s;
    while (!inHandle->eof()) {
        inHandle->read(buffer, s_c);
        if (inHandle->gcount() == 0) {
            break;
        }
        s_r += inHandle->gcount();
        memFile->write(buffer, inHandle->gcount());
        memFile->flush();
        std::cout << "wrote " << inHandle->gcount() << " bytes" << std::endl;
        s_w += inHandle->gcount();
        if (s_r == s) {
            std::cout << s_r << " == " << s << std::endl;
            break;
        } else {
            unsigned int ss = s - s_r;
            s_c = ss > buffer_length ? buffer_length : ss;
        }
        if (!inHandle->eof() && (inHandle->bad() || inHandle->fail())) {
            std::cout << "failed to read " << s_c << " bytes (" << s_r << " of " << s << " bytes)" << std::endl;
            break;
        }
    }
    std::cout << "copied file to memory file" << std::endl;
    return memFile;
}

void toOutStream(std::istream * inHandle, std::ostream * outHandle, unsigned int s, unsigned int buffer_length) {
    std::cout << "copying file to out stream" << std::endl;
    unsigned int s_r = 0;
    unsigned int s_w = 0;

    char buffer[buffer_length];
    unsigned int s_c = s > buffer_length ? buffer_length : s;
    while (!inHandle->eof()) {
        inHandle->read(buffer, s_c);
        if (inHandle->gcount() == 0) {
            break;
        }
        s_r += inHandle->gcount();
        outHandle->write(buffer, inHandle->gcount());
        outHandle->flush();
        std::cout << "wrote " << inHandle->gcount() << " bytes" << std::endl;
        s_w += inHandle->gcount();
        if (s_r == s) {
            std::cout << s_r << " == " << s << std::endl;
            break;
        } else {
            unsigned int ss = s - s_r;
            s_c = ss > buffer_length ? buffer_length : ss;
        }
        if (!inHandle->eof() && (inHandle->bad() || inHandle->fail())) {
            std::cout << "failed to read " << s_c << " bytes (" << s_r << " of " << s << " bytes)" << std::endl;
            break;
        }
    }
    std::cout << "copied file to out stream" << std::endl;
}

void lsdir(const std::string & path)
{
    cppfs::FileHandle handle = cppfs::fs::open(path);

    if (!handle.exists()) {
        std::cout << "item does not exist:  " << path << std::endl;
        return;
    }

    if (handle.isDirectory())
    {
        // std::cout << "entering directory:  " << path << std::endl;
        for (cppfs::FileIterator it = handle.begin(); it != handle.end(); ++it)
        {
            lsdir(path + "/" + *it);
        }
        // std::cout << "leaving directory:  " << path << std::endl;
    } else if (handle.isFile()) {
        std::cout << "reading file:  " << path << std::endl;

        invokeMMAP(path.c_str());

        std::cout << "read file:  " << path << std::endl;

        exit(0);
    } else {
        std::cout << "unknown type:  " << path << std::endl;
    }
}

std::string unescape(const std::string& s)
{
  std::string res;
  std::string::const_iterator it = s.begin();
  while (it != s.end())
  {
    char c = *it++;
    if (c == '\\' && it != s.end())
    {
      char cc = *it++;
      switch (cc) {
      case '\\': c = '\\'; break;
      case 'n': c = '\n'; break;
      case 't': c = '\t'; break;
      case 'r': c = '\r'; break;
      case 'v': c = '\v'; break;
      default:
        res += '\\';
        res += cc;
        continue;
      }
    }
    res += c;
  }

  return res;
}

#ifdef _WIN32
#include <io.h> // _setmode()
#include <fcntl.h> // O_BINARY
#define REOPEN_STDIN_AS_BINARY() _setmode(_fileno(stdin), O_BINARY)
#else
#define REOPEN_STDIN_AS_BINARY() freopen(NULL, "rb", stdin)
#endif

#define GET_STDIN_SIZE(size_name) \
   unsigned int size_name = 0; \
   while (true) { \
      char tmp[1]; \
      unsigned int r = fread(tmp, 1, 1, stdin); \
      if (r == 0) break; \
      size_name += r; \
   }

void help() {
    puts("FindReplace dir/file/--stdin -s search_items [-r replacement]");
    puts("FindReplace dir/file/--stdin search_item [replacement]");
    puts("");
    puts("no arguments       this help text");
    puts("-h, --help         this help text");
    puts("dir/file           REQUIRED: the directory/file to search, CANNOT BE SPECIFIED WITH --stdin");
    puts("--stdin            REQUIRED: use stdin as file to search");
    puts("  FindReplace --stdin f  #  marks f as the item to search for");
    puts("  FindReplace f --stdin  #  invalid");
    puts("");
    puts("the following can be specified in any order");
    puts("long option equivilants:");
    puts("  --file");
    puts("  --dir");
    puts("  --directory");
    puts("  --search");
    puts("  --replace");
    puts("-f --stdin         REQUIRED: use stdin as file to search");
    puts("-f FILE            REQUIRED: use FILE as file to search");
    puts("-d DIR             REQUIRED: use DIR as directory to search");
    puts("-s search_items    REQUIRED: the items to search for");
    puts("-r replacement     OPTIONAL: the item to replace with");
    puts("      |");
    puts("      |  CONSTRAINTS:");
    puts("      |");
    puts("      |  -r is required if -s is given");
    puts("      |");
    puts("      |   the following are equivilant");
    puts("      |    \\");
    puts("      |     |- FindReplace ... item_A item_B -r item_C");
    puts("      |     |   \\");
    puts("      |     |    searches for item_A and item_B and replaces with item_C");
    puts("      |     |");
    puts("      |     |- FindReplace ... -s item_A item_B -r item_C");
    puts("      |         \\");
    puts("      |          searches for item_A and item_B and replaces with item_C");
    puts("      |");
    puts("      |");
    puts("      |   the following are equivilant");
    puts("      |    \\");
    puts("      |     |- FindReplace ... item_A item_B");
    puts("      |     |   \\");
    puts("      |     |    searches for item_A and replaces with item_B");
    puts("      |     |");
    puts("      |     |- FindReplace ... item_A -r item_B");
    puts("      |     |   \\");
    puts("      |     |    searches for item_A and replaces with item_B");
    puts("      |     |");
    puts("      |     |- FindReplace ... -s item_A -r item_B");
    puts("      |         \\");
    puts("      |          searches for item_A and replaces with item_B");
    puts("      |");
    puts("      |");
    puts("      |   the following are NOT equivilant");
    puts("      |    \\");
    puts("      |     |- FindReplace ... -s item_A item_B");
    puts("      |     |   \\");
    puts("      |     |    searches for item_A and item_B");
    puts("      |     |");
    puts("      |     |- FindReplace ... -s item_A -r item_B");
    puts("      |         \\");
    puts("      |          searches for item_A and replaces with item_B");
    puts("      |");
    puts("      |");
    puts("      |   the following are NOT equivilant");
    puts("      |    \\");
    puts("      |     |- FindReplace ... -s item_A item_B");
    puts("      |     |   \\");
    puts("      |     |    searches for item_A and item_B");
    puts("      |     |");
    puts("      |     |- FindReplace ... item_A -r item_B");
    puts("      |         \\");
    puts("      |          searches for item_A and replaces with item_B");
    puts("      |__________________________________________________________________");
    puts("");
    puts("");
    puts("EXAMPLES:");
    puts("");
    puts("FindReplace --stdin \"\\n\" \"\\r\\n\"");
    puts("   searches 'stdin' for the item '\\n' (unix) and replaces it with '\\r\\n' (windows)");
    puts("");
    puts("FindReplace my_file \"\\n\" \"\\r\\n\"");
    puts("   searches 'my_file' for the item '\\n' (unix) and replaces it with '\\r\\n' (windows)");
    puts("");
    puts("FindReplace my_dir \"\\n\" \"\\r\\n\"");
    puts("   searches 'my_dir' recursively for the item '\\n' (unix) and replaces it with '\\r\\n' (windows)");
    puts("");
    puts("FindReplace --stdin apple");
    puts("   searches 'stdin' for the item 'apple'");
    puts("");
    puts("FindReplace --stdin \"apple pies\" \"pie kola\"");
    puts("   searches 'stdin' for the item 'apple pies', and replaces it with 'pie kola'");
    puts("");
    puts("FindReplace --stdin -s a foo \"go to space\"");
    puts("   searches 'stdin' for the items 'a', 'foo', and 'go to space'");
    puts("");
    puts("FindReplace --stdin -s a \"foo \\n bar\" go -r Alex");
    puts("   searches 'stdin' for the items 'a', 'foo \\n bar', and 'go', and replaces all of these with 'Alex'");
    exit(1);
}

std::pair<int, std::vector<int>> find_item(int argc, const char** argv, int start_index, std::vector<const char*> items) {
    std::vector<int> results;
    int r = 0;
    for (int i = start_index; i < argc; i++) {
        for(const char * item : items) {
            if (strcmp(argv[i], item) == 0 && strlen(argv[i]) == strlen(item)) {
                printf("pushing item: %s\n", argv[i]);
                r++;
                results.push_back(i);
            } else {
                results.push_back(0);
            }
        }
    }
    return {r, results};
}

int
#ifdef _WIN32
wmain(int argc, const wchar_t *argv[])
#else
main(int argc, const char*argv[])
#endif
{
    if (argc == 1 || argc == 2) {
        help();
    }

    // argc 1 == prog
    // argc 2 == prog arg1
    // argc 3 == prog arg1 arg2
    // argc 4 == prog arg1 arg3 arg3

    puts("searching for flags");
    auto items = find_item(argc, argv, 1, {"-h", "--help", "-f", "--file", "-d", "--dir", "--directory", "-s", "--search", "-r", "--replace"});
    if (items.first == 0) {
        puts("no flags");
        // if we have no flags given, default to DIR, SEARCH, REPLACE
        // we know argc == 3 or more
        // this means   prog arg1 arg2 ...

        puts("pushing search");
        search_info.s.push_back(unescape(argv[2]));
        puts("pushed search");
        if (argc == 4) {
            puts("pushing replace");
            search_info.r = unescape(argv[3]);
            puts("pushed replace");
        }

        auto dir = argv[1];
        if (strcmp(dir, "--stdin") == 0) {

            std::cout << "using stdin as search area" << std::endl;
            std::cout << "searching for:        " << search_info.s[0] << std::endl;
            if (search_info.r.size() != 0) {
                std::cout << "replacing with:       " << search_info.r << std::endl;
            }

            REOPEN_STDIN_AS_BINARY();

            TempFile tmp_file("tmp_stdin");

            std::cout << "created temporary file: " << tmp_file.get_path() << std::endl;

            std::ofstream std__in__file (tmp_file.get_path(), std::ios::binary | std::ios::out);

            toOutStream(&std::cin, &std__in__file, -1, 4096);

            std__in__file.flush();
            std__in__file.close();

           invokeMMAP(tmp_file.get_path().c_str());
        } else {
            std::cout << "directory/file to search:  " << dir << std::endl;
            std::cout << "searching for:        " << search_info.s[0] << std::endl;
            if (search_info.r.size() != 0) {
                std::cout << "replacing with:       " << search_info.r << std::endl;
            }
            lsdir(dir);
        }
    } else {
        puts("flags found");
        // we have flags
        puts("TODO");
    }
    return 0;
}