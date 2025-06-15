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
	sprites/annoying_dog_overworld_dangling_rope.png \
	sprites/hello.png

build/%.o : caveman/%.c Makefile | build
	${CC} ${CFLAGS} -c $< -o $@

all: build/caveman.elf

build/generated-sprites.gen.o : build/generated-sprites.gen.S Makefile | build
	${CC} ${CFLAGS} -c $< -o $@

build:
	mkdir -p ./build

clean:
	rm -rf ./build

build/caveman.elf: ${objects} Makefile | build
	${CC} ${LDFLAGS} ${objects} -o $@
	avr-size $@

build/generated-sprites.gen.S: $(sprites) | build
	python ./tools/generate-sprites.py \
		--basedir sprites \
		$(addprefix --input ,$(notdir $(sprites))) \
		--compress build/generated-sprites.gen

flash: build/caveman.elf
	avrdude -c ${AVRDUDE_PROGRAMMER} -p m8 -v -U flash:w:$< -B1

gif:
	./tools/capture-gif.sh ${CAMERA_NAME} ./res/progress.gif

.PHONY: all clean flash gif

-include $(objects:.o=.d)
