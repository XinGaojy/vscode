
#if 0
#include<iostream>
#include<vector>
using namespace std;
class Heap{
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
		heap[1]=heap[index--];
		down(1);
	}
	int size(){
		return index;
	}	
	int top(){
		return heap[1];
	}
private:
	int capacity;
	int index;
	vector<int>heap;
	void up(int u){
		if(u/2 && heap[u/2]>heap[u]){
			swap(heap[u],heap[u/2]);
			up(u/2);
		} 
	}
	void down(int u){
		int v=u;
		if(u*2 <= index&& heap[u*2]<heap[v]) v=u*2;
		if(u*2+1<=index&& heap[u*2+1]<heap[v]) v=u*2+1;
		if(u!=v){
			swap(heap[u],heap[v]);
			down(v);
		}
	}
	
};
int main(){
	Heap hp=Heap(10);
	vector<int>vec={1,2,2,6,9,3,4};
	for(auto i:vec){
		hp.push(i);
		if(hp.size()>2){
			hp.pop();
		}
	}	
	cout<< hp.top();
}


#endif 



#include<iostream>
#include<vector>
using namespace std;
class Heap{
private:
	void up(int u){
		if(u/2 && heap[u/2]>heap[u]){
			swap(heap[u/2],heap[u]);
			up(u/2);
		}
	}
	void down(int u){
		int v=u;
		if(u*2 <= index && heap[u*2]<heap[v]) v=u*2;
	       	if(u*2+1 <= index && heap[u*2+1]< heap[v]) v=u*2+1;
		if(u!=v){
			swap(heap[u],heap[v]);
			down(v);
		}	
	}
	

public:
	int capacity;
	int index;
	vector<int>heap;
	Heap(int n){
		capacity=n;
		index=n;
		heap.resize(n+1);

	}
	void push(int x){
		heap[++index]=x;
		up(index);
	}
	void pop(){
		heap[1]=heap[index--];
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
	Heap hp = Heap(10);
	vector<int>vec = {1,2,3,4,5,6,4,6,8};
	for(auto& i:vec){
		hp.push(i);
		if(hp.size() > 8){
			hp.pop();
		}
	}
	cout<< hp.top();

return 0;
}
