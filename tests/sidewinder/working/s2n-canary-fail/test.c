/*
 * Copyright 2014 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <sys/param.h>
#include <stdint.h>

#include <smack.h>
#include <smack-contracts.h>
#include "ct-verif.h"
#include "sidewinder.h"


int unbalanced(int pub, int key)
{
  int rval = 0;
  if(key) {
    rval++;
  }

  return rval;
}

int balanced(int pub, int key)
{
  int rval = 0;
  if(key){
    rval ++;
  } else {
    rval--;
  }
  return rval;

}

int test(int pub, int key)
{
  public_in(__SMACK_value(pub));

  return unbalanced(pub,key);
}
