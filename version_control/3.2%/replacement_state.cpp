//v2
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <map>
#include <iostream>

using namespace std;

#include "replacement_state.h"
//line342:if(samplerSet[way].VALID&&samplerSet[way].TAG==TAG)//TAG TAG BIT

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is distributed as part of the Cache Replacement Championship     //
// workshop held in conjunction with ISCA'2010.                               //
//                                                                            //
//                                                                            //
// Everyone is granted permission to copy, modify, and/or re-distribute       //
// this software.                                                             //
//                                                                            //
// Please contact Aamer Jaleel <ajaleel@gmail.com> should you have any        //
// questions                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

/*
** This file implements the cache replacement state. Users can enhance the code
** below to develop their cache replacement ideas.
**
*/


////////////////////////////////////////////////////////////////////////////////
// The replacement state constructor:                                         //
// Inputs: number of sets, associativity, and replacement policy to use       //
// Outputs: None                                                              //
//                                                                            //
// DO NOT CHANGE THE CONSTRUCTOR PROTOTYPE                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
CACHE_REPLACEMENT_STATE::CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol )
{

    numsets    = _sets;
    assoc      = _assoc;
    replPolicy = _pol;
    assocSampler=assoc;
    mytimer    = 0;

    InitReplacementState();
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// The function prints the statistics for the cache                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
ostream & CACHE_REPLACEMENT_STATE::PrintStats(ostream &out)
{

    out<<"=========================================================="<<endl;
    out<<"=========== Replacement Policy Statistics ================"<<endl;
    out<<"=========================================================="<<endl;

    // CONTESTANTS:  Insert your statistics printing here
    
    return out;

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function initializes the replacement policy hardware by creating      //
// storage for the replacement state on a per-line/per-cache basis.           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void CACHE_REPLACEMENT_STATE::InitReplacementState()
{
    // Create the state for sets, then create the state for the ways

    repl  = new LINE_REPLACEMENT_STATE* [ numsets ];

    // ensure that we were able to create replacement state

    assert(repl);

    // Create the state for the sets
    for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
    {
        repl[ setIndex ]  = new LINE_REPLACEMENT_STATE[ assoc ];

        for(UINT32 way=0; way<assoc; way++) 
        {
            // initialize stack position (for true LRU)
            repl[ setIndex ][ way ].LRUstackposition = way;
			repl[ setIndex ][ way ].DEADFLAG=false;
        }
    }

    if (replPolicy != CRC_REPL_CONTESTANT) return;//hao
     
    // Contestants:  ADD INITIALIZATION FOR YOUR HARDWARE HERE
	
	 prediction=new PREDICTOR [1<<TRACE_LEN];
	//prediction  = new PREDICTOR* [ 2>>TRACE_LEN ];
	assert(prediction);
	for(UINT32 tableIndex=0; tableIndex<(1<<TRACE_LEN); tableIndex++) 
    {
        prediction[ tableIndex ].COUNTER  = 0;

    }
	sampler=new SAMPLER* [UINT32(round(numsets/SETPERSAMPLE)+1)];
	//ERROR HERE
	//assocSampler=assoc-2;
	 for(UINT32 setIndex=0; setIndex<UINT32(round(numsets/SETPERSAMPLE)+1); setIndex++) 
    {
        sampler[ setIndex ]  = new SAMPLER[ assocSampler ];

        for(UINT32 way=0; way<assocSampler; way++) 
        {
            // initialize stack position (for true LRU)
            sampler[ setIndex ][ way ].LRUstackposition = way;
			sampler[ setIndex ][ way ].DEADFLAG=false;
			sampler[ setIndex ][ way ].VALID=false;
			sampler[ setIndex ][ way ].TRACE=0;
        }
    }

	assert(sampler);
	
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache on every cache miss. The input        //
// argument is the set index. The return value is the physical way            //
// index for the line being replaced.                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType ) {
    // If no invalid lines, then replace based on replacement policy
    if( replPolicy == CRC_REPL_LRU ) 
    {
        return Get_LRU_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        return Get_Random_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )//hao
    {
        // Contestants:  ADD YOUR VICTIM SELECTION FUNCTION HERE
	return Get_My_Victim (setIndex,PC, accessType);
    }

    // We should never here here

    assert(0);
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache after every cache hit/miss            //
// The arguments are: the set index, the physical way of the cache,           //
// the pointer to the physical line (should contestants need access           //
// to information of the line filled or hit upon), the thread id              //
// of the request, the PC of the request, the accesstype, and finall          //
// whether the line was a cachehit or not (cacheHit=true implies hit)         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

//when call update the block is already placed
void CACHE_REPLACEMENT_STATE::UpdateReplacementState( 
    UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
    UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit )
{
	UINT32 TAG=0;
	//way+setindex
	//fprintf (stderr, "ain't I a stinker? %lld\n", get_cycle_count ());
	//fflush (stderr);
    // What replacement policy?
    if( replPolicy == CRC_REPL_LRU ) 
    {
        UpdateLRU( setIndex, updateWayID );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        // Random replacement requires no replacement state update
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {
        // Contestants:  ADD YOUR UPDATE REPLACEMENT STATE FUNCTION HERE
        // Feel free to use any of the input parameters to make
        // updates to your replacement policy
		UpdateMyPolicy( setIndex, updateWayID ,cacheHit,PC, currLine,  accessType);
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//////// HELPER FUNCTIONS FOR REPLACEMENT UPDATE AND VICTIM SELECTION //////////
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the LRU victim in the cache set by returning the       //
// cache block at the bottom of the LRU stack. Top of LRU stack is '0'        //
// while bottom of LRU stack is 'assoc-1'                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_LRU_Victim( UINT32 setIndex )
{
	// Get pointer to replacement state of current set

	LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];
	INT32   lruWay   = 0;

	// Search for victim whose stack position is assoc-1

	for(UINT32 way=0; way<assoc; way++) {
		if (replSet[way].LRUstackposition == (assoc-1)) {
			lruWay = way;
			break;
		}
	}

	// return lru way

	return lruWay;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds a random victim in the cache set                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_Random_Victim( UINT32 setIndex )
{
    INT32 way = (rand() % assoc);
    
    return way;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the LRU update routine for the traditional        //
// LRU replacement policy. The arguments to the function are the physical     //
// way and set index.                                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void CACHE_REPLACEMENT_STATE::UpdateLRU( UINT32 setIndex, INT32 updateWayID )
{
	// Determine current LRU stack position
	UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;

	// Update the stack position of all lines before the current line
	// Update implies incremeting their stack positions by one

	for(UINT32 way=0; way<assoc; way++) {
		if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) {
			repl[setIndex][way].LRUstackposition++;
		}
	}

	// Set the LRU stack position of new line to be zero
	repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
}

INT32 CACHE_REPLACEMENT_STATE::Get_My_Victim( UINT32 setIndex,Addr_t PC , UINT32 accessType) {
	// call when miss
	INT32 deadWay=0;
	//if(setIndex%SETPERSAMPLE==0)
		
	UINT32 uselru=1;
		
	
	if((prediction[PC&((1<<TRACE_LEN)-1)].COUNTER>DEADCOUNTER)|| accessType == ACCESS_WRITEBACK||accessType ==ACCESS_PREFETCH)
	//	        if(GetPrediction(PC, 8) || accessType == ACCESS_WRITEBACK)
	    return -1;
	
		
		LINE_REPLACEMENT_STATE *replSet =repl[setIndex];
	    for(UINT32 way=0; way<assoc; way++) 
		{//get deadblock
		    if(replSet[way].DEADFLAG) {
				//cout << "victim dead" << endl;
				uselru=0;
			    deadWay = way;
			    break;
		    }
	    }
		
		/*if(uselru!=0)
		{//get LRU Victim
		  for(UINT32 way=0; way<assoc; way++) 
		  { 
		    if (replSet[way].LRUstackposition == (assoc-1)) {
			    deadWay = way;
			     break;
		        }
		  }
	    }*/	
		if(uselru!=0){
		  deadWay=Get_LRU_Victim(setIndex );
		  //cout << "victim LRU" << endl;
		}
	
		
	return deadWay;
}

void CACHE_REPLACEMENT_STATE::UpdateMyPolicy( UINT32 setIndex, INT32 updateWayID,bool cacheHit,Addr_t PC , const LINE_STATE *currLine, UINT32 accessType) {
	// call when  hit and miss
	
	UINT32 TEMP=PC&((1<<TRACE_LEN)-1);
	Addr_t TAG=currLine[updateWayID].tag;

	if((accessType == ACCESS_WRITEBACK)||(accessType == ACCESS_PREFETCH))//bypass://write back and prefetch
        return; 
	//if(cacheHit)
	//{
	if((prediction[PC&((1<<TRACE_LEN)-1)].COUNTER)>DEADCOUNTER){
			repl[setIndex][updateWayID].DEADFLAG=true;	
			//printf("find a dead pc%d\n",PC&((1<<TRACE_LEN)-1));
		}
	else
			repl[setIndex][updateWayID].DEADFLAG=false;
	//}
	//?else if(!cacheHit)
	//	repl[setIndex][updateWayID].DEADFLAG = 0;
	
	UpdateLRU(setIndex,updateWayID );	
	
	
	if(setIndex%SETPERSAMPLE==0){
	
	ACCESS_SAMPLER(setIndex,updateWayID,cacheHit,PC,TAG);
	}
}

CACHE_REPLACEMENT_STATE::~CACHE_REPLACEMENT_STATE (void) {
}

INT32 CACHE_REPLACEMENT_STATE::ACCESS_SAMPLER(UINT32 setIndex, INT32 updateWayID, bool cacheHit,Addr_t PC, Addr_t TAG) {
	
	INT32  lruWay   = 0;
	INT32  deadWay = 0;
	UINT32  useLRU=1;
	UINT32  usedead=1;
	
	SAMPLER *samplerSet =sampler[UINT32(setIndex/SETPERSAMPLE)];
	//printf("sampler is%d\n",setIndex);
	//printf("sampler Set is%d\n",UINT32(setIndex/SETPERSAMPLE));
	UINT32  replaceWay=0;
	UINT32  replaceWay3=0;
	TAG&=((1<<TRACE_LEN)-1);
	assert(TAG < (1<<TRACE_LEN));
	UINT32 v=0;
	UINT32 currLRUstackposition=0;
	for(UINT32 way=0;way<assocSampler;++way)//??
	{
		//
		//if(samplerSet[way].TAG==TAG)//i am wrong here
		if((samplerSet[way].VALID)&&(samplerSet[way].TAG==TAG))
		{//hit decrease vounter
			//UPDATE predictor
			replaceWay=way;
			//deadWay=way;
			//printf("Hit of tag in sampler,tag is is %d\n",TAG);
			//if(samplerSet[way].TRACE!=PC&((1<<TRACE_LEN)-1)){
			//if(prediction[samplerSet[way].TRACE&((1<<TRACE_LEN)-1)].COUNTER>0)
			//	prediction[samplerSet[way].TRACE&((1<<TRACE_LEN)-1)].COUNTER--;
			if(prediction[PC&((1<<TRACE_LEN)-1)].COUNTER>0)
				prediction[PC&((1<<TRACE_LEN)-1)].COUNTER--;
			//printf("last entry is%d, Prediction counter of this trace is %d\n",samplerSet[way].TRACE&((1<<TRACE_LEN)-1),prediction[samplerSet[way].TRACE&((1<<TRACE_LEN)-1)].COUNTER);
			//}//get prediction
			//?if(prediction[PC&((1<<TRACE_LEN)-1)].COUNTER>DEADCOUNTER)
			//	samplerSet[way].DEADFLAG = true;
		    //?else
			samplerSet[way].DEADFLAG = false;
			
			samplerSet[way].TRACE = PC&((1<<TRACE_LEN)-1);
			
			
			//Move to msu
			currLRUstackposition = samplerSet[way].LRUstackposition;
			for(UINT32 i=0; i<assocSampler; i++) {
				if( samplerSet[i].LRUstackposition < currLRUstackposition ) {
					samplerSet[i].LRUstackposition++;
				}
			}
			samplerSet[way].LRUstackposition = 0;
			samplerSet[way].VALID=true;
			//printf("Hit of tag in sampler,tag is is %d\n",TAG);
			//printf("PC is%d, predictor counter is%d\n",PC&((1<<TRACE_LEN)-1),prediction[PC&((1<<TRACE_LEN)-1)].COUNTER);
			//**printf("MISS shoud not happen\n");
			return 1;
			//**printf("wrong if u see this");
		}
	
	}

	//miss
	//printf("MISS happed\n");
		//prediction[samplerSet[way].TRACE&((1<<TRACE_LEN)-1)].COUNTER++;
		//LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];
		//SAMPLER *samplerSet =sampler[UINT32(setIndex/SETPERSAMPLE)];
	for( v = 0; v < assocSampler; ++v){
	    if(samplerSet[v].VALID == false)
		{   replaceWay3=v;
	        useLRU=0;
			usedead=0;
			//printf("Find INVALID way is %d\n",v);
			//if((samplerSet[v].TAG==0)&&(samplerSet[v].TRACE==0))
			//	printf("THIS is empty %d\n",samplerSet[v].TAG);
			break;
	    }
     }
	if(usedead!=0){
     
    //sprintf("no INVALID way \n");
	
	for(UINT32 way=0; way<assocSampler; way++) 
		{//replace new plock with dead block
		    if (samplerSet[way].DEADFLAG == true) {
			    deadWay = way;
				replaceWay3=way;
				useLRU=0;
				//printf("Find dead way, which is%d\n",deadWay);
			    break;
		    }
	    }
	}
	if(useLRU!=0)
		{  //no dead block ,replace new block by lru policy
	       //MOVE to MRU
		   
			//UINT32 currLRUstackposition = samplerSet[deadWay].LRUstackposition;
			for(UINT32 i=0; i<assocSampler; i++) {
				if (samplerSet[i].LRUstackposition == (assocSampler-1)) {
					lruWay = i;
					break;
				}
			}
			//printf("No dead way, replace by LRUway %d\n",lruWay);
			//miss here//UpdatePredictionTable
			// use the evicted block signature (partial PC) to increase the table
			if(prediction[(samplerSet[lruWay].TRACE)].COUNTER<SATURATECONTER)
				prediction[(samplerSet[lruWay].TRACE)].COUNTER++;
			
			//if(prediction[PC&((1<<TRACE_LEN)-1)].COUNTER<SATURATECONTER)
			//	prediction[PC&((1<<TRACE_LEN)-1)].COUNTER++;
			//printf("last entry is%d, Prediction counter of this trace is %d\n",samplerSet[lruWay].TRACE,prediction[samplerSet[lruWay].TRACE].COUNTER);
			
			//MoveToMRU
			currLRUstackposition = samplerSet[lruWay].LRUstackposition;
			for(UINT32 way=0; way<assocSampler; way++) {
				if( samplerSet[way].LRUstackposition < currLRUstackposition ) {
					samplerSet[way].LRUstackposition++;
					}
			}
			samplerSet[lruWay].LRUstackposition=0;
			
			//samplerSet[lruWay].LRUstackposition = 0;
			samplerSet[lruWay].TAG = TAG&((1<<TRACE_LEN)-1);
			//samplerSet[lruWay].TAG = TAG;
			samplerSet[lruWay].TRACE = PC&((1<<TRACE_LEN)-1);
			samplerSet[lruWay].VALID = true;
			//samplerSet.blocks[0].DEADFLAG = GetPrediction(PC);
			if(prediction[PC&((1<<TRACE_LEN)-1)].COUNTER>DEADCOUNTER)
			{
				samplerSet[lruWay].DEADFLAG = true;
				
			}
		    else
				samplerSet[lruWay].DEADFLAG = false;
			//printf("PC is%d, predictor counter is%d\n",PC&((1<<TRACE_LEN)-1),prediction[PC&((1<<TRACE_LEN)-1)].COUNTER);
	    }
		else
		{//update for dead
	        //printf(",the replaceway i update is %d\n",replaceWay3);
			assert(replaceWay3<assocSampler );
			if(samplerSet[replaceWay3].VALID){
			if(prediction[samplerSet[replaceWay3].TRACE&((1<<TRACE_LEN)-1)].COUNTER<SATURATECONTER)
			  prediction[samplerSet[replaceWay3].TRACE&((1<<TRACE_LEN)-1)].COUNTER++;

		    //if(prediction[PC&((1<<TRACE_LEN)-1)].COUNTER<SATURATECONTER)
			//  prediction[PC&((1<<TRACE_LEN)-1)].COUNTER++;
		  //printf("entry is%d, predictor counter is%d\n",samplerSet[replaceWay3].TRACE,prediction[samplerSet[replaceWay3].TRACE&((1<<TRACE_LEN)-1)].COUNTER);
		    }
			//MoveToMRU
			currLRUstackposition = samplerSet[replaceWay3].LRUstackposition;
			for(UINT32 way=0; way<assocSampler; way++) {
			if( samplerSet[way].LRUstackposition < currLRUstackposition ) {
				samplerSet[way].LRUstackposition++;
				}
			}
			samplerSet[replaceWay3].LRUstackposition=0;
			//PlaceSamplerSet(samplerSet, i, tag, PC);
			//samplerSet[deadWay].LRUstackposition = 0;
			//printf("fill set:%d, way:%d. \n",UINT32(setIndex/SETPERSAMPLE),replaceWay3);
			samplerSet[replaceWay3].TAG = TAG&((1<<TRACE_LEN)-1);
			//samplerSet[replaceWay3].TAG = TAG;
			samplerSet[replaceWay3].TRACE = PC&((1<<TRACE_LEN)-1);
			samplerSet[replaceWay3].VALID = true;
			if(prediction[PC&((1<<TRACE_LEN)-1)].COUNTER>DEADCOUNTER)
			//	samplerSet[replaceWay3].DEADFLAG = true;
		    //?else
				samplerSet[replaceWay3].DEADFLAG = false;
			//samplerSet.blocks[0].PC = fullPC;
			//printf("PC is%d, predictor counter is%d\n",PC&((1<<TRACE_LEN)-1),prediction[PC&((1<<TRACE_LEN)-1)].COUNTER);
		}
	

	return 1;
}

void CACHE_REPLACEMENT_STATE::MoveToMRU(SAMPLER *samplerSet, UINT32 way)
{
		/*for(UINT32 i=0; i<assocSampler; i++) {
				if (samplerSet[i].LRUstackposition == (assocSampler-1)) {
					lruWay = i;
					break;
				}
			}*/
	
	
		UINT32 currLRUstackposition = samplerSet[way].LRUstackposition;
		for(UINT32 wayi=0; wayi<assocSampler; wayi++) {
			if( samplerSet[wayi].LRUstackposition < currLRUstackposition ) {
				samplerSet[wayi].LRUstackposition++;
			}
		}
		samplerSet[way].LRUstackposition = 0;

}
