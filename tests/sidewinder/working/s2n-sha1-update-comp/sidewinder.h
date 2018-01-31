#pragma once

void __VERIFIER_ASSERT_MAX_LEAKAGE(int max);
void __VERIFIER_ASSUME_LEAKAGE(int assumedLeakage);
void __VERIFIER_LOOP_VAR(int);
void __VERIFIER_ADD_LEAKAGE(int leakage);
void benign();
#define BENIGN benign()
