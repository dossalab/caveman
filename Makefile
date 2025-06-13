AVRDUDE_PROGRAMMER ?= usbtiny
CAMERA_NAME ?= USB2.0\ PC\ CAMERA\ \(usb-0000:00:14.0-2\)\:

CC := avr-gcc

COMMON_FLAGS := -mmcu=atmega8 -Os

CFLAGS := \
	${COMMON_FLAGS} \
	-Ibuild/ -Icaveman \
	-DF_CPU=12000000UL \
	-Wextra \
	-Wpedantic

LDFLAGS := \
	${COMMON_FLAGS}

objects := \
	build/generated-sprites.gen.o \
	build/main.o \
	build/video.o

build/%.o : caveman/%.c
	${CC} ${CFLAGS} -c $< -o $@

all: build/caveman.elf

build/generated-sprites.gen.o : build/generated-sprites.gen.S
	${CC} ${CFLAGS} -c $< -o $@

build:
	mkdir -p ./build

clean:
	rm -r ./build

build/caveman.elf: ${objects}
	${CC} ${CFLAGS} $^ -o $@
	avr-size $@

build/generated-sprites.gen.S: build
	python ./tools/generate-sprites.py \
		--basedir sprites \
		--input undyne_head_normal.png \
		--input undyne_head_blink.png \
		--input annoying_dog_1.png \
		--input annoying_dog_2.png \
		--input undyne_body.png --compress build/generated-sprites.gen

flash: all
	avrdude -c ${AVRDUDE_PROGRAMMER} -p m8 -v -U flash:w:main.elf -B1

gif:
	./tools/capture-gif.sh ${CAMERA_NAME} ./res/progress.gif

.PHONY: all compile flash clean
