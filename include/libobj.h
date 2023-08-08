#ifndef LIBOBJ_OBJ_H
#define LIBOBJ_OBJ_H

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#if defined(__clang__)
  #if __has_feature(cxx_rtti)
    #define RTTI_ENABLED
  #endif
#elif defined(__GNUG__)
  #if defined(__GXX_RTTI)
    #define RTTI_ENABLED
  #endif
#elif defined(_MSC_VER)
  #if defined(_CPPRTTI)
    #define RTTI_ENABLED
  #endif
#endif

/*

// covariant template

#include <type_traits>
#include <functional>

#include <stdio.h>
#include <memory>

template <typename T = void>
struct A {
    virtual void x() = 0;

    std::shared_ptr<
    typename std::conditional<std::is_same<void, T>::value, A<void>, T>::type
    > get() {
        return std::make_shared<
            typename std::conditional<std::is_same<void, T>::value, A<void>, T>::type
        >();
    }

    void a() {
        puts("A");
    }
};

#define IF_A(T, IF_VOID) typename std::conditional<std::is_same<void, T>::value, IF_VOID<void>, T>::type

template <typename T = void>
struct B : public A<IF_A(T, B)> {
    virtual void x() override {}
    void a() {
        puts("B");
    }
};

template <typename T = void>
struct C : public B<IF_A(T, C)> {
    virtual void x() override {}
    void a() {
        puts("C");
    }
};

int main() {
    B().get()->a();
    C().get()->a();
    return 0;
}
*/

#define LIBOBJ_BASE_ABSTRACT(T)                                                \
    using Obj_Base::operator==; /* inherit == */                               \
    using Obj_Base::operator!=; /* inherit != */                               \
    using Obj_Base::from;       /* inherit templates */                        \
    using Obj_Base::clone_impl; /* inherit clone_impl */                       \
    using Obj_Base::clone;      /* inherit clone */                            \
    std::size_t getObjBaseSize() const override {                              \
        return sizeof(T);                                                      \
    }

#define LIBOBJ_BASE_ABSTRACT_WITH_CUSTOM_CLONE(T)                              \
    using Obj_Base::operator==; /* inherit == */                               \
    using Obj_Base::operator!=; /* inherit != */                               \
    using Obj_Base::from;       /* inherit templates */                        \
    using Obj_Base::clone_impl; /* inherit clone_impl */                       \
    using Obj_Base::clone;      /* inherit clone */                            \
    std::size_t getObjBaseSize() const override {                              \
        return sizeof(T);                                                      \
    }                                                                          \
    void clone_impl(Obj_Base * ptr) const override {                           \
        clone_impl_actual(static_cast<T *>(ptr));                              \
    }                                                                          \
    void clone_impl_actual(T * obj) const

#define LIBOBJ_BASE(T)                                                         \
    using Obj_Base::operator==; /* inherit == */                               \
    using Obj_Base::operator!=; /* inherit != */                               \
    using Obj_Base::from;       /* inherit templates */                        \
    using Obj_Base::clone_impl; /* inherit clone_impl */                       \
    std::size_t getObjBaseSize() const override {                              \
        return sizeof(T);                                                      \
    }                                                                          \
    T * baseClone() const override {                                           \
        return new T();                                                        \
    }                                                                          \
    T* clone() const override {                                         \
        T * p = static_cast<T *>(baseClone());                                 \
        auto t1 = this->getObjId();                                            \
        auto t2 = p->getObjId();                                               \
        if (t1 != t2) {                                                        \
            delete p;                                                \
            std::ostringstream o;                                              \
            o << "class " << t1.name()                                         \
              << " attempted to clone itself, but the resulting allocation "   \
                 "type is class "                                              \
              << t2.name();                                                    \
            throw std::runtime_error(o.str());                                 \
        }                                                                      \
        clone_impl(p);                                                         \
        return p; \
    }

