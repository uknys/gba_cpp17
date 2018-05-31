#include "gba.h"

std::array<GBA::Interrupts::Handler, 15> IntrTable = {};

auto GBA::Interrupts::IntrMain() -> void
{
    asm(
        "mov	r3, #0x4000000      \n\t"
        "ldr	r2, [r3 ,#0x200]    \n\t"
        "ldr	r1, [r3, #0x208]    \n\t"
        "mrs	r0, spsr            \n\t"
        "stmfd	sp!, {r0-r1,r3,lr}  \n\t"
        "and	r1, r2,	r2, lsr #16 \n\t"
        "ldrh	r2, [r3, #-8]       \n\t"
        "orr	r2, r2, r1          \n\t"
        "strh	r2, [r3, #-8]       \n\t"
        "ldr	r2,=IntrTable       \n\t"
        "add	r3,r3,#0x200        \n\t"
        :
        :
        :);

findIRQ:
    asm goto(
        "ldr	r0, [r2, #4]    \n\t"
        "cmp	r0,#0           \n\t"
        "beq	%l0             \n\t"
        "ands	r0, r0, r1      \n\t"
        "bne	%l1             \n\t"
        "add	r2, r2, #8      \n\t"
        "b	    %l2             \n\t"
        :
        :
        :
        : no_handler, jump_intr, findIRQ);

no_handler:
    asm(
        "strh	r1, [r3, #0x02]     \n\t"
        "ldmfd	sp!, {r0-r1,r3,lr}  \n\t"
        "str	r1, [r3, #0x208]    \n\t"
        "mov	pc,lr               \n\t"
        :
        :
        :);

jump_intr:
    asm goto(
        "ldr	r2, [r2]        \n\t"
        "cmp	r2, #0          \n\t"
        "beq	%l0             \n\t"
        "mrs	r1, cpsr        \n\t"
        "bic	r1, r1, #0xdf   \n\t"
        "orr	r1, r1, #0x1f   \n\t"
        "msr	cpsr,r1         \n\t"
        "strh	r0, [r3, #0x02] \n\t"
        "push	{lr}            \n\t"
        "adr	lr, %l1         \n\t"
        "bx	    r2              \n\t"
        :
        :
        :
        : no_handler, IntrRet);

IntrRet:
    asm(
        "pop	{lr}                \n\t"
        "mov	r3, #0x4000000      \n\t"
        "str	r3, [r3, #0x208]    \n\t"
        "mrs	r3, cpsr            \n\t"
        "bic	r3, r3, #0xdf       \n\t"
        "orr	r3, r3, #0x92       \n\t"
        "msr	cpsr, r3            \n\t"
        "ldmfd  sp!, {r0-r1,r3,lr}  \n\t"
        "str	r1, [r3, #0x208]    \n\t"
        "msr	spsr, r0            \n\t"
        "mov	pc,lr               \n\t"
        :
        :
        :);
}

auto GBA::Interrupts::init() -> void
{
    detail::memory<InterruptFunction>(0x0300'7FFC) = IntrMain;
}

auto GBA::Interrupts::set(const Interrupt type, const InterruptFunction fn) -> void
{
    for (auto &h : IntrTable)
    {
        if (h.type == type || (h.type == GBA::Interrupts::Interrupt::VBLANK && h.fn == nullptr))
        {
            h.type = type;
            h.fn = fn;
        }
    }
}

auto GBA::Interrupts::enable(const GBA::Interrupts::Interrupt type) -> void
{
    REG_IME[0] = false;

    switch (type)
    {
    case Interrupt::VBLANK:
        Video::DISPSTAT.VBlankIRQ = true;
        break;
    case Interrupt::HBLANK:
        Video::DISPSTAT.HBlankIRQ = true;
        break;
    case Interrupt::VCOUNT:
        Video::DISPSTAT.VCounterIRQ = true;
        break;
    default:
        break;
    }

    REG_IE[static_cast<u32>(type)] = true;
    REG_IME[0] = true;
}

auto GBA::Interrupts::disable(const GBA::Interrupts::Interrupt type) -> void
{
    REG_IME[0] = false;

    switch (type)
    {
    case Interrupt::VBLANK:
        Video::DISPSTAT.VBlankIRQ = false;
        break;
    case Interrupt::HBLANK:
        Video::DISPSTAT.HBlankIRQ = false;
        break;
    case Interrupt::VCOUNT:
        Video::DISPSTAT.VCounterIRQ = false;
        break;
    default:
        break;
    }

    REG_IE[static_cast<u32>(type)] = false;
    REG_IME[0] = true;
}