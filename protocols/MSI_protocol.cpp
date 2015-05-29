#include "MSI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MSI_protocol::MSI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry){
																				// Initialize lines to not have the data yet!
    this->state = MSI_CACHE_I;
}

MSI_protocol::~MSI_protocol (){    
}

void MSI_protocol::dump (void){
    const char *block_states[7] = {"X","I","S","M","IS","IM","SM"};
    fprintf (stderr, "MSI_protocol - state: %s\n", block_states[state]);
}

void MSI_protocol::process_cache_request (Mreq *request){
	switch (state) {
	case MSI_CACHE_I:  do_cache_I (request); break;
    case MSI_CACHE_IM: do_cache_IM (request); break;
    case MSI_CACHE_M:  do_cache_M (request); break;
	case MSI_CACHE_S:  do_cache_S (request); break;
    case MSI_CACHE_IS: do_cache_IS (request); break;
    case MSI_CACHE_SM:  do_cache_SM (request); break;
    default:
        fatal_error ("Invalid Cache State for MSI Protocol\n");
    }
}

void MSI_protocol::process_snoop_request (Mreq *request){
	switch (state) {
	case MSI_CACHE_I:  do_snoop_I (request); break;
    case MSI_CACHE_IM: do_snoop_IM (request); break;
    case MSI_CACHE_M:  do_snoop_M (request); break;
	case MSI_CACHE_S:  do_snoop_S (request); break;
    case MSI_CACHE_IS: do_snoop_IS (request); break;
    case MSI_CACHE_SM:  do_snoop_SM (request); break;
	default:
    	fatal_error ("Invalid Cache State for MSI Protocol\n");
    }
}

inline void MSI_protocol::do_cache_I (Mreq *request){
	switch (request->msg) {
    case LOAD:
		send_GETS(request->addr);
		state = MSI_CACHE_IS;
		Sim->cache_misses++;											//ACTUAL CACHE MISSES
    	break;
    case STORE:
    	send_GETM(request->addr);
    	state = MSI_CACHE_IM;
    	Sim->cache_misses++;											//ACTUAL CACHE MISSES
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_S (Mreq *request){
	switch (request->msg) {
    case LOAD:
		send_DATA_to_proc(request->addr);
		break;
    case STORE:
    	send_GETM(request->addr);
    	state = MSI_CACHE_SM;
    	Sim->cache_misses++;											// USED TO SHOW COHERENCE MISSES
    	break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_M (Mreq *request){
	switch (request->msg){
	case LOAD:
		
	case STORE:
		send_DATA_to_proc(request->addr);
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_IS (Mreq *request){
    switch (request->msg){
	case LOAD:
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_IM (Mreq *request){
    switch (request->msg) {
	case LOAD:
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_cache_SM (Mreq *request){
    switch (request->msg) 
    {
	case LOAD:
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: SM state shouldn't see this message\n");
   	}
}

inline void MSI_protocol::do_snoop_I (Mreq *request){
	switch (request->msg){
	case GETS:
	case GETM:
	case DATA:
		break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_S (Mreq *request){
	switch (request->msg){
	case GETS: 
		
		break;
	case GETM:
		
		state = MSI_CACHE_I;											//INVALIDATES DATA BECAUSE OF GETM
		break;
	case DATA:
		fatal_error ("Should not see data for this line!  I have the line!");
		break;
    default:
        request->print_msg (my_table->moduleID, "ERROR");
        fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MSI_protocol::do_snoop_M (Mreq *request){
	switch (request->msg){
	case GETS:
		
		send_DATA_on_bus(request->addr,request->src_mid);				//SUPPLY THE DATA
		state = MSI_CACHE_S;
		break;
	case GETM:
		
		send_DATA_on_bus(request->addr,request->src_mid);				//SUPPLY THE DATA AND INVALIDATE SELF
		state = MSI_CACHE_I;
		break;
	case DATA:
		fatal_error ("Should not see data for this line!  I have the line!");
		break;
	default: request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: M state shouldn't see this message\n");
	}
}

inline void MSI_protocol::do_snoop_IS (Mreq *request){
	switch (request->msg){
	case GETS:
	case GETM:
		break;
	case DATA:
		send_DATA_to_proc(request->addr);
		state = MSI_CACHE_S;
		
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: IS state shouldn't see this message\n");
	}
}

inline void MSI_protocol::do_snoop_IM (Mreq *request){
	switch (request->msg) {
	case GETS:
	case GETM:
		break;
	case DATA:
		send_DATA_to_proc(request->addr);
		state = MSI_CACHE_M;
	
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: IM state shouldn't see this message\n");
	}
}

inline void MSI_protocol::do_snoop_SM (Mreq *request){
	switch (request->msg){
	case GETS:
	case GETM:
		break;
	case DATA:
		send_DATA_to_proc(request->addr);					//JUST SEND REQUIRED DATA TO PROCESSOR
		state = MSI_CACHE_M;
		
	break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: IM state shouldn't see this message\n");
	}
}




