# working dir is './bin'

OUTPUT_FORMAT("binary");
SECTIONS {
	. =0x7c00;
	.text :{
		src/osca.o
		src/main.o
	}
	.data :{
		*(.data);
	}
}
