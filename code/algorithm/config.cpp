
#include<unistd.h>
//#include<unique_ptr>
#include<iostream>
#include <memory>
#include<vector>
#include<string>
using namespace std;
int main(){
    unique_ptr<FILE,decltype(&fclose)> fp(fopen("config.txt","r"),&fclose);
    if(fp==nullptr){
        exit(1);
    }
    char buffer[1024];
    vector<string>vec;
    while(fgets(buffer,1024,fp.get())!=nullptr){
        cout<<buffer;
        vec.push_back(buffer);
    }
    for(auto i:vec){
        cout<<i;
    }
    return 0;
}




