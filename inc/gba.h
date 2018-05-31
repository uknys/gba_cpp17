#pragma once
#ifndef GBA_H
#define GBA_H

#include <type_traits>
#include <cstdint>
#include <bitset>
#include <tuple>
#include <new>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using InterruptFunction = void (*)();

#define IWRAM_CODE __attribute__((section(".iwram"), target("arm")))

namespace GBA::detail
{

template <class T>
inline T &memory(const uintptr_t address)
{
    return *new (reinterpret_cast<void *>(address)) T; 
}

template <class T>
constexpr auto test_bit(const T data, const u8 bit) -> bool
{
    static_assert(std::is_integral_v<T>);
    return (data & (1 << bit)) != 0;
};

constexpr auto rgb15(const u8 r, const u8 g, const u8 b) -> u16
{
    return r | (g << 5) | (b << 10);
}
}

namespace GBA::Input
{
struct KeyInput
{
    const bool a, b, select, start, right, left, up, down, l, r;

    KeyInput(const u32 d)
        : a(!detail::test_bit(d, 0)),
          b(!detail::test_bit(d, 1)),
          select(!detail::test_bit(d, 2)),
          start(!detail::test_bit(d, 3)),
          right(!detail::test_bit(d, 4)),
          left(!detail::test_bit(d, 5)),
          up(!detail::test_bit(d, 6)),
          down(!detail::test_bit(d, 7)),
          l(!detail::test_bit(d, 8)),
          r(!detail::test_bit(d, 8)) {}

    KeyInput() : KeyInput(detail::memory<u16>(0x0400'0130)) {}

    auto X() const { return left ? -1 : right ? 1 : 0; }
    auto Y() const { return up ? -1 : down ? 1 : 0; }
    auto direction_vector() const { return std::make_pair(X(), Y()); }
};
}

namespace GBA::Video
{

struct LCDControl
{
    u8 Mode : 3;
    bool GameBoyColor : 1;
    bool DisplayFrameSelect : 1;
    bool HBlank : 1;
    bool ObjVRAMCharacterMapping : 1;
    bool ForcedBlank : 1;
    bool DisplayBG0 : 1;
    bool DisplayBG1 : 1;
    bool DisplayBG2 : 1;
    bool DisplayBG3 : 1;
    bool DisplayOBJ : 1;
    bool DisplayWin0 : 1;
    bool DisplayWin1 : 1;
    bool DisplayWinOBJ : 1;
} __attribute__((packed, aligned(4)));

struct ObjectAttributes
{
  public:
    auto x() const { return attr1 & 0x1FF; }
    auto y() const { return attr0 & 0xFF; }
    auto position() const { return std::make_pair(x(), y()); }
    auto sprite_offset() const { return attr2 & 0x03FF; }
    auto vertical_flip(const bool e) { e ? attr1 |= 0x2000 : attr1 &= 0xDFFF; }
    auto horizontal_flip(const bool e) { e ? attr1 |= 0x1000 : attr1 &= 0xEFFF; }

    auto x(const u16 x)
    {
        attr1 &= 0xFE00;
        attr1 |= (x & 0X1FF);
    }

    auto y(const u16 y)
    {
        attr0 &= 0xFF00;
        attr0 |= (y & 0xFF);
    }

    auto position(const std::pair<u16, u16> Vec2)
    {
        const auto[Px, Py] = Vec2;
        x(Px);
        y(Py);
    }

    auto sprite_offset(const u32 offset)
    {
        attr2 &= 0xFC00;
        attr2 |= offset & 0x03FF;
    }

    constexpr ObjectAttributes(const u16 a, const u16 b, const u16 c) : attr0(a), attr1(b), attr2(c), pad(static_cast<u16>(0)) {}
    constexpr ObjectAttributes(): attr0(0), attr1(0), attr2(0), pad(0) {}
  private:
    u16 attr0;
    u16 attr1;
    u16 attr2;
    u16 pad;
} __attribute__((packed, aligned(4)));

struct Color
{
    constexpr Color() : _value(0) {}
    constexpr Color(u16 value) : _value(value) {}
    constexpr Color(u8 r, u8 g, u8 b) : _value(detail::rgb15(r, g, b)) {}

  private:
    u16 _value;
};

struct LCDStatus
{
    bool VBlankFlag : 1;
    bool HBlankFlag : 1;
    bool VCounterFlag : 1;
    bool VBlankIRQ : 1;
    bool HBlankIRQ : 1;
    bool VCounterIRQ : 1;

