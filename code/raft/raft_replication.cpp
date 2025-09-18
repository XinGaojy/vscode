

#include<iostream>
using namspace std;
bool appendentries(){
	if(args.term>rf.current_term){
		return false;
	}
	
	//很久没有收到日志，同时判断是否越界
	if(args.prevelogindex > len(rf.log){
		return false;
	}
	//任期不相等	
	if(rf.log[args.prevelogindex].term != args.term){
		return false;
	}

	//
	

	//TO DO if leader comiiteed the local must to comiiteed
	{


	}
	//append to local entry 
	rf.log=append();
	

	resetselecttimer();

}

sendappendentries(){
	args{
		int next_index;
		int term;
		logentry log;
	}
		
	if(reply.term > rf.current_term){
		rf.becomefollower();
	}
	
	if(!reply.success){
		//leader index back
		for(idx > 0 && rf.log[idx].term ==term){

			idx --;

		}
		rf.next_index[peer]=idx+1;
		
		

	}


	{}

	
}

int main(){
	
	return 0;
}
