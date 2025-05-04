#pragma once
// reviewed: 2024-03-09
//           2024-03-13
//           2025-05-04

//
// osca library
//

#include "osca.hpp"

namespace osca {

using ZString = const char*;
using Address = void*;
using Size = i32;
using SizeBytes = Size;

// forward declaration of memory copy and set functions
auto pz_memcpy(Address dst, Address src, SizeBytes n) -> void;
auto pz_memset(Address dst, u8 byte, SizeBytes n) -> void;

using OffsetBytes = Size;

// implements a span of raw data starting at an address with a size
class Data {
    Address address_{};
    SizeBytes size_{};

  public:
    inline constexpr Data(const Address a, const SizeBytes n)
        : address_{a}, size_{n} {}
    inline constexpr Data() = default;
    inline constexpr auto address() const -> Address { return address_; }
    inline constexpr auto size() const -> SizeBytes { return size_; }
    inline constexpr auto address_offset(const OffsetBytes n) const -> Address {
        return static_cast<char*>(address_) + n;
    }
    inline auto to(const Data& d) const -> void {
        pz_memcpy(d.address(), address_, size_);
    }
    inline auto to(const Data& d, const SizeBytes n) const -> void {
        pz_memcpy(d.address(), address_, n); // ? check bounds
    }
    inline auto clear(u8 byte = 0) const -> void {
        pz_memset(address_, byte, size_);
    }
    inline constexpr auto end() const -> Address {
        return static_cast<char*>(address_) + size_;
    }
};

using Real = f32;
using Angle = Real;
using AngleRad = Angle;

inline auto sin(const AngleRad radians) -> Real {
    Real v;
    asm("fsin"
        : "=t"(v) // 't': first (top of stack) floating point register
        : "0"(radians));
    return v;
}

inline auto cos(const AngleRad radians) -> Real {
    Real v;
    asm("fcos"
        : "=t"(v) // 't': first (top of stack) floating point register
        : "0"(radians));
    return v;
}

// sets sin and cos value of 'radians' in 'fsin' and 'fcos'
inline auto sin_and_cos(const AngleRad radians, Real& fsin, Real& fcos)
    -> void {
    asm("fsincos"
        : "=t"(fcos), "=u"(fsin) // 'u' : second floating point register
        : "0"(radians));
}

inline auto sqrt(const Real in) -> Real {
    Real v;
    asm("fsqrt"
        : "=t"(v) // 't': first (top of stack) floating point register
        : "0"(in));
    return v;
}

inline auto abs(const Real in) -> Real {
    Real v;
    asm("fabs"
        : "=t"(v) // 't': first (top of stack) floating point register
        : "0"(in));
    return v;
}

constexpr Real PI = Real(3.141592653589793);

using AngleDeg = Angle;

constexpr inline auto deg_to_rad(const AngleDeg deg) -> AngleRad {
    constexpr Real deg_to_rad{PI / 180};
    return deg * deg_to_rad;
}

using Scale = Real;

// implements a 2d vector
template <typename T> struct VectorT {
    T x{}, y{};
    // normalizes and returns this vector
    inline auto normalize() -> VectorT& {
        const Real len = magnitude();
        x /= len;
        y /= len;
        return *this;
    }
    // scales and returns this vector
    inline constexpr auto scale(const Scale s) -> VectorT& {
        x *= s;
        y *= s;
        return *this;
    }
    // increases and returns this vector by 'v'
    inline constexpr auto inc_by(const VectorT& v) -> VectorT {
        x += v.x;
        y += v.y;
        return *this;
    }
    // increases by 'v' scaled by 's' and returns this vector
    inline constexpr auto inc_by(const VectorT& v, const Scale s) -> VectorT& {
        x += v.x * s;
        y += v.y * s;
        return *this;
    }
    // negates and returns this vector
    inline constexpr auto negate() -> VectorT& {
        x = -x;
        y = -y;
        return *this;
    }
    // sets and returns this vector to absolute value
    inline auto absolute() -> VectorT& {
        x = abs(x);
        y = abs(y);
        return *this;
    }
    // returns dot product of this vector and 'v'
    inline constexpr auto dot(const VectorT& v) const -> T {
        return x * v.x + y * v.y;
    }
    // returns the normal of this vector
    inline constexpr auto normal() const -> VectorT { return {-y, x}; }
    // returns magnitude (length)
    inline auto magnitude() const -> T { return sqrt(x * x + y * y); }
    // returns magnitude squared
    inline constexpr auto magnitude2() const -> T { return x * x + y * y; }
    // inline constexpr auto operator<=>(const VectorT&)const=default; // ? does
    // not compile in clang++ without includes from std
};
template <typename T>
inline constexpr auto operator==(const VectorT<T>& lhs, const VectorT<T>& rhs)
    -> bool {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}
template <typename T>
inline constexpr auto operator+(const VectorT<T>& lhs, const VectorT<T>& rhs)
    -> VectorT<T> {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}
template <typename T>
inline constexpr auto operator-(const VectorT<T>& lhs, const VectorT<T>& rhs)
    -> VectorT<T> {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

using Coord = Real;               // coordinate in real space
using Vector = VectorT<Coord>;    // vector in 2d real space coordinates
using Point = Vector;             // point in 2d
using PointIx = u16;              // index into a list of points
using CoordPx = i16;              // coordinate in pixel space
using PointPx = VectorT<CoordPx>; // point in pixel space
using Count = Size;               // used in loops

// implements a 2d dimension
template <typename T> class DimensionT {
    T width_{};
    T height_{};

  public:
    inline constexpr DimensionT(const T width, const T height)
        : width_{width}, height_{height} {}
    inline constexpr DimensionT() = default;
    inline constexpr auto width() const -> const T { return width_; }
    inline constexpr auto height() const -> const T { return height_; }
};

using SizePx = i16;                     // size in pixel space
using DimensionPx = DimensionT<SizePx>; // dimension in pixel space
using Color8b = u8;                     // 8 bit index in color palette

// configuration of polygon rendering
namespace enable {
constexpr static bool draw_polygons_fill = false;
constexpr static bool draw_polygons_edges = true;
} // namespace enable

// implementation of a bitmap with given address and dimension
template <typename T> class Bitmap {
    DimensionPx dim_{};
    Data data_{};

  public:
    inline constexpr Bitmap(const Address a, const DimensionPx& d)
        : dim_{d}, data_{a, d.width() * d.height() * Size(sizeof(T))} {}
    inline constexpr Bitmap() = default;
    inline constexpr auto dim() const -> const DimensionPx& { return dim_; }
    inline constexpr auto data() const -> const Data& { return data_; }
    inline constexpr auto address_offset(const PointPx p) const -> Address {
        return data_.address_offset(p.y * dim_.width() * Size(sizeof(T)) +
                                    p.x * Size(sizeof(T))); // ? check bounds
    }
    constexpr auto to(const Bitmap& dst, const PointPx& pos) const -> void {
        T* si = static_cast<T*>(data_.address());
        T* di = static_cast<T*>(dst.data_.address());
        di += pos.y * dst.dim().width() + pos.x;
        const SizePx ln = dst.dim().width() - dim_.width();
        SizePx hi = dim_.height();
        SizePx wi = dim_.width();
        while (hi--) {
            while (wi--) {
                *di = *si;
                si++;
                di++;
            }
            di += ln;
        }
    }
    constexpr auto to_transparent(const Bitmap& dst, const PointPx& pos) const
        -> void {
        T* si = static_cast<T*>(data_.address());
        T* di = static_cast<T*>(dst.data_.address());
        di += pos.y * dst.dim().width() + pos.x;
        const SizePx ln = dst.dim().width() - dim_.width();
        SizePx hi = dim_.height();
        SizePx wi = dim_.width();
        while (hi--) {
            while (wi--) {
                T v = *si;
                if (v) {
                    *di = v;
                }
                si++;
                di++;
            }
            di += ln;
        }
    }
    constexpr auto draw_dot(const Point& pos, const T value) -> void {
        const CoordPx x = CoordPx(pos.x);
        const CoordPx y = CoordPx(pos.y);
        if (x < 0 || x >= dim_.width() || y < 0 || y >= dim_.height()) {
            return;
        }
        *static_cast<T*>(address_offset({x, y})) = value;
    }
    constexpr auto draw_bounding_circle(const Point& pos, const Scale radius)
        -> void {
        Count segments = Count(5 * radius); // ? magic number
        AngleRad th = 0;
        AngleRad dth = 2 * PI / AngleRad(segments);
        while (segments--) {
            Real fsin = 0;
            Real fcos = 0;
            sin_and_cos(th, fsin, fcos);
            const Coord x = pos.x + radius * fcos;
            const Coord y = pos.y + radius * fsin;
            draw_dot({x, y}, 1); // blue dot
            th += dth;
        }
    }
    constexpr auto draw_polygon(const Point pts[], const PointIx ix_size,
                                const PointIx ix[], const T color) -> void {
        // ? what if ix_size<4
        PointIx topy_ix = 0;
        const Point& first_point = pts[ix[0]];
        Coord topx = first_point.x;
        Coord topy = first_point.y;
        // find top
        PointIx i = 1;
        while (i < ix_size) {
            const Point& p = pts[ix[i]]; // ? use pointer
            const Coord y = p.y;
            if (y < topy) {
                topy = y;
                topx = p.x;
                topy_ix = i;
            }
            i++;
        }
        PointIx ix_lft, ix_rht;
        ix_lft = ix_rht = topy_ix;
        Coord x_lft, x_rht;
        x_lft = x_rht = topx;
        bool adv_lft = true, adv_rht = true;
        Coord dxdy_lft, dxdy_rht;
        dxdy_lft = dxdy_rht = 0;
        Coord x_nxt_lft = 0;
        Coord y_nxt_lft = topy;
        Coord x_nxt_rht = 0;
        Coord y_nxt_rht = topy;
        Coord dy_rht = 0;
        Coord dy_lft = 0;
        Coord y = topy;
        const CoordPx wi = CoordPx(dim_.width());
        const CoordPx y_scr = CoordPx(y);
        T* pline = static_cast<T*>(data_.address()) + y_scr * wi;
        const PointIx last_elem_ix = ix_size - 1;
        while (true) {
            if (adv_lft) {
                if (ix_lft == last_elem_ix) {
                    ix_lft = 0;
                } else {
                    ix_lft++;
                }
                x_nxt_lft = pts[ix[ix_lft]].x;
                y_nxt_lft = pts[ix[ix_lft]].y; // ? whatif prevy==nxty
                dy_lft = y_nxt_lft - y;
                if (dy_lft != 0) {
                    dxdy_lft = (x_nxt_lft - x_lft) / dy_lft;
                } else {
                    dxdy_lft = x_nxt_lft - x_lft;
                }
            }
            if (adv_rht) {
                if (ix_rht == 0) {
                    ix_rht = last_elem_ix;
                } else {
                    ix_rht--;
                }
                x_nxt_rht = pts[ix[ix_rht]].x;
                y_nxt_rht = pts[ix[ix_rht]].y;
                dy_rht = y_nxt_rht - y;
                if (dy_rht != 0) {
                    dxdy_rht = (x_nxt_rht - x_rht) / dy_rht;
                } else {
                    dxdy_rht = x_nxt_rht - x_rht;
                }
            }
            CoordPx scan_lines_until_next_turn = 0;
            const CoordPx yscr = CoordPx(y);
            if (y_nxt_lft > y_nxt_rht) {
                //				scan_lines_until_next_turn=static_cast<CoordPx>(y_nxt_rht-y);
                scan_lines_until_next_turn = CoordPx(y_nxt_rht) - yscr;
                adv_lft = false;
                adv_rht = true;
            } else {
                //				scan_lines_until_next_turn=static_cast<CoordPx>(y_nxt_lft-y);
                //// this generates more artifacts
                scan_lines_until_next_turn = CoordPx(y_nxt_lft) - yscr;
                adv_lft = true;
                adv_rht = false;
            }
            while (true) {
                if (scan_lines_until_next_turn <= 0) {
                    break;
                }
                T* p_lft = pline + CoordPx(x_lft);
                const T* p_rht = pline + CoordPx(x_rht);
                if (p_lft > p_rht) { // ? can happen?
                    break;
                }
                scan_lines_until_next_turn--;
                const CoordPx npx = CoordPx(p_rht - p_lft);
                if (enable::draw_polygons_fill) {
                    CoordPx n = npx; // ? npx+1?
                    T* p = p_lft;
                    while (n--) {
                        *p++ = color;
                    }
                }
                if (enable::draw_polygons_edges) {
                    *p_lft = color;
                    *(p_lft + npx) = color;
                }
                pline += wi;
                x_lft += dxdy_lft;
                x_rht += dxdy_rht;
            }
            if (ix_lft == ix_rht) { // ? render dot or line?
                break;
            }
            if (adv_lft) {
                x_lft = x_nxt_lft;
                y = y_nxt_lft;
            }
            if (adv_rht) {
                x_rht = x_nxt_rht;
                y = y_nxt_rht;
            }
        }
    }
};

using Bitmap8b = Bitmap<Color8b>;

static constexpr u32 table_hex_to_font[]{
    0b01100'10010'10010'10010'01100'00000'00, // 0
    0b00100'01100'00100'00100'01110'00000'00, // 1
    0b01100'10010'00100'01000'11110'00000'00, // 2
    0b11100'00010'11100'00010'11100'00000'00, // 3
    0b00010'00110'01010'11110'00010'00000'00, // 4
    0b11110'10000'11110'00010'11100'00000'00, // 5
    0b01100'10000'11100'10010'01100'00000'00, // 6
    0b11110'00010'00100'01000'01000'00000'00, // 7
    0b01100'10010'01100'10010'01100'00000'00, // 8
    0b01100'10010'01110'00010'01100'00000'00, // 9
    0b01100'10010'11110'10010'10010'00000'00, // A
    0b11100'10010'11100'10010'11100'00000'00, // B
    0b01110'10000'10000'10000'01110'00000'00, // C
    0b11100'10010'10010'10010'11100'00000'00, // D
    0b11110'10000'11100'10000'11110'00000'00, // E
    0b11110'10000'11100'10000'10000'00000'00, // F
};

// from https://stackoverflow.com/questions/61124564/convert-scancodes-to-ascii
static constexpr char table_scancode_to_ascii[256]{
    0,   27,   '1', '2',  '3',  '4',  '5',  '6', '7',
    '8', '9',  '0', '-',  '=',  '\b', '\t', /* <-- Tab */
    'q', 'w',  'e', 'r',  't',  'y',  'u',  'i', 'o',
    'p', '[',  ']', '\n', 0, /* <-- control key */
    'a', 's',  'd', 'f',  'g',  'h',  'j',  'k', 'l',
    ';', '\'', '`', 0,    '\\', 'z',  'x',  'c', 'v',
    'b', 'n',  'm', ',',  '.',  '/',  0,    '*', 0, /* Alt */
    ' ',                                            /* Space bar */
    0,                                              /* Caps lock */
    0,                                              /* 59 - F1 key ... > */
    0,   0,    0,   0,    0,    0,    0,    0,   0, /* < ... F10 */
    0,                                              /* 69 - Num lock*/
    0,                                              /* Scroll Lock */
    0,                                              /* Home key */
    0,                                              /* Up Arrow */
    0,                                              /* Page Up */
    '-', 0,                                         /* Left Arrow */
    0,   0,                                         /* Right Arrow */
    '+', 0,                                         /* 79 - End key*/
    0,                                              /* Down Arrow */
    0,                                              /* Page Down */
    0,                                              /* Insert Key */
    0,                                              /* Delete Key */
    0,   0,    0,   0,                              /* F11 Key */
    0,                                              /* F12 Key */
    0, /* All other keys are undefined */
};
// 0e - backspace
// 1c - return

static constexpr u32 table_ascii_to_font[]{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0b00000'00000'00000'00000'00000'00000'00, // 32: space
    0b00100'00100'00100'00000'00100'00000'00, // 33: !
    0b01010'01010'00000'00000'00000'00000'00, // 34: "
    0b01010'11110'01010'11110'01010'00000'00, // 35: #
    0b01110'10100'01100'00110'11100'00000'00, // 36: $
    0b00000'10010'00100'01010'10000'00000'00, // 37: %
    0b00110'01000'00100'10010'01101'00000'00, // 38: &
    0b00100'00100'00000'00000'00000'00000'00, // 39: '
    0b00100'01000'01000'01000'00100'00000'00, // 40: (
    0b01000'00100'00100'00100'01000'00000'00, // 41:
    0b00000'01010'00100'01010'00000'00000'00, // 42: *
    0b00000'00100'01110'00100'00000'00000'00, // 43: +
    0b00000'00000'00000'00000'00100'01000'00, // 44: ,
    0b00000'00000'01110'00000'00000'00000'00, // 45: -
    0b00000'00000'00000'00000'00100'00000'00, // 46: .
    0b00010'00010'00100'01000'01000'00000'00, // 47: /
    0b01100'10010'10110'11010'01100'00000'00, // 48: 0
    0b00100'01100'00100'00100'01110'00000'00, // 49: 1
    0b01100'10010'00100'01000'11110'00000'00, // 50: 2
    0b11100'00010'11100'00010'11100'00000'00, // 51: 3
    0b00010'00110'01010'11110'00010'00000'00, // 52: 4
    0b11110'10000'11110'00010'11100'00000'00, // 53: 5
    0b01100'10000'11100'10010'01100'00000'00, // 54: 6
    0b11110'00010'00100'01000'01000'00000'00, // 55: 7
    0b01100'10010'01100'10010'01100'00000'00, // 56: 8
    0b01100'10010'01110'00010'01100'00000'00, // 57: 9
    0b00000'00100'00000'00100'00000'00000'00, // 58: :
    0b00000'00100'00000'00100'01000'00000'00, // 59: ;
    0b00100'01000'10000'01000'00100'00000'00, // 60: <
    0b00000'01110'00000'01110'00000'00000'00, // 61: =
    0b01000'00100'00010'00100'01000'00000'00, // 62: >
    0b00100'00010'00100'00000'00100'00000'00, // 63: ?
    0b01100'10010'10110'10000'01100'00000'00, // 64: @
    0b01100'10010'11110'10010'10010'00000'00, // 65: A
    0b11100'10010'11100'10010'11100'00000'00, // 66: B
    0b01110'10000'10000'10000'01110'00000'00, // 67: C
    0b11100'10010'10010'10010'11100'00000'00, // 68: D
    0b11110'10000'11100'10000'11110'00000'00, // 69: E
    0b11110'10000'11100'10000'10000'00000'00, // 70: F
    0b01110'10000'10010'10010'01110'00000'00, // 71: G
    0b10010'10010'11110'10010'10010'00000'00, // 72: H
    0b01110'00100'00100'00100'01110'00000'00, // 73: I
    0b01110'00010'00010'00010'01100'00000'00, // 74: J
    0b10010'10010'11100'10010'10010'00000'00, // 75: K
    0b10000'10000'10000'10000'11110'00000'00, // 76: L
    0b10010'11110'11110'10010'10010'00000'00, // 77: M
    0b10010'11010'10110'10010'10010'00000'00, // 78: N
    0b01100'10010'10010'10010'01100'00000'00, // 79: O
    0b11100'10010'11100'10000'10000'00000'00, // 80: P
    0b01100'10010'10010'10110'01110'00000'00, // 81: Q
    0b11100'10010'11100'10010'10010'00000'00, // 82: R
    0b01100'10000'01100'00010'11100'00000'00, // 83: S
    0b11100'01000'01000'01000'01000'00000'00, // 84: T
    0b10010'10010'10010'10010'01100'00000'00, // 85: U
    0b10010'10010'10010'01010'00100'00000'00, // 86: V
    0b10010'10010'11110'11110'10010'00000'00, // 87: W
    0b10010'10010'01100'10010'10010'00000'00, // 88: X
    0b10010'10010'01100'00100'00100'00000'00, // 89: Y
    0b11110'00100'01000'10000'11110'00000'00, // 90: Z
    0b01100'01000'01000'01000'01100'00000'00, // 91: [
    0b01000'01000'00100'00010'00010'00000'00, // 92: backslash
    0b00110'00010'00010'00010'00110'00000'00, // 93: ]
    0b00100'01010'00000'00000'00000'00000'00, // 94: ^
    0b00000'00000'00000'00000'11110'00000'00, // 95: _
    0b00100'00010'00000'00000'00000'00000'00, // 96: `
    0b00000'01100'01110'10010'01110'00000'00, // 97: a
    0b00000'10000'11100'10010'11100'00000'00, // 98: b
    0b00000'01100'10000'10000'01100'00000'00, // 99: c
    0b00000'00010'01110'10010'01110'00000'00, // 100: d
    0b00000'01100'11110'10000'01110'00000'00, // 101: e
    0b00100'01000'11100'01000'01000'00000'00, // 102: f
    0b00000'01100'10010'10010'01110'01110'00, // 103: g
    0b00000'10000'11100'10010'10010'00000'00, // 104: h
    0b00000'00100'00000'00100'00100'00000'00, // 105: i
    0b00000'00100'00000'00100'00100'01000'00, // 106: j
    0b00000'10000'10010'11000'10010'00000'00, // 107: k
    0b00000'01000'01000'01000'00100'00000'00, // 108: l
    0b00000'00000'11100'11110'11110'00000'00, // 109: m
    0b00000'00000'11100'10010'10010'00000'00, // 110: n
    0b00000'00000'01100'10010'01100'00000'00, // 111: o
    0b00000'00000'11100'10010'11100'10000'00, // 112: p
    0b00000'00000'01100'10010'01110'00010'00, // 113: q
    0b00000'00000'00110'01000'01000'00000'00, // 114: r
    0b00000'00110'01100'00010'01100'00000'00, // 115: s
    0b01000'11100'01000'01000'00100'00000'00, // 116: t
    0b00000'00000'10010'10010'01100'00000'00, // 117: u
    0b00000'00000'10010'01010'00100'00000'00, // 118: v
    0b00000'00000'11110'11110'01100'00000'00, // 119: w
    0b00000'00000'10010'01100'10010'00000'00, // 120: x
    0b00000'00000'10010'01010'00100'01000'00, // 121: y
    0b00000'00000'11110'00100'11110'00000'00, // 122: z
    0b01100'01000'11000'01000'01100'00000'00, // 123: {
    0b00100'00100'00100'00100'00100'00000'00, // 123: |
    0b01100'00100'00110'00100'01100'00000'00, // 123: }
    0b00000'10110'01100'00000'00000'00000'00, // 124: ~
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

using CoordsChar = VectorT<i16>;

// prints to a bitmap
class PrinterToBitmap {
    Bitmap8b* bmp_{};    // destination bitmap
    Color8b* di_{};      // current pixel in bitmap
    Color8b* dil_{};     // beginning of current line in bitmap
    SizePx bmp_wi_{};    // bitmap width
    SizePx ln_{};        // offset from font line to next line
    Color8b fg_{2};      // foreground
    Color8b bg_{};       // background
    bool transparent_{}; // true if transparent
    u8 padding[1]{};     // unused
    static constexpr SizePx font_wi_{5};
    static constexpr SizePx font_hi_{6};
    static constexpr SizePx line_padding_{2}; // ? attribute
  public:
    constexpr explicit PrinterToBitmap(Bitmap8b* bmp)
        : bmp_{bmp}, di_{static_cast<Color8b*>(bmp->data().address())},
          dil_{di_}, bmp_wi_{bmp->dim().width()},
          ln_{SizePx(bmp_wi_ - font_wi_)} {}
    constexpr auto pos(const CoordsChar p) -> PrinterToBitmap& {
        // ? bounds check
        di_ = static_cast<Color8b*>(bmp_->data().address());
        di_ += bmp_wi_ * p.y * (font_hi_ + line_padding_) + p.x * font_wi_;
        dil_ = di_;
        return *this;
    }
    inline constexpr auto nl() -> PrinterToBitmap& {
        // ? bounds check
        di_ = dil_ + bmp_wi_ * (font_hi_ + line_padding_);
        dil_ = di_;
        return *this;
    }
    inline constexpr auto cr() -> PrinterToBitmap& {
        di_ = dil_;
        return *this;
    }
    inline constexpr auto fg(const Color8b c) -> PrinterToBitmap& {
        fg_ = c;
        return *this;
    }
    inline constexpr auto bg(const Color8b c) -> PrinterToBitmap& {
        bg_ = c;
        return *this;
    }
    inline constexpr auto transparent(const bool b) -> PrinterToBitmap& {
        transparent_ = b;
        return *this;
    }
    constexpr auto draw(u32 bmp_5x6) -> PrinterToBitmap& {
        if (transparent_) {
            draw_transparent(bmp_5x6);
        } else {
            draw_with_bg(bmp_5x6);
        }
        return *this;
    }
    constexpr auto p_hex(const u32 hex_number_4b) -> PrinterToBitmap& {
        draw(table_hex_to_font[hex_number_4b & 0xf]);
        return *this;
    }
    constexpr auto p_hex_8b(const u8 v) -> PrinterToBitmap& {
        const u32 ch1 = v & 0xf;
        const u32 ch2 = (v >> 4) & 0xf;
        p_hex(ch2);
        p_hex(ch1);
        return *this;
    }
    constexpr auto p_hex_16b(const u16 v) -> PrinterToBitmap& {
        // ? ugly code. remake
        const u32 ch1 = v & 0xf;
        const u32 ch2 = (v >> 4) & 0xf;
        const u32 ch3 = (v >> 8) & 0xf;
        const u32 ch4 = (v >> 12) & 0xf;
        p_hex(ch4);
        p_hex(ch3);
        p_hex(ch2);
        p_hex(ch1);
        return *this;
    }
    constexpr auto p_hex_32b(const u32 v) -> PrinterToBitmap& {
        // ? ugly code. remake
        const u32 ch1 = v & 0xf;
        const u32 ch2 = (v >> 4) & 0xf;
        const u32 ch3 = (v >> 8) & 0xf;
        const u32 ch4 = (v >> 12) & 0xf;
        const u32 ch5 = (v >> 16) & 0xf;
        const u32 ch6 = (v >> 20) & 0xf;
        const u32 ch7 = (v >> 24) & 0xf;
        const u32 ch8 = (v >> 28) & 0xf;
        p_hex(ch8);
        p_hex(ch7);
        p_hex(ch6);
        p_hex(ch5);
        p(':');
        p_hex(ch4);
        p_hex(ch3);
        p_hex(ch2);
        p_hex(ch1);
        return *this;
    }
    constexpr auto p(const char ch) -> PrinterToBitmap& {
        draw(table_ascii_to_font[u32(ch)]);
        return *this;
    }
    constexpr auto p(ZString str) -> PrinterToBitmap& {
        while (*str) {
            p(*str);
            str++;
        }
        return *this;
    }
    constexpr auto backspace() -> PrinterToBitmap& {
        // ? bounds check
        di_ -= font_wi_;
        p(' ');
        di_ -= font_wi_;
        return *this;
    }
    constexpr auto spc() -> PrinterToBitmap& {
        p(' ');
        return *this;
    }

  private:
    constexpr auto draw_with_bg(u32 bmp_5x6) -> PrinterToBitmap& {
        constexpr u32 mask = 1u << 31;
        for (SizePx y = 0; y < font_hi_; y++) {
            for (SizePx x = 0; x < font_wi_; x++) {
                const bool px = bmp_5x6 & mask;
                *di_ = px ? fg_ : bg_;
                bmp_5x6 <<= 1;
                di_++;
            }
            di_ += ln_;
        }
        di_ = di_ - bmp_wi_ * font_hi_ + font_wi_;
        return *this;
    }
    constexpr auto draw_transparent(u32 bmp_5x6) -> PrinterToBitmap& {
        constexpr u32 mask = 1u << 31;
        for (SizePx y = 0; y < font_hi_; y++) {
            for (SizePx x = 0; x < font_wi_; x++) {
                const bool px = bmp_5x6 & mask;
                if (px) {
                    *di_ = fg_;
                }
                bmp_5x6 <<= 1;
                di_++;
            }
            di_ += ln_;
        }
        di_ = di_ - bmp_wi_ * font_hi_ + font_wi_;
        return *this;
    }
};

// vga mode 13h bitmap at 0xa'0000 (320 x 200 x 8)
class Vga13h {
    Bitmap8b bmp_{};

  public:
    inline Vga13h() : bmp_{Address(0xa'0000), DimensionPx{320, 200}} {}
    inline constexpr auto bmp() -> Bitmap8b& { return bmp_; }
};

extern Vga13h vga13h;
Vga13h vga13h; // initialized by `osca_init`

class PrinterToVga : public PrinterToBitmap {
  public:
    inline constexpr PrinterToVga() : PrinterToBitmap{&vga13h.bmp()} {}
};

// print debug to vga13h
extern PrinterToVga out;
PrinterToVga out; // initialized by `osca_init`

// print error to vga13h
extern PrinterToVga err;
PrinterToVga err; // initialized by `osca_init`

// crashes by printing `msg` and stack to `err` then hangs
[[noreturn]] auto osca_crash(ZString msg) -> void;
[[noreturn]] auto osca_crash(ZString msg) -> void {
    err.p(msg).nl();
    osca_on_exception();
    while (true)
        ;
}

// implementation of a matrix handling 2d transforms
template <typename T> class MatrixT {
    T xx{}, xy{}, xt{};
    T yx{}, yy{}, yt{};
    T ux{}, uy{}, id{};

  public:
    auto set_transform(const Scale scale, const AngleRad rotation,
                       const VectorT<T>& translation) -> void {
        T fcos = 0;
        T fsin = 0;
        sin_and_cos(rotation, fsin, fcos);
        const T cs = scale * fcos;
        const T sn = scale * fsin;
        xx = cs;
        xy = -sn;
        xt = translation.x;
        yx = sn;
        yy = cs;
        yt = translation.y;
        ux = 0;
        uy = 0;
        id = 1;
    }
    constexpr auto transform(const VectorT<T> src[], VectorT<T> dst[],
                             Count n) const -> void {
        while (n--) {
            dst->x = xx * src->x + xy * src->y + xt;
            dst->y = yx * src->x + yy * src->y + yt;
            src++;
            dst++;
        }
    }
    // does the rotation part of the transform
    constexpr auto rotate(const VectorT<T> src[], VectorT<T> dst[],
                          Count n) const -> void {
        while (n--) {
            dst->x = xx * src->x + xy * src->y;
            dst->y = yx * src->x + yy * src->y;
            src++;
            dst++;
        }
    }
    inline constexpr auto axis_x() const -> VectorT<T> {
        return {xx, yx};
    } // math correct?
    inline constexpr auto axis_y() const -> VectorT<T> {
        return {xy, yy};
    } // math correct?
};

using Matrix = MatrixT<Coord>;

// implementation of an owning pointer
template <typename T> class UniquePtr {
    T* ptr_{};

  public:
    inline constexpr UniquePtr() = default;
    inline constexpr explicit UniquePtr(T* p) : ptr_{p} {}
    inline constexpr ~UniquePtr() { delete ptr_; }
    inline constexpr UniquePtr(const UniquePtr&) = delete;
    inline constexpr UniquePtr& operator=(const UniquePtr&) = delete;
    inline constexpr UniquePtr(UniquePtr&& other) : ptr_{other.ptr_} {
        other.ptr_ = nullptr;
    }
    inline constexpr auto operator=(UniquePtr&& other) -> UniquePtr& {
        if (this != &other) {
            delete ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    inline constexpr auto release() -> T* {
        T* p = ptr_;
        ptr_ = nullptr;
        return p;
    }
    inline constexpr auto get() const -> T* { return ptr_; }
    inline constexpr auto operator*() const -> T& { return *ptr_; }
    inline constexpr auto operator->() const -> T* { return ptr_; }
    inline constexpr explicit operator bool() const { return ptr_ != nullptr; }
};

// implementation of an owning pointer to an array
template <typename T> class UniquePtr<T[]> {
    T* ptr_{};

  public:
    inline constexpr UniquePtr() = default;
    inline constexpr explicit UniquePtr(T* p) : ptr_{p} {}
    inline constexpr ~UniquePtr() { delete[] ptr_; }
    inline constexpr UniquePtr(const UniquePtr&) = delete;
    inline constexpr UniquePtr& operator=(const UniquePtr&) = delete;
    inline constexpr UniquePtr(UniquePtr&& other) : ptr_{other.ptr_} {
        other.ptr_ = nullptr;
    }
    inline constexpr auto operator=(UniquePtr&& other) -> UniquePtr& {
        if (this != &other) {
            delete[] ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    inline constexpr auto release() -> T* {
        T* p = ptr_;
        ptr_ = nullptr;
        return p;
    }
    inline constexpr auto get() const -> T* { return ptr_; }
    inline constexpr auto operator[](const i32 index) const -> T& {
        return ptr_[index];
    }
    inline constexpr explicit operator bool() const { return ptr_ != nullptr; }
};

template <typename T> inline constexpr auto move(T& t) -> T&& {
    return static_cast<T&&>(t);
}

template <typename T> inline constexpr auto forward(T&& t) -> T&& {
    return static_cast<T&&>(t);
}

template <typename T, typename... Args>
inline constexpr auto make_unique(Args&&... args) -> UniquePtr<T> {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

template <typename T>
inline constexpr auto make_unique_array(const u32 size) -> UniquePtr<T[]> {
    return UniquePtr<T[]>(new T[size]{});
}

inline auto pz_memcpy(Address dst, Address src, SizeBytes n) -> void {
    // note: volatile so g++ does not optimizes it away
    asm volatile("cld;"
                 "rep movsb;"
                 : "=D"(dst), "=S"(src), "=c"(n)
                 : "0"(dst), "1"(src), "2"(n)
                 : "memory");
}

inline auto pz_memset(Address dst, u8 value, SizeBytes n) -> void {
    // note: volatile so g++ does not optimizes it away
    asm volatile("cld;"
                 "rep stosb;"
                 : "=D"(dst), "=c"(n)
                 : "0"(dst), "1"(n), "a"(value)
                 : "memory");
}

// built-in functions replacements (used by clang++ -O0 and -Os)
extern "C" void* memcpy(void* dst, void* src, int n);
extern "C" void* memset(void* dst, int value, int n);
void* memcpy(void* dst, void* src, int n) {
    pz_memcpy(Address(dst), Address(src), SizeBytes(n));
    return dst;
}
void* memset(void* dst, int value, int n) {
    pz_memset(Address(dst), u8(value), SizeBytes(n));
    return dst;
}

} // end namespace osca
