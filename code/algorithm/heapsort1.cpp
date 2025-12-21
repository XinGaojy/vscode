
#include<iostream>
#include<vector>
using namespace std;
calss Heap{
private:
    int index;
    int capacity;
    vector<int>heap;
    void up(int u){
        if(u/2 && heap[u/2] > heap[u]){
            swap(heap[u/2],heap[u]);
            up(u/2);
        }
    }
    void down(int u){
        int v=u;
        //找到最小的下标
        if(u*2<=index && heap[u*2]<heap[v])v=u*2;
        if(u*2+1<=index && heap[u*2+1]<heap[v])v=u*2+1;

        //递归
        if(u!=v){
            swap(heap[u],heap[v]);
            down(v);
        }
    }
public:
    Heap(int n){
        capacity=n;
        index=0;
        heap.resize(n+1);
    }
    void push(int x){
        heap[++index]=x;
        up(index);
        
    }
    void pop(){
        heap[1]=heap[index++];
        down(1);
        
    }
    int size(){
        return index;
    }
    int top(){
        return heap[1];
    }
};
int main(){
    Heap hp(10);
    vector<int>vec={1,2,5,4,3};
    for(auto i:vec){
        hp.push(i);
        if(hp.size()>2){
            hp.pop();
        }
    }
    cout<<  hp.top() <<endl;
    return 0;
}