#define LIBOBJ_BASE_WITH_CUSTOM_CLONE(T)                                       \
    using Obj_Base::operator==; /* inherit == */                               \
    using Obj_Base::operator!=; /* inherit != */                               \
    using Obj_Base::from;       /* inherit templates */                        \
    using Obj_Base::clone_impl; /* inherit clone_impl */                       \
    std::size_t getObjBaseSize() const override {                              \
        return sizeof(T);                                                      \
    }                                                                          \
    T * baseClone() const override {                                           \
        return new T();                                                        \
    }                                                                          \
    T* clone() const override {                                         \
        T * p = static_cast<T *>(baseClone());                                 \
        auto t1 = this->getObjId();                                            \
        auto t2 = p->getObjId();                                               \
        if (t1 != t2) {                                                        \
            delete p;                                                \
            std::ostringstream o;                                              \
            o << "class " << t1.name()                                         \
              << " attempted to clone itself, but the resulting allocation "   \
                 "type is class "                                              \
              << t2.name();                                                    \
            throw std::runtime_error(o.str());                                 \
        }                                                                      \
        clone_impl(p);                                                         \
        return p; \
    }                                                                          \
                                                                               \
    void clone_impl(Obj_Base * ptr) const override {                           \
        clone_impl_actual(static_cast<T *>(ptr));                              \
    }                                                                          \
    void clone_impl_actual(T * obj) const

#define LIBOBJ_OVERRIDE__FROM_COPY                                             \
    void from(const Obj_Base & other) const override

#define LIBOBJ_OVERRIDE__FROM_MOVE void from(Obj_Base && other) const override

#define LIBOBJ_OVERRIDE__EQUALS                                                \
    bool operator==(const Obj_Base & other) const override

#define LIBOBJ_OVERRIDE__STREAM                                                \
    std::ostream & toStream(std::ostream & os) const override

#define LIBOBJ_OVERRIDE__HASHCODE std::size_t hashCode() const override

#define LIB_OBJ_ERROR_STRING                                                   \
    "attempting to assign a const pointer to a non-const pointer ( result of " \
    "[ T* = const T* ] would make [ T = const T ], cannot modify read-only "   \
    "variable )"

#define LIBOBJ_POINTER_ASSIGN(other_base_class, other_, value_,                 \
                              other_is_const_func, get_value_from_other)       \
    LibObj::Obj_Base_make_overload(                                            \
        [this](const other_base_class & other, auto & v) ->                    \
        typename std::enable_if<!std::is_const<decltype(v)>::value,            \
                                void>::type {                                  \
            if (other.other_is_const_func) {                                   \
                throw std::runtime_error(                                      \
                    std::string(LIB_OBJ_ERROR_STRING)                          \
                    + ", this: " + this->getObjId().name()                     \
                    + ", other: " + other.getObjId().name()                    \
                    + ", func: " + __PRETTY_FUNCTION__);                       \
            } else {                                                           \
                v = static_cast<                                               \
                    typename std::remove_reference<decltype(v)>::type>(        \
                    const_cast<void *>(other.get_value_from_other));           \
            }                                                                  \
        },                                                                     \
        [this](const other_base_class & other, auto & v) ->                    \
        typename std::enable_if<std::is_const<decltype(v)>::value,             \
                                void>::type {                                  \
            v = static_cast<                                                   \
                const typename std::remove_reference<decltype(v)>::type>(      \
                other.get_value_from_other);                                   \
        })(other_.as<other_base_class>(), value_);

namespace LibObj {

    struct Obj_Base {
            struct Obj_Base_ID {
#ifdef RTTI_ENABLED
                    const std::type_info & id;
#else
                    const int id = 0;
#endif

                    Obj_Base_ID(const Obj_Base & base);

                    std::string name() const;

                    bool operator==(const Obj_Base_ID & other);

                    bool operator!=(const Obj_Base_ID & other);
            };

            template <typename T, class... Args>
            static std::shared_ptr<T> Create(Args &&... args) {
                static_assert(std::is_base_of<Obj_Base, T>::value,
                              "template argument T must derive from Obj_Base ( "
                              "T : public Obj )");
                return std::make_shared<T>(std::forward<Args>(args)...);
            }

