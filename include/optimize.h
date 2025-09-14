#pragma once

enum BLASImplementation { MKL, MKL3 };

int optimize(BLASImplementation impl, int argc, char* argv[]);
