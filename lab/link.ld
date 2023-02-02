SECTIONS {
	. =0x7c00;
	.text :{
		boot.o
		tasks.o
	}
	.data :{
		*(.data);
	}
}