            template <typename T, class... Args>
            std::shared_ptr<T> create(Args &&... args) {
                static_assert(std::is_base_of<Obj_Base, T>::value,
                              "template argument T must derive from Obj_Base ( "
                              "T : public Obj )");
                return std::make_shared<T>(std::forward<Args>(args)...);
            }

            Obj_Base_ID getObjId() const;

            virtual std::size_t getObjBaseSize() const = 0;
            virtual Obj_Base * baseClone() const = 0;
            virtual void clone_impl(Obj_Base * obj) const = 0;
            virtual Obj_Base * clone() const = 0;

            template <typename U, typename std::enable_if<
                                      std::is_base_of<Obj_Base, U>::value,
                                      bool>::type = true>
            void from(const std::shared_ptr<U> & other) const {
                from(*other);
            }
            template <typename U, typename std::enable_if<
                                      std::is_base_of<Obj_Base, U>::value,
                                      bool>::type = true>
            void from(std::shared_ptr<U> && other) const {
                from(std::move(*other));
            }
            virtual void from(const Obj_Base & other) const = 0;
            virtual void from(Obj_Base && other) const = 0;
            virtual std::ostream & toStream(std::ostream & os) const;
            virtual std::size_t hashCode() const = 0;
            std::string toString() const;

            virtual bool operator==(const Obj_Base & other) const;

            template <typename T, typename std::enable_if<!std::is_pointer<T>::value,bool>::type = true>
            const T & as() const {
                static_assert(std::is_base_of<Obj_Base, T>::value,
                              "template argument T must derive from Obj_Base ( "
                              "T : public Obj )");
                return static_cast<const T &>(*this);
            }

            template <typename T, typename std::enable_if<std::is_pointer<T>::value,bool>::type = true>
            const T as() const {
                static_assert(std::is_base_of<Obj_Base, typename std::remove_pointer<T>::type>::value,
                              "template argument T must derive from Obj_Base ( "
                              "T : public Obj )");
                return static_cast<T>(this);
            }

            bool operator!=(const Obj_Base & other) const;
            Obj_Base() = default;
            Obj_Base(const Obj_Base & other) = delete;
            Obj_Base(Obj_Base && other) = delete;
            Obj_Base & operator=(const Obj_Base & other) = delete;
            Obj_Base & operator=(Obj_Base && other) = delete;
            virtual ~Obj_Base() {}

            struct HashCodeBuilder {

                    std::size_t hash = 1;

                    template <typename U,
                              typename std::enable_if<
                                  std::is_base_of<Obj_Base, U>::value,
                                  bool>::type = true>
                    HashCodeBuilder & add(const U & obj) {
                        hash = 31 * hash + obj.hashCode();
                        return *this;
                    }

                    template <typename U,
                              typename std::enable_if<
                                  !std::is_base_of<Obj_Base, U>::value,
                                  bool>::type = true>
                    HashCodeBuilder & add(const U & value) {
                        hash =
                            31 * hash
                            + std::hash<typename std::remove_const<U>::type>()(
                                value);
                        return *this;
                    }

                    template <typename T>
                    std::string hashAsHex(const T & value) {
                        return add(value).hashAsHex();
                    }

                    std::string hashAsHex();
            };
    };

    template <class F>
    struct Obj_Base_ext_fncall : private F {
            Obj_Base_ext_fncall(F v) : F(v) {}

            using F::operator();
    };

    template <class... Fs>
    struct Obj_Base_overload : public Obj_Base_ext_fncall<Fs>... {
            using Obj_Base_ext_fncall<Fs>::operator()...;
            Obj_Base_overload(Fs... vs) : Obj_Base_ext_fncall<Fs>(vs)... {}
    };

    template <class... Fs>
    Obj_Base_overload<Fs...> Obj_Base_make_overload(Fs... vs) {
        return Obj_Base_overload<Fs...> {vs...};
    }

