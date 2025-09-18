#include<iostream>
#include<vector>

using namespace std;

#if 0
void bubblesort(vector<int>& vec){
	for(int i=0;i<vec.size();i++){
		bool flag=false;
		for(int j=0;j< vec.size()-i-1;j++){
			if(vec[j]>vec[j+1]){
				swap(vec[j],vec[j+1]);
				flag=true;
			}
		}
		if(!flag){
			break;
		}
	}
}

#endif

void bubblesort(vector<int>& vec){
	int n=vec.size();
	for(int i=0;i<n;i++){
		bool flag=false;
		for(int j=0;j<n-i-1;j++){
			if(vec[j+1]<vec[j]){
				swap(vec[j],vec[j+1]);
				flag=true;
			}
		}
		if(!flag){
			break;
		}
	}
}
int main(){
	vector<int> vec={2,3,1,2,1};
	bubblesort(vec);
	for(auto i:vec){
		cout<<i <<endl;
	}
	return 0;
}
