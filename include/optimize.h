#pragma once

enum BLASImplementation { MKL, MKL3, CUBLAS };

int optimize(BLASImplementation impl, int argc, char* argv[]);
