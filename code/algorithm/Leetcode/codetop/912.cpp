


#include<iostream>
#include<vector>
using namespace std;
void quicksort(vector<int>&vec,int left,int right){
    if(left>=right)return ;
    int i=left-1;
    int j=right+1;
    int mid=(right-left)/2+left;
    int x=vec[mid];
    while(i<j){
        do i++;while(vec[i]<x);
        do j--;while(vec[j]>x);
        if(i<j){
            swap(vec[i],vec[j]);
        }
    }
    quicksort(vec, left, j);
    quicksort(vec, j+1, right);
}
int main(){
    vector<int>vec={1,4,3,2,2,1};
    quicksort(vec,0,vec.size()-1);
    for(auto i:vec){
        cout<<i<<endl;
    }
    return 0;
}