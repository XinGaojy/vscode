#if 0
#include<algorithm>
#include<iostream>
#include<vector>
using namespace std;
class observe{
public:
  virtual ~observe(){}

  virtual void update(const string &s)=0;
  
};

class object{
private:
  vector<observe*>vec_;
  string state_;
  
public:
  void push(observe* o){
    vec_.push_back(o);
  }

  void pop(observe* o){
    vec_.erase(std::remove(vec_.begin(),vec_.end(),o),vec_.end());

  }

  void setstate(const string &newstate){
    state_=newstate;
    notify();
  }

  void notify(){
    for(auto i:vec_){
      i->update(state_);
    }
  }
};

class phone:public observe{
private:
  string name_;
public:
  phone(const string &name):name_(name){}

  void update(const string & m) override{
    cout<<name_<< m<<endl;

  }

};

class computer:public observe{
public:
  computer(const string &name):name_(name){}
  void update(const string &m)override{
    cout<<name_<<m<<endl;
  }
private:
  string name_;
};
int main(){
  phone iphone("iphone14");
  phone android("华为");
  computer macbook("macbook14");
  object ob;
  ob.push(&iphone);
  ob.push(&android);
  ob.push(&macbook);
  ob.setstate("newversion");

  ob.pop(&android);
  ob.setstate("lastversion");
}



#endif





#if 0
#include<vector>
#include<iostream>
using namespace std;
class observe{
  virtual ~observe(){}
  
  virtual void update(const string s) = 0;
  
};

class object{
private:
  string name_;
  vector<observe*>vec_;


public:
  void push(observe*o){
    vec_.push_back(o);

  }

  void pop(observe*o){
    vec_.erase(std::remove(vec_.begin(),vec_.end(),o),vec_.end());

  }
  
  void set_state(const string &s){
      name_=s;
      notify();
  }

  void notify(){
    for(auto i:vec_){
      i->update(name_);
    }
  }

};

class phone:public observe{
private:
  string name_;
public:
    void update(const string &name)override{
      cout<<name<<name_<<endl;
    }


}
class computer:public observe{
private:
  string name_;
public:
  void update(const string &name)override{
    cout<<name<<name_<<endl;

  }
};
int main(){

  phone iphone("iphone14");
  phone android("华为");
  computer macbook("macbook14");
  object ob;
  ob.push(&iphone);
  ob.push(&android);
  ob.push(&macbook);
  ob.setstate("newversion");

  ob.pop(&android);
  ob.setstate("lastversion");
  return 0;
}



#endif


