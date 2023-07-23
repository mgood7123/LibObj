#include <gtest/gtest.h>

#include <libobj.h>

using namespace LibObj;

#define LOG(NAME) std::cout << "[LOG] " << #NAME << " = " << NAME << std::endl

#define CREATE(NAME, TYPE)                                                     \
    std::cout << "[LOG] creating " #NAME " of type " #TYPE "\n";               \
    auto NAME = Obj::Create<TYPE>();                                           \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE created " #NAME " of type "                       \
              << NAME->getObjId().name() << "\n\n"
#define CREATE1(NAME, TYPE, ARG1)                                              \
    std::cout << "[LOG] creating " #NAME " of type " #TYPE "\n";               \
    auto NAME = Obj::Create<TYPE>(ARG1);                                       \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE created " #NAME " of type "                       \
              << NAME->getObjId().name() << "\n\n"
#define CLONE(NAME, FROM)                                                      \
    std::cout << "[LOG] cloning " #NAME " from " #FROM " of type "             \
              << FROM->getObjId().name() << "\n";                              \
    auto NAME = FROM->clone();                                                 \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE cloned " #NAME " from " #FROM " of type "         \
              << FROM->getObjId().name() << "\n\n"
#define CREATE_FROM(NAME, TYPE, FROM)                                          \
    std::cout << "[LOG] creating " #NAME " of type " #TYPE " from " #FROM      \
                 " of type "                                                   \
              << FROM->getObjId().name() << "\n";                              \
    auto NAME = Obj::Create<TYPE>();                                           \
    try {                                                                      \
        NAME->from(*FROM);                                                      \
    } catch (std::exception & e) {                                             \
        std::cout << "CAUGHT EXCEPTION: " << e.what() << "\n";                 \
    }                                                                          \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE created " #NAME " of type " #TYPE " from " #FROM  \
                 " of type "                                                   \
              << FROM->getObjId().name() << "\n\n"

TEST(libobj, basic) {
    int * a_ = new int {1};
    const int * b_ = new const int {2};
    {
        {
            CREATE(o1, Obj);
            CLONE(o2, o1);
            CREATE_FROM(o3, Obj, o1);
            CREATE_FROM(o4, Obj, o2);
            CREATE1(o5, Obj_Example<int>, a_);
            CLONE(o6, o5);
            CREATE_FROM(o7, Obj_Example<int>, o5);
            CREATE_FROM(o8, Obj_Example<int>, o6);
            CREATE_FROM(o9, Obj_Example<int>, o7);
            CREATE_FROM(o10, Obj_Example<float>, o5);
            CREATE_FROM(o11, Obj_Example<float>, o6);
            CLONE(o12, o7);
            CREATE_FROM(o13, Obj_Example<std::string>, o7);
            CLONE(o14, o13);

            CREATE1(o15, Obj_Example2<int>, a_);
            CLONE(o16, o15);
            CREATE1(non_const_o, Obj_Example2<int>, a_);
            CREATE1(const_o, Obj_Example2<const int>, b_);
            CREATE_FROM(non_const_o__to__const_o, Obj_Example2<const int>,
                        non_const_o);
            CREATE_FROM(const_o__to__non_const_o, Obj_Example2<int>, const_o);
        }
    }
    delete a_;
    delete b_;
}
