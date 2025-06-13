LINE_BUFFER_SIZE := 208

AVRDUDE_PROGRAMMER ?= usbtiny
CAMERA_NAME ?= USB2.0\ PC\ CAMERA\ \(usb-0000:00:14.0-2\)\:

all: build-asm-helpers
	avr-gcc -g -mmcu=atmega8 -DLINE_BUFFER_SIZE=${LINE_BUFFER_SIZE} -DF_CPU=12000000UL -Os -o main.elf main.c video.c generated-sprites.S -Wextra -Wpedantic
	avr-size main.elf

build-asm-helpers:
	python generate-sprites.py \
		--basedir sprites \
		--input undyne_head_normal.png \
		--input undyne_head_blink.png \
		--input annoying_dog_1.png \
		--input annoying_dog_2.png \
		--input undyne_body.png --compress

flash: all
	avrdude -c ${AVRDUDE_PROGRAMMER} -p m8 -v -U flash:w:main.elf -B1

gif:
	./tools/capture-gif.sh ${CAMERA_NAME} ./res/progress.gif

.PHONY: all flash build-asm-helpers
