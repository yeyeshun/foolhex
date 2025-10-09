#pragma once

void kmp_cal_next(const unsigned char *str, int len, int *next);
int KMP(const unsigned char *str, int slen, const unsigned char *ptr, int plen, const int* next);

// lower_str可以是任意编码，但是要按照正确的编码先转成小写
void kmp_cal_next_ignore_case(const unsigned char *lower_str, int len, int *next);
// lower_str可以是任意编码，但是要按照正确的编码先转成小写
int KMP_ignore_case(const unsigned char *str, int slen, const unsigned char *lower_str, int plen, const int* next);

