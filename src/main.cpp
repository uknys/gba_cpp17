#include "gba.h"

int main()
{
    GBA::Interrupts::init();
    GBA::Interrupts::enable(GBA::Interrupts::Interrupt::VBLANK);

    while (true)
    {
        GBA::Syscall::VSyncWait();
    }
}