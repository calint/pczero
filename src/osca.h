#pragma once
// reviewed: 2024-03-09
// reviewed: 2024-03-13

namespace osca {

using i8 = char;
using u8 = unsigned char;
using i16 = short;
using u16 = unsigned short;
using i32 = int;
using u32 = unsigned;
using i64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;

struct alignas(16) Task {
    u32 eip{};
    u32 esp{};
    u32 eflags{};
    u16 bits{}; // 1: accepts keyboard focus, 2: is running
    u16 id{};
    i32 edi{};
    i32 esi{};
    i32 ebp{};
    i32 esp_unused{};
    i32 ebx{};
    i32 edx{};
    i32 ecx{};
    i32 eax{};
    // note: The FSAVE instruction saves a 108-byte data structure to memory
    // (fpu_state), with the
    //       first byte of the structure needed to be aligned on a 16-byte
    //       boundary.
    alignas(16) u8 fpu_state[108]{};
    u8 padding[4]{};

    constexpr inline auto get_id() const -> u16 { return id; }
    constexpr inline auto is_grab_keyboard_focus() const -> bool {
        return bits & 1;
    }
    constexpr inline auto is_running() const -> bool { return bits & 2; }
    constexpr inline auto set_running(const bool b) -> void {
        if (b) {
            bits |= 2;
        } else {
            bits &= u16(~2);
        }
    }
};

// tasks list implemented in kernel.h
// used from osca.S in '_main', 'osca_yield', 'isr_tmr' and 'isr_fpu'
extern "C" Task osca_tasks[];

// pointer to end of tasks (1 past last entry)
extern "C" const Task *const osca_tasks_end;

// hangs the system
inline auto osca_hang() -> void { asm("cli;hlt"); }

// rest of time slice is spent in halt
inline auto osca_halt() -> void { asm("hlt"); }

// nop instruction
inline auto osca_nop() -> void { asm("nop"); }

// disables interrupts
inline auto osca_interrupts_disable() -> void { asm("cli"); }

// enables interrupts
inline auto osca_interrupts_enable() -> void { asm("sti"); }

//
// exported from osca.S
//

// current active task
extern "C" Task *osca_task_active;

// timer ticks - 64 bits
extern "C" volatile const u64 osca_tick;

// timer ticks - lower 32 bits
extern "C" volatile const u32 osca_tick_lo;

// timer ticks - higher 32 bits
extern "C" volatile const u32 osca_tick_hi;

// seconds per timer tick
constexpr f32 osca_tick_per_sec = 1.0f / 1024; // 1024 Hz

// switches to next running task
// note: single small task yielding in a tight loop might inhibit
//       interrupts due to most time being spent in non-interruptable
//       task switching code
extern "C" auto osca_yield() -> void;

//
// callbacks from osca.S
//

// called before starting tasks
extern "C" auto osca_init() -> void;

// called from 'isr_kbd' when a key is pressed or released
extern "C" auto osca_on_key(u8 scan_code) -> void;

// called when interrupt other than keyboard or timer
extern "C" auto osca_on_exception() -> void;

} // end namespace osca
