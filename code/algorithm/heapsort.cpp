
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
		// 将新元素x插入到堆的末尾，并将index加1
		heap[++index]=x;
		// 调用up函数，调整堆的结构，确保堆的性质
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
#if 0

//new
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

#if 0
	Heap hp = Heap(10);
	vector<int>vec = {1,2,3,4,5,6,4,6,8};
	for(auto& i:vec){
		hp.push(i);
		if(hp.size() > 6){
			hp.pop();
		}
	}

#endif


#endif


#if 0
	Heap hp=Heap(4);
	vector<int>vec = {1,2,3,4,5,6,4,6,8};
	for(auto i:vec){
		if(hp.size()<4){
			hp.push(i);
		}else if(i>hp.top()){
			hp.pop();
			hp.push(i);
		}
	}

#endif


#if 0
	vector<int>res;
	while(hp.size()){
		res.push_back(hp.top());
		hp.pop();
	}
		
	for(auto i:res){
		cout << i <<endl;
	}


	return 0;
}

#endif





#if 0
#include<iostream>
using namespace std;
class Heap{
private:
	int index;
	int capacity;
	vector<int>heap;
	void up(int u){
		if(u/2 && heap[u/2]>heap[u]){
			swap(heap[u/2],heap[u]);
			up(u/2);
		}

		
	}

	void down(int u){
		int v=u;
		if(u*2<=index && heap[u*2]<heap[u])v=u*2;
		if(u*2+1<=index && heap[u*2+1]<heap[u])v=u*2+1;
		if(u!=v)
		{
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
	void push(int val){
		heap[++index]=val;
		up(index);
	}

	void pop(){
		heap[1]=heap[index--];
		down(1);
	}

	int size(){
		return size_;
	}

	int top(){
		return heap[1];
	}

};


int main(){

	return 0;
}


#endif





#if 0
#include<vector>
#include<iostream>
using namespace std;
class Heap{
private:
	int index;
	vector<int>heap;

	void up(int u){
		if(u/2 && heap[u/2]>heap[u]){
			swap(heap[u/2],heap[u]);
			up(u/2);
		}
	}

	void down(int u){
		int v=u;
		if(u*2<=index && heap[u*2]<heap[v])v=u*2;	//注意比较heap[u/2]<heap[v]而不是和heap[u*2]<heap[u];
		if(u*2+1<=index && heap[u*2+1]<heap[v])v=u*2+1;
		if(u!=v){
			swap(heap[v],heap[u]);
			down(v);
		}
	}


	
public:
	Heap(int n ){
		heap.resize(n+1);
		index=0;
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
	Heap hp(11);
	vector<int>vec={1,4,3,2,2,1,6,5,1};
	for(auto i:vec){
		hp.push(i);
	}
	while(hp.size()){
		cout<<hp.top()<<endl;
		hp.pop();
	}
	return 0;
}
#endif






#include<iostream>
#include<vector>
using namespace std;
class Heap{
private:
	int index;
	vector<int>heap;
	void up(int u){
		if(u/2 && heap[u/2]>heap[u]){
			swap(heap[u/2],heap[u]);
			up(u/2);
		}
	}

	void down(int u){
		int v=u;
		if(u*2<=index && heap[u*2]<heap[v])v=u*2;
		if(u*2+1<=index && heap[u*2+1]<heap[v])v=u*2+1;
		if(u!=v){
			swap(heap[u],heap[v]);
			down(v);
		}
		
	}

public:
	Heap(int n){
		index=0;
		heap.resize(n+1);
	}

	void push(int x){
		heap[++index]=x;
		up(index);
	}

	void  pop(){
		heap[1]=heap[index--];
		down(1);
	}
	
	int size()const {
		return index;
	}

	int top()const {
		return heap[1];
	}
};
int main(){
	vector<int>vec={1,4,3,2};
	Heap hp(10);
	for(auto i:vec){
		hp.push(i);

#if 0	
		if(hp.size()>2){
			hp.pop();
		}

#endif

	}
//	cout<<hp.top()<<endl;
	while(hp.size()){
		cout<<hp.top()<<endl;
		hp.pop();
	}
	return 0;
}






