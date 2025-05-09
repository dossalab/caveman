LINE_BUFFER_SIZE := 208

all: build-asm-helpers
	avr-gcc -mmcu=atmega8 -DLINE_BUFFER_SIZE=${LINE_BUFFER_SIZE} -DF_CPU=12000000UL -Os -o main.elf main.c video.c

build-asm-helpers:
	python build-asm-helpers.py --line-buffer-size ${LINE_BUFFER_SIZE}
