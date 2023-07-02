#include <libobj.h>

#if defined(__clang__)
    #include <cxxabi.h>
#elif defined(__GNUC__)
    #include <cxxabi.h>
#elif defined(_MSC_VER)
#else
    #error Unsupported compiler
#endif

namespace LibObj {
    const std::string demangle(const std::type_info & ti) {
#if defined(__clang__)
        int status = -1;
        char * realname = nullptr;
        realname = abi::__cxa_demangle(ti.name(), NULL, NULL, &status);
        std::string r = std::string(realname);
        std::free(realname);
        return r;
#elif defined(__GNUC__)
        int status = -1;
        char * realname = nullptr;
        realname = abi::__cxa_demangle(ti.name(), NULL, NULL, &status);
        std::string r = std::string(realname);
        std::free(realname);
        return r;
#elif defined(_MSC_VER)
        std::string r = std::string(it.name());
        return r;
#else
    #error Unsupported compiler
#endif
    }

    std::ostream & operator<<(std::ostream & os, const Obj_Base & obj) {
        return obj.toStream(os);
    }

    bool Obj_Base::operator==(const Obj_Base & other) const {
        return hashCode() == other.hashCode();
    }

    bool Obj_Base::operator!=(const Obj_Base & other) const {
        return !(*this == other);
    }

    std::string Obj_Base::HashCodeBuilder::hashAsHex() {
        std::ostringstream h;
        h << "0x" << std::setw(6) << std::hex << hash;
        return h.str();
    }

    void Obj::from(const Obj & other) const {}
    void Obj::from(Obj && other) const {}

    void Obj::from(const Obj_Base & other) const {
        from(static_cast<const Obj &>(other));
    }
    void Obj::from(Obj_Base && other) const {
        from(std::move(static_cast<Obj &&>(other)));
    }

    std::size_t Obj::hashCode() const {
        return HashCodeBuilder().add(this).hash;
    }

    std::ostream & Obj_Base::toStream(std::ostream & os) const {
        return os << getObjBaseRealName() << "@"
                  << HashCodeBuilder().hashAsHex(this).substr(2);
    }

    std::string Obj_Base::toString() const {
        std::ostringstream os;
        toStream(os);
        return os.str();
    }
} // namespace LibObj