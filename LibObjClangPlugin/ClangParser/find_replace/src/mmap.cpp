#include <mmap.h>

void MMapHelper::error(std::exception const& e) {
    if (strstr(e.what(), "size of zero invalid") != nullptr) {
        zero_size = true;
        open = true;
        return;
    }
    std::cerr << "failed to open file:" << std::endl;
    std::cerr << "\t" << e.what() << std::endl;
    open = false;
}

MMapHelper::MMapHelper() {}

MMapHelper::MMapHelper(const char * path, char mode) {
    try {
        const char m[3] = {mode, 'e', '\0'};
        allocated_file = std::shared_ptr<MMapHelper::Map>(mmaptwo::open(path, m, 0, 0), [](auto p) { delete static_cast<MMapHelper::Map*>(p); });
        open = true;
    } catch (std::exception const& e) {
        error(e);
    }
}

MMapHelper::MMapHelper(const unsigned char * path, char mode) {
    try {
        const char m[3] = {mode, 'e', '\0'};
        allocated_file = std::shared_ptr<MMapHelper::Map>(mmaptwo::u8open(path, m, 0, 0), [](auto p) { delete static_cast<MMapHelper::Map*>(p); });
        open = true;
    } catch (std::exception const& e) {
        error(e);
    }
}

MMapHelper::MMapHelper(const wchar_t * path, char mode) {
    try {
        const char m[3] = {mode, 'e', '\0'};
        allocated_file = std::shared_ptr<MMapHelper::Map>(mmaptwo::wopen(path, m, 0, 0), [](auto p) { delete static_cast<MMapHelper::Map*>(p); });
        open = true;
    } catch (std::exception const& e) {
        error(e);
    }
}

bool MMapHelper::operator==(const MMapHelper & other) const { return allocated_file == other.allocated_file && open == other.open && zero_size == other.zero_size; }
bool MMapHelper::operator!=(const MMapHelper & other) const { return !(*this == other); }

const char * MMapHelper::get_api() const {
    int os = mmaptwo::get_os();
    return os == mmaptwo::os_unix ? "mmap(2)" : os == mmaptwo::os_win32 ? "MapViewOfFile" : "(unknown api)";
}

size_t MMapHelper::get_page_size() const {
    return mmaptwo::get_page_size();
}

std::shared_ptr<MMapHelper::Page> MMapHelper::obtain_map(size_t offset, size_t size) const {
    return obtain_map_impl(offset, size, allocated_file);
}

bool MMapHelper::is_open() const { return open; }

size_t MMapHelper::length() const {
    if (open && !zero_size) {
        return allocated_file->length();
    }
    return 0;
}

std::shared_ptr<MMapHelper::Page> MMapHelper::obtain_map_impl(size_t offset, size_t size, std::shared_ptr<MMapHelper::Map> allocated_file) const {
    if (!open || zero_size) goto fail;
    try {
        MMapHelper::Page * page = allocated_file->acquire(size, offset);
        if (!page) {
            throw std::system_error(mmaptwo::get_errno(), std::generic_category());
        }
        // tie allocated file to page
        return std::shared_ptr<MMapHelper::Page>(page, [allocated_file](auto page) { delete static_cast<MMapHelper::Page*>(page); });
    } catch (std::exception const& e) {
        std::cerr << "failed to call api " << get_api() << " to bring file into memory:" << std::endl;
        std::cerr << "\t" << e.what() << std::endl;
    }
    fail:
    return std::shared_ptr<MMapHelper::Page>(nullptr, [](auto){});
}
