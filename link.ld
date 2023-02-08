OUTPUT_FORMAT("binary");
SECTIONS {
	. =0x7c00;
	.text :{
		bin/src/osca.o
		bin/src/main.o
	}
	.data :{
		*(.data);
	}
}
