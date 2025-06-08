#pragma once

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define barrier() do { asm volatile ("" ::: "memory"); } while (0)
