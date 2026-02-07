//手撕字符转拷贝:
//https://cloud.tencent.com/developer/article/1510222

#if 0
char *strcpy1(char *dest, const char *src) {
    if (!dest || !src)
        return NULL;
    char *d = dest;
    int size = strlen(src) + 1;
    if (d > src && d < src + size) {
        d = d + size - 1;
        src = src + size - 1;
        while (size--) {
            *d-- = *src--;
        }
    } else {
        while (size--) {
            *d++ = *src++;
        }
    }
}



void *memcpy1(void *dest, const void *src, size_t n) {
    if (!dest || !src)
        return NULL;
    char *d = (char *) dest;
    const char *s = (const char *) src;
    if (d > s && d < s + n) {
        d = d + n - 1;
        s = s + n - 1;
        while (n--)
            *d-- = *s--;
    } else {
        while (n--)
            *d++ = *s++;
    }
    return dest;
}


int strcmp(const char* str1,const char* str2) {
    while(*str1==*str2&&*str1!='\0') {
        str1++;
        str2++;
    }
    return *str1-*str2;
}char* strcat(char* dest,const char* src) {
    char* d = dest;
    while(*d!='\0') ++d;

    while(*src!='\0') {
        *d++=*src++;
    }
    *d='\0';
    return dest;
}

char* strstr(char *str1, char *str2) {
    if (str1 == NULL || str2 == NULL) return NULL;
    char *s = str1;
    if (*str2 == '\0') {
        return NULL;//若str2为空，则直接返回空
    }
    while (*s != '\0') {//若不为空，则进行查询
        char *s1 = s;
        char *s2 = str2;
        while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
            s1++, s2++;
        }
        if (*s2 == '\0') {
            return s;//若s2先结束
        }
        if (*s2 != '\0' && *s1 == '\0') {
            return NULL;//若s1先结束而s2还没结束，则返回空
        }
        s++;
    }
    return NULL;
}
#endif




#include<iostream>
#include<stdlib.h>
#include<cstring>
using namespace std;
int main(){
    struct element{
        int key;
        int value;
    };
    element e{1,2};
    char buffer[8]={0};
    cout<<e.key<<" "<<e.value<<endl;
    //strcpy(buffer,"1234567890");
    //strcpy(buffer,"12\034567891112131431212121211111111121211222122222222");
    memcpy(buffer,"12\034567891112131431212121211111111121211222122222222",10);
    //strcpy_s(buffer,sizeof(buffer),"1234567890");//linux不支持这个api
    cout<<e.key<<" "<<e.value<<endl;
    cout<<buffer<<endl;


    char buffer1[100]={'a','b','c','d','e','f','g','h'};
    memcpy(buffer1+20,buffer1,10);
    for(int i=0;i<100;i++){
        cout<<buffer1[i]<<endl;
    }
    cout<<buffer1<<endl;
}




