#pragma once
#include <stdint.h>

__attribute__((used))
static void set8(void* destination, uint8_t value, uint64_t count) {
    __asm__ volatile ("rep stosb" : "+D"(destination), "+c"(count) : "a"(value) : "memory");
}

__attribute__((used))
static void set16(void* destination, uint16_t value, uint64_t count) {
    __asm__ volatile ("rep stosw" : "+D"(destination), "+c"(count) : "a"(value) : "memory");
}

__attribute__((used))
static void set32(void* destination, uint32_t value, uint64_t count) {
    __asm__ volatile ("rep stosl" : "+D"(destination), "+c"(count) : "a"(value) : "memory");
}

__attribute__((used))
static void set64(void* destination, uint64_t value, uint64_t count) {
    __asm__ volatile ("rep stosq" : "+D"(destination), "+c"(count) : "a"(value) : "memory");
}

__attribute__((used))
static void copy8(void* source, void* destination, uint64_t count) {
    __asm__ volatile ("rep movsb" : "+S"(source), "+D"(destination), "+c"(count) : : "memory");
}

__attribute__((used))
static void copy16(void* source, void* destination, uint64_t count) {
    __asm__ volatile ("rep movsw" : "+S"(source), "+D"(destination), "+c"(count) : : "memory");
}

__attribute__((used))
static void copy32(void* source, void* destination, uint64_t count) {
    __asm__ volatile ("rep movsl" : "+S"(source), "+D"(destination), "+c"(count) : : "memory");
}

__attribute__((used))
static void copy64(void* source, void* destination, uint64_t count) {
    __asm__ volatile ("rep movsq" : "+S"(source), "+D"(destination), "+c"(count) : : "memory");
}

__attribute__((used))
static void copy128(void* source, void* destination, uint64_t count) {
    for (uint64_t i = 0; i < count; i++)
    {
        __asm__ volatile ("vmovdqu %0, %%xmm1; vmovdqu %%xmm1, %1" : : "m"(*(uint8_t*)source), "m"(*(uint8_t*)destination) : "%xmm1", "memory");
        source += 16;
        destination += 16;
    }
}

__attribute__((used))
static void copy256(const void* source, void* destination, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        __asm__ volatile (
            "vmovdqu %0, %%ymm1\n\t"
            "vmovdqu %%ymm1, %1"
            :
            : "m"(*(const uint8_t*)source), "m"(*(uint8_t*)destination)
            : "%ymm1", "memory"
        );
        source = (const uint8_t*)source + 32;
        destination = (uint8_t*)destination + 32;
    }
}

