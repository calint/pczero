#pragma once
// reviewed: 2024-03-09
//           2024-03-13
//           2025-05-04
//           2025-09-23

//
// osca kernel
//

#include "lib.hpp"

// sample tasks implemented in `main.cpp`
extern "C" [[noreturn]] auto tsk0() -> void;
extern "C" [[noreturn]] auto tsk1() -> void;
extern "C" [[noreturn]] auto tsk2() -> void;
extern "C" [[noreturn]] auto tsk3() -> void;
extern "C" [[noreturn]] auto tsk4() -> void;

namespace osca {

// called by the interrupt handler for events other than keyboard or timer
extern "C" auto osca_on_exception() -> void {
    static u32 stack_0;
    static u32 stack_1;
    static u32 stack_2;
    static u32 stack_3;
    static u32 stack_4;
    static u32 stack_5;
    static u32 stack_6;
    static u32 stack_7;

    osca_interrupts_disable();

    asm("mov   (%%esp),%0" : "=r"(stack_0));
    asm("mov  4(%%esp),%0" : "=r"(stack_1));
    asm("mov  8(%%esp),%0" : "=r"(stack_2));
    asm("mov 12(%%esp),%0" : "=r"(stack_3));
    asm("mov 16(%%esp),%0" : "=r"(stack_4));
    asm("mov 20(%%esp),%0" : "=r"(stack_5));
    asm("mov 24(%%esp),%0" : "=r"(stack_6));
    asm("mov 28(%%esp),%0" : "=r"(stack_7));

    err.p_hex_32b(stack_0);
    err.spc().p_hex_32b(stack_1);
    err.spc().p_hex_32b(stack_2);
    err.spc().p_hex_32b(stack_3).nl();
    err.p_hex_32b(stack_4);
    err.spc().p_hex_32b(stack_5);
    err.spc().p_hex_32b(stack_6);
    err.spc().p_hex_32b(stack_7);

    osca_hang();
}

// note: The FSAVE instruction saves a 108-byte data structure to memory
//       (fpu_state), with the first byte of the field needed to be aligned to a
//       16-byte boundary.
alignas(16) struct Task osca_tasks[]{
    //                 :-> 0b01 grabs keyboard focus, 0b10 running
    // eip esp eflags bits id edi esi ebp esp0 ebx edx ecx eax
    {u32(tsk4), 0xa'0000 + 320 * 176, 0, 0b11, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {u32(tsk1), 0xa'0000 + 320 * 180, 0, 0b10, 2, 0, 0, 0, 0, 0, 0, 0, 0},
    {u32(tsk0), 0xa'0000 + 320 * 184, 0, 0b11, 3, 0xde, 0xec, 0xeb, 0xe5, 0xb,
     0xd, 0xc, i32("kernel osca")},
    {u32(tsk2), 0xa'0000 + 320 * 188, 0, 0b10, 4, 0, 0, 0, 0, 0, 0, 0, 0},
    {u32(tsk3), 0xa'0000 + 320 * 192, 0, 0b10, 5, 0, 0, 0, 0, 0, 0, 1, 140},
    {u32(tsk3), 0xa'0000 + 320 * 196, 0, 0b10, 6, 0, 0, 0, 0, 0, 0, 2, 160},
    {u32(tsk3), 0xa'0000 + 320 * 200, 0, 0b10, 7, 0, 0, 0, 0, 0, 0, 4, 180},
};

const Task* const osca_tasks_end =
    osca_tasks + sizeof(osca_tasks) / sizeof(Task);

constexpr u32 heap_entries_size = 256;

class Heap final {
    struct Entry final {
        void* ptr{};
        u32 size_bytes{};
    };

    Data data_{};                // location and size of heap
    char* mem_pos_{};            // position in heap to contiguous memory
    const char* mem_end_{};      // end of heap memory (1 past last)
    Entry* ls_used_{};           // list of used memory entries
    Entry* ls_used_pos_{};       // next available slot
    const Entry* ls_used_end_{}; // end (1 past last) of used entries list
    Entry* ls_free_{};           // list of freed memory entries
    Entry* ls_free_pos_{};       // next available slot
    const Entry* ls_free_end_{}; // end (1 past last) of free entries list
    Size entries_size_{};        // maximum slots

  public:
    Heap() = default;

    Heap(const Data& data, const Size entries_size)
        : data_{data}, entries_size_{entries_size} {

        // place start of free memory
        mem_pos_ = reinterpret_cast<char*>(data.address());

        // place used entries area at top of the heap
        ls_used_ = static_cast<Entry*>(data.end()) - entries_size;

        // check that list start is within bounds
        if (reinterpret_cast<char*>(ls_used_) < mem_pos_) {
            osca_crash("Heap:1");
        }
        ls_used_pos_ = ls_used_;
        ls_used_end_ = ls_used_ + entries_size;

        // place free entries area before used entries
        ls_free_ = ls_used_ - entries_size;

        // check that list start is within bounds
        if (reinterpret_cast<char*>(ls_free_) < mem_pos_) {
            osca_crash("Heap:2");
        }
        ls_free_pos_ = ls_free_;
        ls_free_end_ = ls_free_ + entries_size;

        // place end of free heap memory to start of free entries area
        mem_end_ = reinterpret_cast<char*>(ls_free_);
    }

    inline auto data() -> const Data& { return data_; }

    // called by operator 'new'
    auto alloc(const u32 size_bytes) -> void* {
        // try to find a free slot of that size
        for (Entry* ent = ls_free_; ent < ls_free_pos_; ent++) {
            if (ent->size_bytes != size_bytes) {
                continue;
            }

            // found a matching size entry
            void* ptr = ent->ptr;

            // check that next entry is within the list
            if (ls_used_pos_ >= ls_used_end_) {
                osca_crash("Heap:3");
            }

            // move to used entries
            *ls_used_pos_ = *ent;
            ls_used_pos_++;
            ls_free_pos_--;

            // move last free entry to current position that has been allocated
            // avoiding gaps
            *ent = *ls_free_pos_;

            // debugging (can be removed)
            pz_memset(ls_free_pos_, 0x0f, sizeof(Entry));
            return ptr;
        }

        // did not find in free list, create new
        char* ptr = mem_pos_;
        mem_pos_ += size_bytes;

        // check bounds
        if (mem_pos_ > mem_end_) {
            osca_crash("Heap:4");
        }
        if (ls_used_pos_ >= ls_used_end_) {
            osca_crash("Heap:5");
        }

        // write to used list
        *ls_used_pos_ = {ptr, size_bytes};
        ls_used_pos_++;
        return ptr;
    }

    // called by operator `delete`
    auto free(void* ptr) -> void {
        // find the allocated memory in the used list
        for (Entry* ent = ls_used_; ent < ls_used_pos_; ent++) {
            if (ent->ptr != ptr) {
                continue;
            }

            // found the allocation entry
            // copy entry from used to free

            // check bounds
            if (ls_free_pos_ >= ls_free_end_) {
                osca_crash("Heap:6");
            }

            // add entry to free slots
            *ls_free_pos_ = *ent;
            ls_free_pos_++;

            // copy last entry of used list to this entry location avoiding gaps
            ls_used_pos_--;
            const u32 size = ent->size_bytes;
            *ent = *ls_used_pos_;

            // debugging (can be removed)
            pz_memset(ptr, 0x0f, SizeBytes(size));
            pz_memset(ls_used_pos_, 0x0f, sizeof(Entry));

            return;
        }

        // did not find the allocated memory. probably a double delete
        osca_crash("Heap:7");
    }

    auto clear(const u8 b = 0) -> void { data_.clear(b); }

    auto clear_heap_entries(const u8 free_area = 0, const u8 used_area = 0)
        -> void {
        const SizeBytes es = SizeBytes(sizeof(Entry));
        pz_memset(ls_free_, free_area, entries_size_ * es);
        pz_memset(ls_used_, used_area, entries_size_ * es);
    }
};

// initiated at `osca_init`
extern Heap heap;
Heap heap;

class Keyboard final {
    u8 buf_[2 << 3]{}; // ring buffer, minimum size 2, size power of 2, max 256
    u8 out_{};         // next event index
    u8 in_{};          // last event index +1 & roll

  public:
    // called by `osca_on_key`
    constexpr auto on_key(const u8 scan_code) -> void {
        const u8 next_in = (in_ + 1) & (sizeof(buf_) - 1);
        if (next_in == out_) { // check overrun
            // write would overwrite unhandled scan_code. display on
            // status line?
            return;
        }
        buf_[in_] = scan_code;
        in_ = next_in;
    }

    // returns keyboard scan code or 0 if no more events
    constexpr auto get_next_key() -> u8 {
        if (out_ == in_) {
            return 0; // no more events
        }
        const u8 scan_code = buf_[out_];
        out_++;
        out_ &= sizeof(buf_) - 1; // roll
        return scan_code;
    }
};

// initiated at `osca_init`
extern Keyboard keyboard;
Keyboard keyboard;

// focused task that should read keyboard
inline Task* osca_task_focused{};

// boundary address of symbol marks start of contiguous memory
// note: declared in linker script `link.ld` after code and data at first 64KB
extern "C" int free_mem_start_symbol;

// called by osca before starting tasks
// initiates globals
extern "C" auto osca_init() -> void {
    using namespace osca;

    // green dot on screen (top left)
    *reinterpret_cast<char*>(0xa'0000) = 2;

    // initiate non heap dependent globals

    vga13h = Vga13h{};

    err = PrinterToVga{};
    err.pos({1, 1}).fg(4);

    out = PrinterToVga{};
    out.pos({1, 2}).fg(2);

    keyboard = Keyboard{};

    osca_task_focused = &osca_tasks[0];

    //
    // initiate heap
    //

    // start of contiguous free memory
    const Address free_mem_start = Address(&free_mem_start_symbol);

    // usable memory before EBDA. see https://wiki.osdev.org/Memory_Map_(x86)
    // const unsigned short usable_kb_before_ebda = *reinterpret_cast<unsigned
    // short*>(0x413); size of free memory (to beginning of EBDA) const Address
    // start_of_ebda=Address(usable_kb_before_ebda<<10); debugging
    // err.p_hex_32b(unsigned(start_of_ebda));
    // size of free memory
    // const SizeBytes
    // free_mem_size=SizeBytes(start_of_ebda)-SizeBytes(free_mem_start);

    // safe version does not use unused EBDA memory
    const SizeBytes free_mem_size =
        SizeBytes(0x8'0000) - SizeBytes(free_mem_start);
    // err.p_hex_32b(unsigned(free_mem_size));

    // clear free memory
    pz_memset(free_mem_start, 0, free_mem_size);

    // initiate heap with a size of 320*100 B
    heap = Heap{{free_mem_start, 320 * 100}, heap_entries_size};

    // fill buffers with colors for debugging output
    heap.clear(0x2c);
    heap.clear_heap_entries(0x2e, 0x2f);
}

// called by osca from the keyboard interrupt
// note: there is no task switch during this function
extern "C" auto osca_on_key(const u8 scan_code) -> void {
    static bool keyboard_ctrl_pressed{};

    using namespace osca;

    // on screen
    *reinterpret_cast<u32*>(0xa0000 + 4) = scan_code;

    if (scan_code == 0x1d) {
        keyboard_ctrl_pressed = true;
    } else if (scan_code == 0x9d) {
        keyboard_ctrl_pressed = false;
    }

    // ? implement better task focus switch (same behaviour as alt+tab)
    if (keyboard_ctrl_pressed) { // ctrl+tab
        if (scan_code == 0xf) {  // tab pressed
            const Task* prev_task_focused = osca_task_focused;
            while (true) {
                osca_task_focused++;
                if (osca_task_focused == osca_tasks_end) {
                    osca_task_focused = osca_tasks;
                }
                if (osca_task_focused == prev_task_focused) {
                    return; // found no new focusable task
                }
                if (osca_task_focused->is_running() &&
                    osca_task_focused->is_grab_keyboard_focus()) {
                    // task is running and requests keyboard focus
                    return;
                }
            }
        }

        // if F1 through F12 pressed then toggle running state of task
        if (scan_code >= 0x3b && scan_code < 0x3b + 12) {
            const u8 tsk_ix = scan_code - 0x3b;
            if (sizeof(osca_tasks) / sizeof(Task) > tsk_ix) {
                osca_tasks[tsk_ix].set_running(
                    !osca_tasks[tsk_ix].is_running());
            }
            return;
        }
    }

    // to keyboard buffer
    keyboard.on_key(scan_code);
}

} // end namespace osca

// called by c++ to allocate and free memory
// note: inline not allowed by the standard
void* operator new[](unsigned size) { return osca::heap.alloc(size); }
void* operator new(unsigned size) { return osca::heap.alloc(size); }
void operator delete(void* ptr) noexcept { osca::heap.free(ptr); }
void operator delete(void* ptr, unsigned size) noexcept;
void operator delete(void* ptr, [[maybe_unused]] unsigned size) noexcept {
    osca::heap.free(ptr);
}
void operator delete[](void* ptr) noexcept { osca::heap.free(ptr); }
void operator delete[](void* ptr, unsigned size) noexcept;
void operator delete[](void* ptr, [[maybe_unused]] unsigned size) noexcept {
    osca::heap.free(ptr);
}
