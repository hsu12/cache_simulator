#ifndef REPL_STATE_H
#define REPL_STATE_H
 
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

#include <cstdlib>
#include <cassert>
#include "utils.h"
#include "crc_cache_defs.h"
#include <iostream>


using namespace std;

#define TRACE_LEN 15
#define SETPERSAMPLE 64
#define DEADCOUNTER 40
#define SATURATECONTER 60
#define SAMPLER_POLICY 1

// Replacement Policies Supported
typedef enum 
{
    CRC_REPL_LRU        = 0,
    CRC_REPL_RANDOM     = 1,
    CRC_REPL_CONTESTANT = 2
} ReplacemntPolicy;

// Replacement State Per Cache Line
typedef struct
{
    UINT32  LRUstackposition;
	

    UINT32  DEADFLAG;
    // CONTESTANTS: Add extra state per cache line here//hao

} LINE_REPLACEMENT_STATE;

typedef struct
{
//Five
UINT32 TAG;//
Addr_t TRACE;//TRACE==PC,>15BIT
bool DEADFLAG;
UINT32 LRUstackposition;
bool VALID;	
	
}SAMPLER; // Jimenez's structures


typedef struct
{
	UINT32 COUNTER;
	
}PREDICTOR;


// The implementation for the cache replacement policy
class CACHE_REPLACEMENT_STATE
{
public:
    LINE_REPLACEMENT_STATE   **repl;
	SAMPLER   **sampler;
	PREDICTOR   *prediction;
	UINT32     *SAMPLER_status;
  private:

    UINT32 numsets;
    UINT32 assoc;
	UINT32 assocSampler;//
    UINT32 replPolicy;

    COUNTER mytimer;  // tracks # of references to the cache

    // CONTESTANTS:  Add extra state for cache here

  public:
    ostream & PrintStats(ostream &out);

    // The constructor CAN NOT be changed
    CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol );

    INT32 GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType );

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID );

    void   SetReplacementPolicy( UINT32 _pol ) { replPolicy = _pol; } 
    void   IncrementTimer() { mytimer++; } 

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
                                   UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit );

    ~CACHE_REPLACEMENT_STATE(void);

  private:
    
    void   InitReplacementState();
    INT32  Get_Random_Victim( UINT32 setIndex );

    INT32  Get_LRU_Victim( UINT32 setIndex );
    INT32  Get_My_Victim( UINT32 setIndex,Addr_t PC, UINT32 accessType  );
    void   UpdateLRU( UINT32 setIndex, INT32 updateWayID );
    //void   UpdateMyPolicy( UINT32 setIndex, INT32 updateWayID );
	void UpdateMyPolicy( UINT32 setIndex, INT32 updateWayID,bool cacheHit,Addr_t PC , const LINE_STATE *currLine, UINT32 accessType); 
	INT32 ACCESS_SAMPLER(UINT32 setIndex, INT32 updateWayID, bool cacheHit,Addr_t PC, Addr_t TAG);
	
};

#endif
