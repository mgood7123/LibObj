namespace LibObj {
    struct Obj_Base {
        void x();
    };

    struct Obj : public Obj_Base {};

    template <typename T1, typename T2>
    struct Templated_Obj : public Obj {};

    struct TOI : public Templated_Obj<int, float> {
        void x() {
            init();
        }
    };
}


#error plugin test