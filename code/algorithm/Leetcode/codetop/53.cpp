

#if 0
#include<iostream>
#include <vector>
using namespace std;

int main(){
    vector<int>vec={1,5,4,-1,-2,2};
    int dp=0;
    int index=0;
    int start=0;
    int end=0;
    int res=0;
    for(int i=0;i<vec.size();i++){
        if(dp<0){
            dp=vec[i];
            start=i;
        }
        else{
            dp+=vec[i];
        }
        if(dp>res){
            res=dp;
            end=i;
            index=start;
        }

    }
        vector<int>result(vec.begin()+index,vec.begin()+end+1);
        for(auto i:result){
            cout<<i<<endl;
        }    
    return 0;
}

#endif




#include<iostream>
#include<vector>
using namespace std;

int main(){
    vector<int>vec={1,3,2,3,-2,1};
    int index=0;
    int start=0;
    int end=0;
    int dp=0;
    int res=0;
    for(int i=0;i<vec.size();i++){
        if(dp<0){
            dp=vec[i];
            start=i;
        }else{
            dp+=vec[i];
        }
        if(dp>res){
            res=dp;
            end=i;
            //index=start;
        }

    }
    vector<int>result(vec.begin()+start,vec.begin()+end+1);
    for(auto i:result){
        cout<<i<<endl;
    }
    return 0;
}





