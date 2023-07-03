# cloning

```sh
# as a non-submodule
git clone --recursive https://github.com/mgood7123/LibObj

## as a submodule
git submodule add https://github.com/mgood7123/LibObj path/to/LibObj
git submodule update --init --recursive

# building and testing
make test_debug
```

# LibObj

LibObj is a C++17 compatible library, exposing an Object interface similar to Java's `Object` class

a standard implementation of `Obj` is provided, along with a `Obj_Example` to demonstrate a `T*` referencing object

## Details

`LibObj::Obj_Base` is the base class of `LibObj::Obj`,

much like all objects extend from `Object` in Java, all subclasses of `Obj_Base` and `Obj` are ultimately a subclass of `Obj_Base`

the `as` function can be used to `static_cast` an `Obj_Base` to another `Obj_Base`

- `obj.as(Obj2)` returns `static_cast<const T &>(*this);` after a `static_assert` to make sure `T` derives from `Obj_Base`

## clone functionality

the `clone` mechanism is implemented via virtual inheritence and macros, and will return an `allocation` of the `bottom-most subclass` of the object it has been invoked upon

all subclasses of `Obj_Base` inherit `Clone` if set up correctly

the default implementation is

```cpp
// called from clone() after type checking

obj->from(*this);

// no return needed
```

the actual implementation is

```cpp
T* p = baseClone();

if (this->getObjId() != p->getObjId()) {
    // log and throw
}

clone_impl(p); // default impl : obj->from(*this);

return std::shared_ptr<T>(static_cast<T *>(p), [](auto p) { delete static_cast<T *>(p); });
```

`clone` is automatically provided via the `LIBOBJ_BASE` macro among other important functions

a custom implementation can be given via the `LIBOBJ_BASE_WITH_CUSTOM_CLONE` macro

```cpp
LIBOBJ_BASE_WITH_CUSTOM_CLONE(X) {
    // the pointer:   X * obj
    //  is available for use inside this function
    //
    // type checking gaurentees the actual type is itself X
    //   if type checking fails then an error is thrown
    //
}
```

an unsafe clone can be done via

```cpp
auto x = o->baseClone(); // must be deallocated manually

if (x->getObjId() != o->getObjId()) {
    // can do something here if types dont match
}

// attempt to clone even if types dont match, UB may happen
o->clone_impl(x);

// ...

delete x;
```

like `Java`, if a class does not implement `Clone` then its base class will be tried for `Clone`

the same is true here

```cpp
    template <typename T>
    struct Obj_Example2 : public Obj_Example<T> {
            using Obj_Example<T>::Obj_Example;
            LIBOBJ_BASE(Obj_Example2<T>)
    };
```

`Obj_Example` has a custom clone override, and `Obj_Example2` has no custom clone override

invoking `clone` on `Obj_Example2` will invoke `clone_impl` on the base class `Obj_Example` or `Obj` if `Obj_Example` did not have a custom clone override

```java
[LOG] cloning o16 from o15 of type LibObj::Obj_Example2<int>
Obj_Example2<T>CLONE
constructing with assigned value: nullptr
Obj_Example<T>CLONE_IMPL
Obj_Example CLONE 
other: LibObj::Obj_Example2<int>@1e3340f, value: 1
assigned value: 1
[LOG] o16 = LibObj::Obj_Example2<int>@1e3365f, value: 1
[LOG] DONE cloned o16 from o15 of type LibObj::Obj_Example2<int>
```

# other details

the object `name` can be obtained via `getObjId().name()`

`all copy and move constructors` are explicitly marked as `= delete` due to the fact that `constructors in C++ do not correctly participate in overload resolution`

we provide a `from` function to assign objects to other objects, in both `copy` and `move` form

`from` is the recommended way to assign objects

`clone` and `from` are similar but `semantically different`

- `from` imples the object should be `copied` or `moved` to another object, `optionally being converted from one type to another`

- `clone` imples the object should be `duplicated`, `conversion should be skipped if possible`

typically `from` would be used for `conversion(optional)/copying/moving` without `allocating` a `new object`

`clone` would be used when the making a `copy` of an object `with an unknown/abstract type`

`operator==` can be overrided via `LIBOBJ_OVERRIDE__EQUALS`

```cpp
LIBOBJ_OVERRIDE__EQUALS {
    if (getObjId() != other.getObjId()) {
        return false;
    }
    return value == static_cast<const Obj_Example<T> &>(other).value;
}
```

`toStream()` is used for printing the object to streams, use `LIBOBJ_OVERRIDE__STREAM` to override

```cpp
LIBOBJ_OVERRIDE__STREAM {
    Obj_Example_Base::toStream(os) << ", value: ";
    if (value == nullptr) {
        return os << "nullptr";
    } else {
        return os << *value;
    }
}
```

`toString()` constructs a string representation of the object via the `toStream(os)` function

`hashCode()` returns a hash of the object itself, tho implementors are encouraged to use equality instead of identity

a specialization exists for `std::hash<LibObj::Obj_Base>` and invokes the `hashCode()` function
a specialization exists for `std::hash<const LibObj::Obj_Base>` and invokes the `hashCode()` function

- `Obj` returns a hash of `this` by default

- subclasses should implement `hash` by excluding `this` and `Obj::hashCode` from the hash calculation to allow for multiple copies of the same object to produce the same hash as long as that object compairs equal via `operator==`

`HashCodeBuilder` is provided for convinient implementation of hashCode from one or more values

```cpp
LIBOBJ_OVERRIDE__HASHCODE {
    return HashCodeBuilder().add(value).hash;
}
```

```cpp
struct HashCodeBuilder {

        // add specialization for Obj_Base and all subclasses of Obj_Base

        template <typename U, typename std::enable_if<
                                std::is_base_of<Obj_Base, U>::value,
                                bool>::type = true>
        HashCodeBuilder & add(const U & value) ...

        // add specialization for everything else

        template <typename U, typename std::enable_if<
                                !std::is_base_of<Obj_Base, U>::value,
                                bool>::type = true>
        HashCodeBuilder & add(const U & value) ...

        template <typename T>
        std::string hashAsHex(const T & value) ...

        std::string hashAsHex() ...
};
```
