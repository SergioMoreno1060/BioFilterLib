#ifndef UTILS_H
#define UTILS_H

#include <arm_math.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calcula la relación señal/ruido (Signal-to-Noise Ratio).
 * 
 * @param signal Puntero al vector de señal original.
 * @param noise  Puntero al vector de señal con ruido o error.
 * @param length Longitud de los vectores.
 * @return SNR en decibelios (dB).
 */
float32_t calculateSNR(const float32_t* signal, const float32_t* noise, uint32_t length);

/**
 * @brief Calcula el error cuadrático medio (Mean Squared Error).
 * 
 * @param signal1 Primer vector de señal.
 * @param signal2 Segundo vector de señal.
 * @param length  Longitud de los vectores.
 * @return Valor MSE.
 */
float32_t calculateMSE(const float32_t* signal1, const float32_t* signal2, uint32_t length);

/**
 * @brief Calcula el valor cuadrático medio (Root Mean Square).
 * 
 * @param signal Puntero al vector de señal.
 * @param length Longitud del vector.
 * @return Valor RMS.
 */
float32_t calculateRMS(const float32_t* signal, uint32_t length);

/**
 * @brief Calcula la desviación estándar.
 * 
 * @param signal Puntero al vector de señal.
 * @param length Longitud del vector.
 * @return Desviación estándar.
 */
float32_t calculateStdDev(const float32_t* signal, uint32_t length);

/**
 * @brief Calcula el coeficiente de correlación de Pearson entre dos señales.
 * 
 * @param signal1 Primer vector de señal.
 * @param signal2 Segundo vector de señal.
 * @param length  Longitud de los vectores.
 * @return Coeficiente de correlación (-1.0 a 1.0).
 */
float32_t calculateCorrelation(const float32_t* signal1, const float32_t* signal2, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif  // UTILS_H
