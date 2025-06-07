LINE_BUFFER_SIZE := 208

AVRDUDE_PROGRAMMER ?= usbtiny

all: build-asm-helpers
	avr-gcc -g -mmcu=atmega8 -DLINE_BUFFER_SIZE=${LINE_BUFFER_SIZE} -DF_CPU=12000000UL -Os -o main.elf main.c video.c generated-sprites.S -Wextra -Wpedantic
	avr-size main.elf

build-asm-helpers:
	python generate-sprites.py \
		--basedir sprites \
		--input elephant.png \
		--input snake.png \
		--input monke.png

flash: all
	avrdude -c ${AVRDUDE_PROGRAMMER} -p m8 -v -U flash:w:main.elf
