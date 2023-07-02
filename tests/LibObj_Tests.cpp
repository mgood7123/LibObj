#include <gtest/gtest.h>

#include <libobj.h>

using namespace LibObj;

#define LOG(NAME) std::cout << "[LOG] " << #NAME << " = " << NAME << std::endl

#define CREATE(NAME, TYPE)                                                     \
    std::cout << "[LOG] creating " #NAME " of type " #TYPE "\n";               \
    auto NAME = Obj::Create<TYPE>();                                           \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE created " #NAME " of type "                       \
              << NAME->getObjBaseRealName() << "\n\n"
#define CREATE1(NAME, TYPE, ARG1)                                              \
    std::cout << "[LOG] creating " #NAME " of type " #TYPE "\n";               \
    auto NAME = Obj::Create<TYPE>(ARG1);                                       \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE created " #NAME " of type "                       \
              << NAME->getObjBaseRealName() << "\n\n"
#define CLONE(NAME, FROM)                                                      \
    std::cout << "[LOG] cloning " #NAME " from " #FROM " of type "             \
              << FROM->getObjBaseRealName() << "\n";                           \
    auto NAME = FROM->clone();                                                 \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE cloned " #NAME " from " #FROM " of type "         \
              << FROM->getObjBaseRealName() << "\n\n"
#define CREATE_FROM(NAME, TYPE, FROM)                                          \
    std::cout << "[LOG] creating " #NAME " of type " #TYPE " from " #FROM      \
                 " of type "                                                   \
              << FROM->getObjBaseRealName() << "\n";                           \
    auto NAME = Obj::Create<TYPE>();                                           \
    NAME->from(FROM);                                                          \
    LOG(NAME);                                                                 \
    std::cout << "[LOG] DONE created " #NAME " of type " #TYPE " from " #FROM  \
                 " of type "                                                   \
              << FROM->getObjBaseRealName() << "\n\n"

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
        }
    }
    delete a_;
    delete b_;
}
