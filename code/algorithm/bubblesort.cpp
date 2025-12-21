
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


#if 0
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
	vector<int> vec={1,5,3,2,1};
	bubblesort(vec);
	for(auto i:vec){
		cout<<i <<endl;
	}
	return 0;
}



#endif




#if 0
#include<iostream>
#include<vector>
using namespace  std;
void bubblesort(vector<int>& vec){
	int n=vec.size();
	for(int i=0;i<n;i++){
		bool flag=true;
		for(int j=0;j<n-i-1;j++){
			
			if(vec[j] > vec[j+1]){
				swap(vec[j+1],vec[j]);
				flag=false;
			}
		}
		if(flag){
			break;
		}
	}
}
int main(){
	vector<int>vec={1,5,3,2,1};

	bubblesort(vec);
	for(auto i:vec){
		cout<<i<<endl;
	}
	return 0;
}


#endif






#include<iostream>
using namespace std;
void bubblesort(vector<int>& vec){
	int n=vec.size();
	for(int i=0;i<n;i++){
		bool flag=false;
		for(int j=0;j<n-i-1;j++){
			if(vec[j]>vec[j+1]){
					swap(vec[j],vec[j+1]);
					flag=true;
			}
		}

		if(flag==false){
			break;
		}
	}
}
int main(){
	vector<int>vec={-1,5,1,2,2,4,3,2};
	bubblesort(vec);
	for(auto i:vec){
			cout<<i<<endl;
	}
	return 0;
}






