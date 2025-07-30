#include "FIRFilter.h"

FIRFilter::FIRFilter(const float32_t* coeffs, uint16_t numTaps, uint16_t blockSize)
    : _coeffs(coeffs), _numTaps(numTaps), _blockSize(blockSize), _sampleIndex(0)
{
    // El tamaño del estado debe ser numTaps + blockSize - 1
    _state = new float32_t[_numTaps + _blockSize - 1]();
    arm_fir_init_f32(&_firInstance, _numTaps, _coeffs, _state, _blockSize);
}

FIRFilter::~FIRFilter() {
    delete[] _state;
}

float32_t FIRFilter::processSample(float32_t input) {
    float32_t output;
    // Procesar una muestra a la vez (blockSize = 1)
    arm_fir_f32(&_firInstance, &input, &output, 1);
    return output;
}

void FIRFilter::processBuffer(const float32_t* inputArray, float32_t* outputArray, uint32_t length) {
    // Procesar en bloques del tamaño especificado
    arm_fir_f32(&_firInstance, inputArray, outputArray, length);
}
