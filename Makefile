AVRDUDE_PROGRAMMER ?= usbtiny
CAMERA_NAME ?= USB2.0\ PC\ CAMERA\ \(usb-0000:00:14.0-2\)\:

CC := avr-gcc

COMMON_FLAGS := -mmcu=atmega8 -Os

CFLAGS := \
	${COMMON_FLAGS} \
	-Ibuild/ -Icaveman \
	-DF_CPU=12000000UL \
	-Wextra \
	-Wpedantic \
	-MMD -MP

LDFLAGS := \
	${COMMON_FLAGS}

objects := \
	build/generated-sprites.gen.o \
	build/main.o \
	build/video.o

tool-deps := \
	tools/generate-sprites.py \
	tools/compressor.py \
	tools/compressor-demo.py \
	Makefile

sprites := \
	sprites/annoying_dog_overworld_dangling_1.png \
	sprites/annoying_dog_overworld_dangling_2.png \
	sprites/annoying_dog_overworld_dangling_3.png \
	sprites/annoying_dog_overworld_dangling_4.png \
	sprites/annoying_dog_overworld_dangling_5.png \
	sprites/annoying_dog_overworld_dangling_6.png \
	sprites/annoying_dog_overworld_dangling_7.png \
	sprites/annoying_dog_overworld_dangling_8.png \
	sprites/annoying_dog_overworld_dangling_9.png \
	sprites/annoying_dog_overworld_dangling_10.png \
	sprites/annoying_dog_overworld_dangling_rope.png \
	sprites/hello.png

build/%.o : caveman/%.c $(tool-deps) | build
	${CC} ${CFLAGS} -c $< -o $@

all: build/caveman.elf

build/generated-sprites.gen.o : build/generated-sprites.gen.S $(tool-deps) | build
	${CC} ${CFLAGS} -c $< -o $@
	avr-size $@

build:
	mkdir -p ./build

clean:
	rm -rf ./build

build/caveman.elf: ${objects} $(tool-deps) | build
	${CC} ${LDFLAGS} ${objects} -o $@
	avr-size $@

build/generated-sprites.gen.S: $(sprites) $(tool-deps) | build
	python ./tools/compressor-demo.py \
		--basedir sprites \
		$(addprefix --input ,$(notdir $(sprites))) \
		--debug --force \
		--compress build/generated-sprites.gen

flash: build/caveman.elf
	avrdude -c ${AVRDUDE_PROGRAMMER} -p m8 -v -U flash:w:$< -B1

gif:
	./tools/capture-gif.sh ${CAMERA_NAME} ./res/progress.gif

.PHONY: all clean flash gif

-include $(objects:.o=.d)
