// reviewed: 2024-03-09

#include"kernel.h"

using namespace osca;

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] auto tsk0()->void{
	osca_interrupts_disable();
	const Task*this_task=osca_task_active;
	const int32 eax=osca_task_active->eax;
//	Register ebx=osca_active_task->ebx;
//	Register ecx=osca_active_task->ecx;
//	Register edx=osca_active_task->edx;
//	Register esi=osca_active_task->esi;
//	Register edi=osca_active_task->edi;
//	Register ebp=osca_active_task->ebp;
//	Register esp0=osca_active_task->esp0;
	osca_interrupts_enable();
//	err.p_hex_32b(unsigned(eax)).spc();
//	err.p_hex_32b(unsigned(ebx)).spc();
//	err.p_hex_32b(unsigned(ecx)).spc();
//	err.p_hex_32b(unsigned(edx)).spc();
//	err.p_hex_32b(unsigned(esi)).spc();
//	err.p_hex_32b(unsigned(edi)).spc();
//	err.p_hex_32b(unsigned(ebp)).spc();
//	err.p_hex_32b(unsigned(esp0)).spc();

	const char*hello=reinterpret_cast<const char*>(eax);
	PrinterToBitmap pb{&vga13h.bmp()};

	pb.fg(5).pos({13,3});
	for(char i='0';i<='9';i++){
		pb.p(i);
	}
	pb.fg(7).pos({13+10,3});
	for(char i='A';i<='Z';i++){
		pb.p(i);
	}
	pb.fg(6).pos({13+10,4});
	for(char i='a';i<='z';i++){
		pb.p(i);
	}

	pb.fg(7).pos({13+10,5}).p("\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");

	pb.fg(0x66);
	pb.pos({50,3}).p("./\\.");
	pb.pos({50,4}).p("/--\\");

	pb.fg(0x60);
	pb.pos({54,3}).p(" ___ ");
	pb.pos({54,4}).p("|o_o|");
	pb.pos({54,5}).p("/| |\\");

	pb.fg(0x5c);
	pb.pos({59,3}).p("  ___  ");
	pb.pos({59,4}).p(" /- -\\ ");
	pb.pos({59,5}).p("/\\_-_/\\");
	pb.pos({59,6}).p("  | |");

	pb.pos({2,3}).fg(2).p(hello).nl().p('_');

	while(true){
		// handle keyboard events
		while(true){
			if(osca_task_focused!=this_task){
				break;
			}
			const uint8 sc=keyboard.get_next_key();
			if(!sc){
				break;
			}
			if(sc==0xe){ // backspace
				pb.backspace().backspace().p('_');
				continue;
			}
			if(sc==0x1c){ // return
				pb.backspace().nl().p('_');
				continue;
			}
			char ch=table_scancode_to_ascii[sc];
			if(!ch){ // not an ascii. probably key release
				continue;
			}
			if(ch>='a' && ch<='z'){
				ch&=~0x20; // to upper case
			}
			pb.backspace().p(ch).p('_');
		}
		osca_yield(); // ?! if it is only task running osca 'hangs'
		              // because no interrupts get through due to
		              // almost all time spent in non-interruptable
		              // osca_yield code
//		osca_halt(); // pauses task until next interrupt
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
asm(".global tsk1");
asm(".align 16");
asm("tsk1:");
asm("  incl 0xa0000+80");
// asm("  call osca_yield");
asm("  jmp tsk1");
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] auto tsk2()->void{
	while(true){
		// draw red line
		pz_memset(Address(0xa'0000+320*150),4,320);

		// copy kernel and osca stack to screen
		constexpr SizeBytes kernel_size=512*2;
		Data src_kernel=Data(Address(0x7c00),kernel_size); // kernel binary
		Data dst_kernel=Data(Address(0xa'0000+320*151),kernel_size); // on screen
		src_kernel.to(dst_kernel);

		// draw red line
		pz_memset(Address(0xa'0000+320*155),4,320);

		// copy osca stack
		Data src_stk=Data(Address(0x7b00),256);
		Data dst_stk=Data(Address(0xa'0000+320*156),256); // on screen
		src_stk.to(dst_stk);

		// draw red line
		pz_memset(Address(0xa'0000+320*157),4,320);

		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
extern "C" [[noreturn]] auto tsk3()->void{
	osca_interrupts_disable();
	const int32 eax=osca_task_active->eax;
	const int32 ecx=osca_task_active->ecx;
	osca_interrupts_enable();

	while(true){
		const float f=float(osca_tick_lo)/float(ecx);
		*static_cast<uint32*>(vga13h.bmp().address_offset({CoordPx(eax),0}))=uint32(f);
		osca_yield();
	}
}
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
#include"game.h"
extern "C" [[noreturn]] auto tsk4()->void{
	osca::game::Game::run();
}
