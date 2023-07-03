#ifndef LIBOBJ_OBJ_H
#define LIBOBJ_OBJ_H

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#define LIBOBJ_BASE(T)                                                         \
    using Obj_Base::from;       /* inherit templates */                        \
    using Obj_Base::clone_impl; /* inherit clone_impl */                       \
    std::size_t getObjBaseSize() const override {                              \
        return sizeof(T);                                                      \
    }                                                                          \
    T * baseClone() const override {                                           \
        return new T();                                                        \
    }                                                                          \
    void baseCloneDelete(Obj_Base * ptr) const override {                      \
        delete static_cast<T *>(ptr);                                          \
    }                                                                          \
    std::shared_ptr<T> clone() const {                                         \
        T * p = static_cast<T *>(baseClone());                                 \
        auto t1 = this->getObjId();                                            \
        auto t2 = p->getObjId();                                               \
        if (t1 != t2) {                                                        \
            baseCloneDelete(p);                                                \
            std::ostringstream o;                                              \
            o << "class " << t1.name()                                         \
              << " attempted to clone itself, but the resulting allocation "   \
                 "type is class "                                              \
              << t2.name();                                                    \
            throw std::runtime_error(o.str());                                 \
        }                                                                      \
        clone_impl(p);                                                         \
        return std::shared_ptr<T>(static_cast<T *>(p),                         \
                                  [](auto p) { delete static_cast<T *>(p); }); \
    }

#define LIBOBJ_BASE_WITH_CUSTOM_CLONE(T)                                       \
    using Obj_Base::from; /* inherit templates */                              \
    std::size_t getObjBaseSize() const override {                              \
        return sizeof(T);                                                      \
    }                                                                          \
    T * baseClone() const override {                                           \
        return new T();                                                        \
    }                                                                          \
    void baseCloneDelete(Obj_Base * ptr) const override {                      \
        delete static_cast<T *>(ptr);                                          \
    }                                                                          \
    std::shared_ptr<T> clone() const {                                         \
        T * p = static_cast<T *>(baseClone());                                 \
        auto t1 = this->getObjId();                                            \
        auto t2 = p->getObjId();                                               \
        if (t1 != t2) {                                                        \
            baseCloneDelete(p);                                                \
            std::ostringstream o;                                              \
            o << "class " << t1.name()                                         \
              << " attempted to clone itself, but the resulting allocation "   \
                 "type is class "                                              \
              << t2.name();                                                    \
            throw std::runtime_error(o.str());                                 \
        }                                                                      \
        clone_impl(p);                                                         \
        return std::shared_ptr<T>(static_cast<T *>(p),                         \
                                  [](auto p) { delete static_cast<T *>(p); }); \
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

namespace LibObj {

    struct Obj_Base {
            struct Obj_Base_ID {
                    const std::type_info & id;

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
            virtual void baseCloneDelete(Obj_Base * ptr) const = 0;
            virtual void clone_impl(Obj_Base * obj) const = 0;

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

            template <typename T>
            const T & as() const {
                static_assert(std::is_base_of<Obj_Base, T>::value,
                              "template argument T must derive from Obj_Base ( "
                              "T : public Obj )");
                return static_cast<const T &>(*this);
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

            template <typename U = T,
                      typename std::enable_if<std::is_const<U>::value,
                                              bool>::type = true>
            void from_(const Obj_Base & other) const {
                if (other.as<Obj_Example_Base>().isConst()) {
                    value = static_cast<const U *>(
                        other.as<Obj_Example_Base>().getValue());
                    if (value == nullptr) {
                        std::cout << "assigned value: nullptr\n";
                    } else {
                        std::cout << "assigned value: " << *value << "\n";
                    }
                } else {
                    throw std::runtime_error(
                        "dont know how to assign a non-const to a const");
                }
            }

            template <typename U = T,
                      typename std::enable_if<!std::is_const<U>::value,
                                              bool>::type = true>
            void from_(const Obj_Base & other) const {
                if (!other.as<Obj_Example_Base>().isConst()) {
                    value = static_cast<T *>(const_cast<void *>(
                        other.as<Obj_Example_Base>().getValue()));
                    if (value == nullptr) {
                        std::cout << "assigned value: nullptr\n";
                    } else {
                        std::cout << "assigned value: " << *value << "\n";
                    }
                } else {
                    throw std::runtime_error(
                        "dont know how to assign a const to a non-const");
                }
            }

            template <typename U = T,
                      typename std::enable_if<std::is_const<U>::value,
                                              bool>::type = true>
            void from_(Obj_Base && other) const {
                if (other.as<Obj_Example_Base>().isConst()) {
                    value = static_cast<const U *>(const_cast<const void *>(
                        other.as<Obj_Example_Base>().getValue()));
                    if (value == nullptr) {
                        std::cout << "assigned value: nullptr\n";
                    } else {
                        std::cout << "assigned value: " << *value << "\n";
                    }
                } else {
                    throw std::runtime_error(
                        "dont know how to assign a non-const to a const");
                }
            }

            template <typename U = T,
                      typename std::enable_if<!std::is_const<U>::value,
                                              bool>::type = true>
            void from_(Obj_Base && other) const {
                if (!other.as<Obj_Example_Base>().isConst()) {
                    value = static_cast<T *>(const_cast<void *>(
                        other.as<Obj_Example_Base>().getValue()));
                    if (value == nullptr) {
                        std::cout << "assigned value: nullptr\n";
                    } else {
                        std::cout << "assigned value: " << *value << "\n";
                    }
                } else {
                    throw std::runtime_error(
                        "dont know how to assign a const to a non-const");
                }
            }

            LIBOBJ_OVERRIDE__FROM_COPY {
                std::cout << "other: " << other << "\n";
                from_(other);
            }

            LIBOBJ_OVERRIDE__FROM_MOVE {
                std::cout << "other: " << other << "\n";
                from_(std::move(other));
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