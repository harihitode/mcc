#include <stdio.h>
#include <stdint.h>
#include <math.h>

float min_caml_atan(float f) {
    return atanf(f);
}

float min_caml_sin(float f) {
    return sinf(f);
}

float min_caml_cos(float f) {
    return cosf(f);
}

float min_caml_sqrt(float f) {
    return sqrtf(f);
}

int32_t min_caml_read_int() {
    int r;
    scanf("%d", &r);
    return r;
}

float min_caml_read_float() {
    float r;
    scanf("%f", &r);
    return r;
}

float min_caml_fabs(float f) {
    return fabs(f);
}

float min_caml_fhalf(float f) {
    return f * 0.5;
}

int32_t min_caml_fisneg(float f) {
    return (f < 0.0) ? 1 : 0;
}

int32_t min_caml_fispos(float f) {
    return (f > 0.0) ? 1 : 0;
}

int32_t min_caml_fiszero(float f) {
    return (f == 0.0) ? 1 : 0;
}

int32_t min_caml_fless(float lhf, float rhf) {
    return (lhf < rhf) ? 1 : 0;
}

float min_caml_fneg(float f) {
    return -f;
}

float min_caml_fsqr(float f) {
    return f * f;
}

void min_caml_print_char(int32_t c) {
    char cc = (char)c;
    putchar(cc);
}

void min_caml_print_int(int32_t i) {
    printf("%d", i);
}

int32_t min_caml_truncate(float f) {
    return (f < 0) ? ceil(f) : floor(f);
}

int32_t min_caml_ceil(float f) {
    return ceilf(f);
}

int32_t min_caml_floor(float f) {
    return floorf(f);
}

float min_caml_float_of_int(int32_t i) {
    float f = i;
    return f;
}

int32_t min_caml_int_of_float(float f) {
    int32_t i = f;
    return i;
}

void min_caml_print_float(float f) {
    printf("%f", f);
}

void min_caml_print_newline() {
    putchar('\n');
}
