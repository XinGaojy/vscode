

#include<iostream>
#include<vector>
using namespace std;
struct ListNode{
    int value;
    ListNode* next;
    ListNode(int v):value(v),next(nullptr){}
};
ListNode* createList(vector<int>vec){
    ListNode* dummy=new ListNode(-1);
    ListNode* cur=dummy;
    for(auto i:vec){
        cur->next=new ListNode(i);
        cur=cur->next;
    }
    //cur->next=nullptr;
    return dummy->next;
}
void print(ListNode* head){
    ListNode* cur=head;
    while(cur){
        cout<<cur->value<<"---->";
        cur=cur->next;
    }
}
1 1 2 3 
ListNode* uniqueList(ListNode* head){
    ListNode* dummy=new ListNode(-1);
    dummy->next=head;
    ListNode* cur=head;
    while(cur&& cur->next){
        if(cur->next->value==cur->value){
            cur->next=cur->next->next;
            cur=cur->next;
        }
    }   
    return dummy->next;
}
ListNode* mergeList(ListNode* list1,ListNode* list2){
    ListNode* pre=list1;
    ListNode* cur=list2;
    ListNode* dummy=new ListNode(-1);
    ListNode* p=dummy;
    while(pre && cur){
        if(pre->val>cur->val){
            p->next=cur;
            cur=cur->next;
        }
        else if(pre->val<cur->val){
            p->next=pre;
            pre=pre->next;
        }
        else{
            p->next=pre;
            pre=pre->next;
            cur=cur->next;
        }
    }
    if(pre){
        uniqueList(pre);
    }
    else{
        uniqueList(cur);
    }
}
int main(){
    vector<int>vec={1,3,3,4,4,2};
    ListNode* head=createList(vec);
    print(head);
    return 0;
}