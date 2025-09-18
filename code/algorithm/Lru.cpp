#if 0

#include<iostream>
#include<unordered_map>
using namespace std;
class LruCache{
private:
    struct ListNode{
        int key;
        int val;
        ListNode*pre;
        ListNode*next;
        ListNode(int k,int v):key(k),val(v),pre(nullptr),next(nullptr){}
    };
    ListNode*dummy;
    ListNode*last;
    unordered_map<int,ListNode*>map;
    int capacity;
private:
    void remove(ListNode*node){
        node->pre->next=node->next;
        node->next->pre=node->pre;
        map.erase(node->key);
    }
    void insert(int key,int value){
        ListNode*node=new ListNode(key,value);
        last->pre->next=node;
        node->pre=last->pre;
        last->pre=node;
        node->next=last;
        map[key]=node;
    }
public:
    LruCache(int n):capacity(n){
        dummy=new ListNode(-1,-1);
        last=new ListNode(-1,-1);
        dummy->next=last;
        last->pre=dummy;
    }
    int get(int key){
        if(map.count(key)){
            ListNode*node=map[key];
            remove(node);
            insert(node->key,node->val);
            return node->val;
        }
        else{
            return -1;
        }
    }
    void put(int key,int value){
        if(map.count(key)){
            ListNode*node=map[key];
            remove(node);
            insert(key,value);

        }
        else{
            if(map.size()==capacity){
                remove(dummy->next);
                insert(key, value);
            }
            else{
                insert(key,value);
            }
        }
    }

};

int main(){
    LruCache lru(4);
    cout<<lru.get(1)<<endl;
    lru.put(1,2);
    lru.put(2,3);
    lru.put(1,3);
    lru.put(2,2);
    lru.put(3,4);
    lru.put(4,5);
    lru.put(5,6);
    cout<<lru.get(1)<<endl;
    cout<<lru.get(2)<<endl;
    return 0;
}



#endif



#include<iostream>
class LruCache{
private:
    struct ListNode{
        int key;
        int val;
        ListNode*pre;
        ListNode*next;
        ListNode(int k,int v):key(k),val(v),pre(nullptr),next(nullptr){}
    };
    ListNode*dummy;
    ListNode*last;
    int capacity;
    unordered_map<int,ListNode*>map;
    void remove(ListNode *node){
        node->pre->next=node->next;
        node->next->pre=node->pre;
    }
    void insert(int key,int val){
        ListNode*node=new ListNode(key,val);
        last->pre->next=node;
        node->pre=last->pre;
        node->next=last;
        last->pre=node;
    }
public:
    LruCache(int n):capacity(n){
        dummy=new ListNode(-1,-1);
        last=new ListNode(-1,-1);
        dummy->next=last;
        last->pre=dummy;
    }
    int get(int key){
        if(map.count(key)){
            ListNode*node=map[key];
            remove(node);
            insert(node->key,node->val);
            return node->val;
        }   
        else{
            return -1;
        }     
    }
    void put(int key,int val){
        if(map.count(key)){
            ListNode*node=map[key];
            remove(node);
            insert(key,val);

        }
        else{
            if(map.size==capacity){
                remove(dummy->next);
                insert(key,value);
            }
            else{
                inset(key,val);
            }
        }
    }

}








// class LruCache{
// private:
//     struct ListNode{
//         int key;
//         int val;
//         ListNode* pre;
//         ListNode* next;
//         ListNode(int k,int v):key(k),val(v),pre(nullptr),next(nullptr){}
//     };
//     unordered_map<int,ListNode*>map;
//     ListNode* dummy;
//     ListNode*last;
//     int capacity_;
//     void remove(ListNode* node){
//         node->pre->next=node->next;
//         node->next->pre=node->pre;
//         map.erase(node->key);
//     }
//     void insert(int key,int val){
//         ListNode* node=new ListNode(key,val);
//         last->pre->next=node;
//         node->pre=last->pre;
//         node->next=last;
//         last->pre=node;
//         map[key]=val;
//     }
// public:
//     LruCache(int n):capacity(n){
//         dummy=new ListNode(-1,-1);
//         last=new ListNode(-1,-1);
//         dummy->next=last;
//         last->pre=dummy;

//     }
//     int get(int k){
//         if(map.count(k)){
//             ListNode*node=map[k];
//             insert(k,node->val);
//             remove(node);
//             return node->val;
//         }
//         else{
//             return -1;
//         }
//     }
//     void put(int key,int val){
//         if(map.count(key)){
//             ListNode* node=map[key];
//             insert(key,val);
//             remove(node);
//         }
//         else{
//             if(map.size()==capacity_){
//                 remove(dummy->next);
//                 insert(key,value);
//             }
//             else{
//                 insert(key,value);
//             }
//         }
//     }

// };


