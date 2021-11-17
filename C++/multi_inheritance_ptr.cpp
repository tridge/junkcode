#include <stdio.h>
#include <stdint.h>

class CANSensor {
    private:
        int32_t aa;
};
class AP_RangeFinder_Backend {
    private:
        int32_t bb;
};

class AP_RangeFinder_Benewake_CAN1 : public CANSensor, public AP_RangeFinder_Backend {
    public:
    AP_RangeFinder_Benewake_CAN1() {
        printf("constructor1 this=%p\n", this);
    }
    private:
    int32_t x;
};


int main()
{
    auto *c1 = new AP_RangeFinder_Benewake_CAN1();
    printf("c1=%p\n", c1);
    AP_RangeFinder_Backend *rb = c1;
    printf("rb=%p", rb);
    return 0;
}