    struct Obj : public Obj_Base {
            LIBOBJ_BASE_WITH_CUSTOM_CLONE(Obj) {
                obj->from(*this);
            }

            LIBOBJ_OVERRIDE__FROM_COPY;
            LIBOBJ_OVERRIDE__FROM_MOVE;

            LIBOBJ_OVERRIDE__HASHCODE;
    };

    std::ostream & operator<<(std::ostream & os, const Obj_Base & obj);

    template <typename T,
              typename std::enable_if<std::is_base_of<Obj_Base, T>::value,
                                      bool>::type = true>
    std::ostream & operator<<(std::ostream & os,
                              const std::shared_ptr<T> & obj) {
        return obj->toStream(os);
    }

    struct Obj_Example_Base : public Obj {
            virtual bool isConst() const = 0;
            virtual const void * getValue() const = 0;
    };

    template <typename T>
    struct Obj_Example : public Obj_Example_Base {

            LIBOBJ_BASE_WITH_CUSTOM_CLONE(Obj_Example<T>) {
                std::cout << "Obj_Example CLONE "
                          << "\n";
                obj->from(*this);
            }

        public:
            mutable T * value = nullptr;

            LIBOBJ_OVERRIDE__STREAM {
                Obj_Example_Base::toStream(os) << ", value: ";
                if (value == nullptr) {
                    return os << "nullptr";
                } else {
                    return os << *value;
                }
            }

            LIBOBJ_OVERRIDE__EQUALS {
                return value == other.as<Obj_Example_Base>().getValue();
            }

            LIBOBJ_OVERRIDE__HASHCODE {
                return HashCodeBuilder().add(value).hash;
            }

            bool isConst() const override {
                return std::is_const<T>::value;
            }
            const void * getValue() const override {
                return value;
            }

            LIBOBJ_OVERRIDE__FROM_COPY {
                std::cout << "other: " << other << "\n";

                LIBOBJ_POINTER_ASSIGN(Obj_Example_Base, other, value, isConst(),
                                      getValue())

                if (value == nullptr) {
                    std::cout << "assigned value: nullptr\n";
                } else {
                    std::cout << "assigned value: " << *value << "\n";
                }
            }

            LIBOBJ_OVERRIDE__FROM_MOVE {
                std::cout << "other: " << other << "\n";

                LIBOBJ_POINTER_ASSIGN(Obj_Example_Base, other, value, isConst(),
                                      getValue())

                if (value == nullptr) {
                    std::cout << "assigned value: nullptr\n";
                } else {
                    std::cout << "assigned value: " << *value << "\n";
                }
            }

            Obj_Example() {
                if (value == nullptr) {
                    std::cout << "constructing with assigned value: nullptr\n";
                } else {
                    std::cout << "constructing with assigned value: " << *value
                              << "\n";
                }
            }
            Obj_Example(T * value) : value(value) {
                if (value == nullptr) {
                    std::cout << "constructing with assigned value: nullptr\n";
                } else {
                    std::cout << "constructing with assigned value: " << *value
                              << "\n";
                }
            }
            ~Obj_Example() {
                if (value == nullptr) {
                    std::cout << "destructing with assigned value: nullptr\n";
                } else {
                    std::cout << "destructing with assigned value: " << *value
                              << "\n";
                }
            }
    };
    template <typename T>
    struct Obj_Example2 : public Obj_Example<T> {
            using Obj_Example<T>::Obj_Example;
            LIBOBJ_BASE(Obj_Example2<T>)
    };
} // namespace LibObj

namespace std {
    template <>
    struct hash<LibObj::Obj_Base> {
            size_t operator()(const LibObj::Obj_Base & obj) const {
                return obj.hashCode();
            }
    };

    template <>
    struct hash<const LibObj::Obj_Base> {
            size_t operator()(const LibObj::Obj_Base & obj) const {
                return obj.hashCode();
            }
    };
} // namespace std

#endif