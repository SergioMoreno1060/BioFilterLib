#include "utils.h"
#include <math.h>  // Para log10f(), sqrtf() si no se usa CMSIS math

/**
 * @brief Calcula la relación señal/ruido (SNR) en decibelios.
 * 
 * Se calcula como 10 * log10(P_signal / P_noise),
 * donde P_signal y P_noise son las potencias promedio de las señales.
 */
float32_t calculateSNR(const float32_t* signal, const float32_t* noise, uint32_t length) {
    float32_t signalPower = 0.0f;
    float32_t noisePower = 0.0f;
    
    for (uint32_t i = 0; i < length; i++) {
        signalPower += signal[i] * signal[i];              // Potencia de la señal
        float32_t error = signal[i] - noise[i];            // Error entre señal y ruido
        noisePower += error * error;                       // Potencia del ruido
    }
    
    signalPower /= length;
    noisePower /= length;
    
    // Evitar división por cero o valores muy pequeños
    if (noisePower < 1e-10f) return 999.9f;
    
    return 10.0f * log10f(signalPower / noisePower);
}

/**
 * @brief Calcula el error cuadrático medio (MSE) entre dos señales.
 */
float32_t calculateMSE(const float32_t* signal1, const float32_t* signal2, uint32_t length) {
    float32_t mse = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        float32_t error = signal1[i] - signal2[i];
        mse += error * error;
    }
    return mse / length;
}

/**
 * @brief Calcula el valor cuadrático medio (RMS) de una señal.
 */
float32_t calculateRMS(const float32_t* signal, uint32_t length) {
    float32_t sum = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        sum += signal[i] * signal[i];
    }
    return sqrtf(sum / length);
}

/**
 * @brief Calcula la desviación estándar de una señal.
 */
float32_t calculateStdDev(const float32_t* signal, uint32_t length) {
    float32_t mean = 0.0f;
    
    // Calcular media
    for (uint32_t i = 0; i < length; i++) {
        mean += signal[i];
    }
    mean /= length;
    
    // Calcular varianza
    float32_t variance = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        float32_t diff = signal[i] - mean;
        variance += diff * diff;
    }
    variance /= length;
    
    return sqrtf(variance);
}

/**
 * @brief Calcula el coeficiente de correlación de Pearson entre dos señales.
 * 
 * Devuelve un valor entre -1 y 1:
 *  -  1 indica correlación positiva perfecta.
 *  - -1 indica correlación negativa perfecta.
 *  -  0 indica ausencia de correlación lineal.
 */
float32_t calculateCorrelation(const float32_t* signal1, const float32_t* signal2, uint32_t length) {
    float32_t mean1 = 0.0f, mean2 = 0.0f;
    
    // Calcular medias
    for (uint32_t i = 0; i < length; i++) {
        mean1 += signal1[i];
        mean2 += signal2[i];
    }
    mean1 /= length;
    mean2 /= length;
    
    // Calcular numerador y denominadores
    float32_t numerator = 0.0f;
    float32_t denom1 = 0.0f, denom2 = 0.0f;
    
    for (uint32_t i = 0; i < length; i++) {
        float32_t diff1 = signal1[i] - mean1;
        float32_t diff2 = signal2[i] - mean2;
        numerator += diff1 * diff2;
        denom1 += diff1 * diff1;
        denom2 += diff2 * diff2;
    }
    
    // Calcular correlación normalizada
    float32_t denominator = sqrtf(denom1 * denom2);
    return (denominator > 1e-10f) ? (numerator / denominator) : 0.0f;
}
