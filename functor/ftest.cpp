#pragma GCC optimize("O3")

#include <stdio.h>
#include "functor.h"

class CallBack {
public:
    FUNCTOR_TYPEDEF(callback_fn_t, void);
    CallBack(callback_fn_t _fn) : fn(_fn) {};
private:
    callback_fn_t fn;
};

class ftest {
public:
    CallBack c{FUNCTOR_BIND_MEMBER(&ftest::my_callback, void)};

    void my_callback(void);
};

int main(int argc, const char *argv[])
{
    ftest a;

    return 0;
}
