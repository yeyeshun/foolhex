#include "kmp.h"

void kmp_cal_next(const unsigned char *str, int len, int *next)
{
    next[0] = -1;//next[0]��ʼ��Ϊ-1��-1��ʾ��������ͬ�����ǰ׺������׺
    int k = -1;//k��ʼ��Ϊ-1
    for (int q = 1; q <= len - 1; q++)
    {
        while (k > -1 && str[k + 1] != str[q])//�����һ����ͬ����ôk�ͱ��next[k]��ע��next[k]��С��k�ģ�����kȡ�κ�ֵ��
        {
            k = next[k];//��ǰ����
        }
        if (str[k + 1] == str[q])//�����ͬ��k++
        {
            k = k + 1;
        }
        next[q] = k;//����ǰ����k��ֵ��������ͬ�����ǰ׺������׺��������next[q]
    }
}

int KMP(const unsigned char *str, int slen, const unsigned char *ptr, int plen, const int* next)
{
//     int *next = new int[plen];
//     cal_next(ptr, next, plen);//����next����
    int k = -1;
    for (int i = 0; i < slen; i++)
    {
        while (k >-1 && ptr[k + 1] != str[i])//ptr��str��ƥ�䣬��k>-1����ʾptr��str�в���ƥ�䣩
            k = next[k];//��ǰ����
        if (ptr[k + 1] == str[i])
            k = k + 1;
        if (k == plen - 1)//˵��k�ƶ���ptr����ĩ��
        {
            //cout << "��λ��" << i-plen+1<< endl;
            //k = -1;//���³�ʼ����Ѱ����һ��
            //i = i - plen + 1;//i��λ����λ�ã����forѭ��i++���Լ�������һ��������Ĭ�ϴ�������ƥ���ַ������Բ����ص�������л������ͬѧָ������
            return i - plen + 1;//������Ӧ��λ��
        }
    }
    return -1;
}

void kmp_cal_next_ignore_case(const unsigned char *lower_str, int len, int *next)
{
    kmp_cal_next(lower_str, len, next);
}

int KMP_ignore_case(const unsigned char *str, int slen, const unsigned char *lower_str, int plen, const int* next)
{
    int k = -1;
    for (int i = 0; i < slen; i++)
    {
        unsigned char char_lower = str[i];
        if (char_lower >= 'A' && char_lower <= 'Z')
            char_lower += 0x20;
        while (k >-1 && lower_str[k + 1] != str[i] && lower_str[k + 1] != char_lower)//ptr��str��ƥ�䣬��k>-1����ʾptr��str�в���ƥ�䣩
            k = next[k];//��ǰ����
        if (lower_str[k + 1] == str[i] || lower_str[k + 1] == char_lower)
            k = k + 1;
        if (k == plen - 1)//˵��k�ƶ���ptr����ĩ��
        {
            //cout << "��λ��" << i-plen+1<< endl;
            //k = -1;//���³�ʼ����Ѱ����һ��
            //i = i - plen + 1;//i��λ����λ�ã����forѭ��i++���Լ�������һ��������Ĭ�ϴ�������ƥ���ַ������Բ����ص�������л������ͬѧָ������
            return i - plen + 1;//������Ӧ��λ��
        }
    }
    return -1;
}
