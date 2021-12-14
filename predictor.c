//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
// gshare method:history list and predict answer
uint8_t gshareCache[8*1024];
int histroy;
int digitLimit;
int index;
// tournament method:global history to predictions, local pc to local history
// local history to predictions, global history to choice, global history, local history
int localHistoryCache[1024];
uint8_t localPredicCache[1024];
uint8_t globalCache[512];
uint8_t chooserCache[512];
int globalHistory;
int localHistory;
int localpclimit;
int localhislimit;
int globlimit;
int pcindex;
int lhindex;
uint8_t prelc;
uint8_t pregb;
int temp;
int prelccorrect;
int pregbcorrect;
// custom method (perceptron):
int xHistory;
int xHistoryLimit;
#define xHistoryBits 32
#define numberPerceptron 248
int weights[numberPerceptron][xHistoryBits+1];
int curPerceptron;
int y;
int theta = 1.93 * xHistoryBits + 14;
#define MAXVAL 127
#define MINVAL -128
uint8_t prediction;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//
// help function
int
help_limit(int bits)
{
  int temp = 1;
  for (int i=0; i<bits; i++){
    temp *= 2;
  }
  temp -= 1;
  return temp;
}

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  switch (bpType) {
    case STATIC:
      return;

    case GSHARE:
      // gshare initial all history bits with n and cache with wn
      for (int i=0; i<8*1024; i++){
        gshareCache[i] = WN;
      }
      histroy = 0;
      digitLimit = help_limit(ghistoryBits);
      return;

    case TOURNAMENT:
      for (int i=0; i<1024; i++){
        localHistoryCache[i] = 0;
        localPredicCache[i] = WN;
      }
      for (int i=0; i<512; i++){
        globalCache[i] = WN;
        chooserCache[i] = 1;
      }
      // localHistory = 0;
      globalHistory = 0;
      localpclimit = help_limit(pcIndexBits);
      localhislimit = help_limit(lhistoryBits);
      globlimit = help_limit(ghistoryBits);
      return;
    
    case CUSTOM:
      xHistory = 0;
      xHistoryLimit = help_limit(xHistoryBits);
      for (int i=0; i<numberPerceptron; i++) {
        for (int j=0; j<=xHistoryBits; j++) {
          weights[i][j] = 0;
        }
      }
      return;
    
    default:
      break;
  }
  return;

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;

    case GSHARE:
      index = (pc^histroy)&digitLimit;
      if (gshareCache[index] >= 2){
        return TAKEN;
      }
        
    case TOURNAMENT:
      // local prediction
      pcindex = pc&localpclimit;
      lhindex = localHistoryCache[pcindex];
      prelc = NOTTAKEN;
      if (localPredicCache[lhindex] >= 2){
        prelc = TAKEN;
      }
      // global prediction
      pregb = NOTTAKEN;
      if (globalCache[globalHistory] >= 2){
        pregb = TAKEN;
      }
      // chooser
      if (chooserCache[globalHistory] >= 2){
        return prelc;
      }
      else{
        return pregb;
      }

    case CUSTOM:
      curPerceptron = pc % numberPerceptron;
      y = weights[curPerceptron][0];
      int j = 1;
      int x;
      for (int i=1; i<=xHistoryBits; i++) {
        if ((xHistory & j) == 0)
          x = -1;
        else
          x = 1;
        
        y += weights[curPerceptron][i] * x;
        j = j << 1;
      }

      if (y >= 0)
        prediction = TAKEN;
      else
        prediction = NOTTAKEN;
      
      return prediction;
      
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  // gshare: update cache and history
  switch (bpType) {
    case STATIC:
      return;

    case GSHARE:
      if (outcome == TAKEN){
        if (gshareCache[index] != ST){
          gshareCache[index]++;
        }
        histroy = (histroy*2+1)&digitLimit;
      }
      else{
        if (gshareCache[index] != SN){
          gshareCache[index]--;
        }
        histroy = (histroy*2)&digitLimit;
      }
      return;

    case TOURNAMENT:
      temp = globalHistory;
      if (outcome == TAKEN){
        // update local
        localHistoryCache[pcindex] = (localHistoryCache[pcindex]*2+1)&localhislimit;
        if (localPredicCache[lhindex] != ST){
          localPredicCache[lhindex]++;
        }
        // update global
        if (globalCache[globalHistory] != ST){
          globalCache[globalHistory]++;
        }
        globalHistory = (globalHistory*2+1)&globlimit;
      }
      else{
        // update local
        localHistoryCache[pcindex] = (localHistoryCache[pcindex]*2)&localhislimit;
        if (localPredicCache[lhindex] != SN){
          localPredicCache[lhindex]--;
        }
        // update global
        if (globalCache[globalHistory] != SN){
          globalCache[globalHistory]--;
        }
        globalHistory = (globalHistory*2)&globlimit;
      }
      // update chooser
      prelccorrect = 0;
      pregbcorrect = 0;
      if (prelc == outcome){
        prelccorrect = 1;
      }
      if (pregb == outcome){
        pregbcorrect = 1;
      }
      int reschooser = chooserCache[temp]+prelccorrect-pregbcorrect;
      if (reschooser > 3){
        reschooser = 3;
      }
      if (reschooser < 0){
        reschooser = 0;
      }
      chooserCache[temp] = reschooser;
      return;

    case CUSTOM:
      curPerceptron = pc % numberPerceptron;
      int t = (outcome == 1) ? 1:-1;
      int x;

      if (prediction != outcome || abs(y) <= theta) {
        if (weights[curPerceptron][0] > MINVAL && weights[curPerceptron][0] < MAXVAL) {
            weights[curPerceptron][0] += t; 
        }  

        int j = 1;
        for (int i=1; i<=xHistoryBits; i++) {
          if (weights[curPerceptron][i] > MINVAL && weights[curPerceptron][i] < MAXVAL) {      
            if (( ( (xHistory&j) != 0) && (outcome == 1) ) ||  ( ((xHistory&j) == 0) && (outcome == 0) ))
              x = 1;
            else
              x = -1;
            
            weights[curPerceptron][i] += x;
          }
        
          j = j << 1;
        }
      }
      // update history
      xHistory <<= 1;
      xHistory |= outcome;
      return;
    
    default:
      break;
  }
  return;
}
