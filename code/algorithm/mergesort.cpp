//实现归并排序


#if 0
#include<iostream>
#include<vector>
using namespace std;
void mergeSort(vector<int> vec,vector<int>&res,int left,int right){
    if(left>=right)return ;
    int mid=(right-left)/2+left;
    mergeSort(vec,res,left,mid);
    mergeSort(vec,res,mid+1,right);
    int i=left;
    int j=mid+1;
    int k=left;
    while(i<=mid && j<=right){
        if(vec[i]<=vec[j])res[k++]=vec[i++];
        else{
            res[k++]=vec[i++];
        }
    }
    while(i<=mid)res[k++]=vec[i++];
    while(j<=right)res[k++]=vec[j++];
    for(int i=left;i<=right;i++)vec[i]=res[i];
}


int main(){
    vector<int>vec={1,6,5,3,2};
    vector<int>res(vec.size());
    mergeSort(vec,res,0,vec.size()-1);
    for(auto i:res){
      cout<<i<<endl;
    }
    return 0;
}



#endif

#if 0


#include<vector>
#include<iostream>
using namespace std;

void mergeSort(vector<int>&vec,vector<int>&res,int left,int right){
	if(left>=right)return ;
	int mid=(right-left)/2+left;
	mergeSort(vec,res,left,mid);
	mergeSort(vec,res,mid+1,right);
	int i=left;
	int j=mid+1;
	int k=left;
	while(i<=mid && j<=right){
		if(vec[i]<vec[j]){
				res[k++]=vec[i++];
		}
		else{
				res[k++]=vec[j++];
		}
		
	}
	while(i<=mid){
		res[k++]=vec[i++];
	}
	while(j<=right){
		res[k++]=vec[j++];
	}
	for(int i=left;i<=right;i++)vec[i]=res[i];
}
int main(){
	vector<int>vec={1,5,4,3,2};
	vector<int>res(vec.size());
	mergeSort(vec,res,0,vec.size()-1);
	for(auto i:res){
		cout<<i<<endl;
	}
	return 0;
}




#endif




#if 0


#include<iostream>
#include<vector>
using namespace std;

void mergeSort(vector<int>& vec,vector<int>& res,int left ,int right){
	if(left>=right)return ;
	int mid=(right-left)/2+left;
	mergeSort(vec,res,left,mid);
	mergeSort(vec,res,mid+1,right);
	int i=left;
	int j=mid+1;
	int k=left;
	while(i<=mid && j<=right){
		if(vec[i]<vec[j]){
			res[k++]=vec[i++];
		}
		else{
			res[k++]=vec[j++];
		}
	}

	while(i<=mid){
			res[k++]=vec[i++];
	}

	while(j<=right){
		res[k++]=vec[j++];
	}

	for(int i=left;i<=right;i++){
			vec[i]=res[i];
	}
}
int main(){
	//vector<int>vec={1,4,3,2};
	vector<int>vec={1,5,4,4,3,2};	
	vector<int>res(vec.size());
	
	mergeSort(vec,res,0,vec.size()-1);
	
	for(auto i:res){
		cout<<i<<endl;
	}


	return 0;
}


#endif

#if 0

#include<iostream>
#include<vector>
using namespace std;

void mergeSort(vector<int>& vec,vector<int>& res,int left,int right){
	if(left>=right)return ;

	int mid=(right-left)/2+left;
	mergeSort(vec,res,left,mid);
	mergeSort(vec,res,mid+1,right);

	int i=left;
	int j=mid+1;
	int k=left;
	while(i<=mid && j<=right){
		if(vec[i]<vec[j]){
			res[k++]=vec[i++];
		}else{
			res[k++]=vec[j++];
		}	
		
	}
	while(i<=mid){
		res[k++]=vec[i++];
	}

	while(j<=right){
		res[k++]=vec[j++];
	}

	for(int i=left;i<=right;i++){
		vec[i]=res[i];	
	}

}
int main(){
	//vector<int>vec={1,5,4,3,2};
	vector<int>vec={1,6,5,2,1,1,3,4};
	vector<int>res(vec.size());

	mergeSort(vec,res,0,vec.size()-1);
	for(auto i:res){
		cout<<i<<endl;
	}
	return 0;
}


#endif








#if 0
#include<iostream>
#include<vector>
using namespace std;

void mergeSort(vector<int>& vec,vector<int>&res,int left,int right){
	if(left>=right)return;
	int mid=(right-left)/2+left;
	mergeSort(vec,res,left,mid);
	mergeSort(vec,res,mid+1,right);

	int i=left;
	int j=mid+1;
	int k=left;
	while(i<=mid && j<=right){
		if(vec[i]<vec[j]){
			res[k++]=vec[i++];
		}else{
			res[k++]=vec[j++];	
		}

	}

	while(i<=mid){
		res[k++]=vec[i++];
	}

	while(j<=right){
		res[k++]=vec[j++];
	}

	for(int i=left;i<=right;i++){
		vec[i]=res[i];
	}
}
int main(){
	vector<int>vec={1,4,3,2,-1,0};
	vector<int>res(vec.size());
	mergeSort(vec,res,0,vec.size()-1);
	for(auto i:vec){
		cout<<i<<endl;
	}

	return 0;
}	



#endif


#include<vector>
#include<iostream>
using namespace std;
namespace name{
	void  mergesort(vector<int>&vec,vector<int>&res,int left,int right){
			if(left>=right)return ;
			int mid=(right-left)/2+left;
			mergesort(vec,res,left,mid);
			mergesort(vec,res,mid+1,right);
			int i=left;
			int j=mid+1;
			int k=left;
			while(i<=mid && j<=right){
				if(vec[i]<vec[j]){
					res[k++]=vec[i++];
				}
				else{
					res[k++]=vec[j++];
				}
			}

			while(i<=mid){
				res[k++]=vec[i++];
			}

			while(j<=right){
				res[k++]=vec[j++];
			}
			for(int i=left;i<=right;i++){
				vec[i++]=res[i++];
			}
	}

};

int main(){
	vector<int>vec={1,5,4,3,2};
	vector<int>res(vec.size());
	name::mergesort(vec,res,0,vec.size()-1);
	for(auto i:res){
		cout<<i<<endl;
	}
	return 0;
}