  private:
    u8 _pad : 2;

  public:
    u16 LYC : 8;
} __attribute__((packed, aligned(4)));

enum class BGSize : u8
{
    Regular32x32 = 0,
    Regular64x32 = 1,
    Regular32x64 = 2,
    Regular64x64 = 3
};

struct BackgroundContent
{
  public:
    u8 Priority : 2;
    u8 CharacterBaseBlock : 2;

  private:
    u8 _pad : 2;

  public:
    bool Mosaic : 1;
    bool ColorMode : 1;
    u8 ScreenBaseBlock : 5;
    bool Wrap : 1;

  private:
    u8 BackgroundSize : 2;

  public:
    auto background_size() const { return static_cast<BGSize>(BackgroundSize); }
    auto background_size(const BGSize b) { BackgroundSize = static_cast<u8>(b); }
} __attribute__((packed, aligned(4)));

static auto &DISPCNT = detail::memory<Video::LCDControl>(0x0400'0000);
static auto &OAM = detail::memory<std::array<Video::ObjectAttributes, 15>>(0x0700'0000);
static auto &BG0_TILES = detail::memory<std::array<u32, 1024>>(0x0600'0000);
static auto &BG1_TILES = detail::memory<std::array<u32, 1024>>(0x0600'4000);
static auto &OBJ_TILES = detail::memory<std::array<u32, 1024>>(0x0601'0000);
static auto &OBJ_PAL = detail::memory<std::array<Video::Color, 256>>(0x0500'0200);
static auto &BG_PAL = detail::memory<std::array<Video::Color, 256>>(0x0500'0000);
static auto &DISPSTAT = detail::memory<Video::LCDStatus>(0x0400'0004);

static auto &BG0CNT = detail::memory<Video::BackgroundContent>(0x0400'0008 + 2 * 0);
static auto &BG1CNT = detail::memory<Video::BackgroundContent>(0x0400'0008 + 2 * 1);
static auto &BG2CNT = detail::memory<Video::BackgroundContent>(0x0400'0008 + 2 * 2);
static auto &BG3CNT = detail::memory<Video::BackgroundContent>(0x0400'0008 + 2 * 3);

static auto& BG0HOFFSET = detail::memory<uint16_t>(0x0400'0010 + 4 * 0);
static auto& BG1HOFFSET = detail::memory<uint16_t>(0x0400'0010 + 4 * 1);
static auto& BG2HOFFSET = detail::memory<uint16_t>(0x0400'0010 + 4 * 2);
static auto& BG3HOFFSET = detail::memory<uint16_t>(0x0400'0010 + 4 * 3);
static auto& BG0VOFFSET = detail::memory<uint16_t>(0x0400'0012 + 4 * 0);
static auto& BG1VOFFSET = detail::memory<uint16_t>(0x0400'0012 + 4 * 1);
static auto& BG2VOFFSET = detail::memory<uint16_t>(0x0400'0012 + 4 * 2);
static auto& BG3VOFFSET = detail::memory<uint16_t>(0x0400'0012 + 4 * 3);
}

namespace GBA::Syscall
{

inline void IntrWait([[maybe_unused]] u32 ReturnFlag, [[maybe_unused]] u32 IntFlag)
{
    asm volatile("swi %0"
                 :
                 : "i"(4)
                 : "r0", "r1", "r2", "r3");
}

inline void VSyncWait()
{
    asm volatile("swi %0"
                 :
                 : "i"(5)
                 : "r0", "r1", "r2", "r3");
}
}

namespace GBA::Interrupts
{

static auto &REG_IME = detail::memory<std::bitset<16>>(0x0400'0208);
static auto &REG_IE = detail::memory<std::bitset<16>>(0x0400'0200);

enum class Interrupt : u32
{
    VBLANK,
    HBLANK,
    VCOUNT,
    TIMER0,
    TIMER1,
    TIMER2,
    TIMER3,
    SERIAL,
    DMA0,
    DMA1,
    DMA2,
    DMA3,
    KEYPAD,
    CARTRIDGE,
};

struct Handler
{
    Interrupt type;
    InterruptFunction fn = nullptr;
};

IWRAM_CODE void IntrMain();
auto init() -> void;
auto set(const Interrupt type, const InterruptFunction fn) -> void;
auto enable(const Interrupt type) -> void;
auto disable(const Interrupt type) -> void;
}

#endif // GBA_H