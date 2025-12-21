#include<iostream>
#include<unordered_map>
#include<string>
#include<cstdio>
#include<cstring>
using namespace std;

class Lrucache{
public:
	struct ListNode{
		char* key;
		string value;
		ListNode* pre;
		ListNode* next;
		ListNode(char* k,string v):key(k),value(v),pre(nullptr),next(nullptr){}
	};

	unordered_map<char* ,ListNode*>map;
	
    string getfirstline(char *filename){
        FILE* file = fopen(filename,"r");
        if(!file) return "";
        string s;
        char buffer[1024];
        if(fgets(buffer, sizeof(buffer), file)){
            s = string(buffer);
            // 移除换行符
            if(!s.empty() && s[s.length()-1] == '\n'){
                s.pop_back();
            }
        }
        fclose(file);
        return s;
    }
	
	Lrucache(int n){
		capacity=n;
		dummy=new ListNode((char*)"-1","");
		last=new ListNode((char*)"-1","");
		dummy->next=last;
		last->pre=dummy;
	}

	string get (char *filename){
        if(map.count(filename)){
            ListNode* node=map[filename];
            string val=node->value;
            remove(node);
            insert(node->key,node->value);
            return val;
        }
        else{
            FILE *fd=fopen(filename,"r");
            if(!fd) return "";
            fclose(fd);
            string s=getfirstline(filename);
            ListNode* node=new ListNode(filename,s);
            if(map.size()>=capacity){
                remove(dummy->next);
            }
            insert(node->key,node->value);
            map[filename]=node;
            return node->value;
        }
	}


	void put(char *filename,string s){
        if(map.count(filename)){
            ListNode* node=map[filename];
            remove(node);
            insert(filename,s);
            map[filename]=new ListNode(filename,s);
        }else{
            if(map.size()>=capacity){
                remove(dummy->next);
            }
            insert(filename,s);
            map[filename]=new ListNode(filename,s);
        }
	}


private:
	ListNode* dummy;
	ListNode* last;
	int capacity;
	
	void insert(char*k,string value){
        ListNode* node=new ListNode(k,value);
        node->pre=last->pre;
        node->next=last;
        last->pre->next=node;
        last->pre=node;
        map[k]=node;
    }

    void remove(ListNode* node){
        if(node->pre && node->next){
            node->pre->next=node->next;
            node->next->pre=node->pre;
            map.erase(node->key);
            delete node;
        }
    }
};

int main(){
    // 简单测试
    Lrucache cache(2);
    
    // 创建测试文件
    FILE* f1 = fopen("test1.txt", "w");
    fprintf(f1, "content1\n");
    fclose(f1);
    
    FILE* f2 = fopen("test2.txt", "w");
    fprintf(f2, "content2\n");
    fclose(f2);
    
    FILE* f3 = fopen("test3.txt", "w");
    fprintf(f3, "content3\n");
    fclose(f3);
    
    cout << "Get test1: " << cache.get("test1.txt") << endl;
    cout << "Get test2: " << cache.get("test2.txt") << endl;
    cout << "Get test3: " << cache.get("test3.txt") << endl; // 应该淘汰test1
    
    //remove("test1.txt");
    //remove("test2.txt");
    //remove("test3.txt");
    
    return 0;
}
