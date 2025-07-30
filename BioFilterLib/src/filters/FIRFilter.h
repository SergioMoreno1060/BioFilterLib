#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <Arduino.h>
#include <arm_math.h>  // CMSIS-DSP

class FIRFilter {
public:
    /**
     * @brief Constructor de la clase FIRFilter
     * @param coeffs Puntero al array de coeficientes
     * @param numTaps Número de coeficientes (orden del filtro)
     * @param blockSize Tamaño del bloque de procesamiento (puede ser 1 para tiempo real)
     */
    FIRFilter(const float32_t* coeffs, uint16_t numTaps, uint16_t blockSize);

    /**
     * @brief Destructor
     */
    ~FIRFilter();

    /**
     * @brief Aplica el filtro a una muestra individual (modo tiempo real)
     * @param input Valor de entrada
     * @return Valor filtrado
     */
    float32_t processSample(float32_t input);

    /**
     * @brief Aplica el filtro a un bloque de muestras
     * @param inputArray Array de entrada
     * @param outputArray Array de salida (misma longitud)
     * @param length Número de muestras
     */
    void processBuffer(const float32_t* inputArray, float32_t* outputArray, uint32_t length);

private:
    const float32_t* _coeffs;
    float32_t* _state;
    uint16_t _numTaps;
    uint16_t _blockSize;
    arm_fir_instance_f32 _firInstance;
    uint32_t _sampleIndex;  // Para tiempo real
};

#endif // FIR_FILTER_H
