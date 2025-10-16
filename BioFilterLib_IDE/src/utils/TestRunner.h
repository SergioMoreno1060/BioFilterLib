/**
 * @file TestRunner.h
 * @brief Sistema simple para probar filtros automáticamente
 * @author Sergio
 * @version 2.0.0
 * @date 2025
 * 
 * @example
 * #include "TestRunner.h"
 * TestRunner test;
 * test.testFIR(myFilter, "ecg_60hz_noised");
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "arm_math.h"
#include "utils/Waveforms.h"

// Forward declarations
class FIRFilter;
class IIRFilter;
class LMSFilter;
class WaveletFilter;

/**
 * @brief Clase para hacer pruebas de filtros
 */
class TestRunner {
private:
    float32_t input[maxSamplesNum];
    float32_t output[maxSamplesNum];
    float32_t reference[maxSamplesNum];
    
    // Calcular SNR (relación señal/ruido)
    float32_t calculateSNR(const float32_t* signal, const float32_t* ref) {
        float32_t signalPower = 0.0f;
        float32_t noisePower = 0.0f;
        
        for (int i = 0; i < maxSamplesNum; i++) {
            signalPower += ref[i] * ref[i];
            float32_t noise = signal[i] - ref[i];
            noisePower += noise * noise;
        }
        
        if (noisePower < 1e-10f) return 100.0f;
        return 10.0f * log10(signalPower / noisePower);
    }
    
public:
    /**
     * @brief Prueba un filtro FIR con una señal
     * 
     * @param filter Puntero al filtro FIR
     * @param signalTag Señal a probar: "ecg_60hz_noised", "ecg_white_noised", etc.
     * @param referenceTag Señal de referencia (default: "ecg_clean")
     * @return true si pasó la prueba (mejora > 3dB)
     * 
     * @example
     * FIRFilter* filter = new FIRFilter(coeffs, 51, 32);
     * test.testFIR(filter, "ecg_60hz_noised");
     */
    bool testFIR(FIRFilter* filter, const char* signalTag, const char* referenceTag = "ecg_clean") {
        if (!loadSignal(signalTag, input) || !loadSignal(referenceTag, reference)) {
            Serial.println("Error cargando señales");
            return false;
        }
        
        float32_t snrBefore = calculateSNR(input, reference);
        filter->processBuffer(input, output, maxSamplesNum);
        float32_t snrAfter = calculateSNR(output, reference);
        float32_t improvement = snrAfter - snrBefore;
        
        Serial.print("Señal: ");
        Serial.println(getSignalName(signalTag));
        Serial.print("  SNR antes:  ");
        Serial.print(snrBefore, 2);
        Serial.println(" dB");
        Serial.print("  SNR después: ");
        Serial.print(snrAfter, 2);
        Serial.println(" dB");
        Serial.print("  Mejora:     ");
        Serial.print(improvement, 2);
        Serial.println(" dB");
        Serial.print("  Estado:     ");
        Serial.println(improvement > 3.0f ? "✓ PASÓ" : "✗ FALLÓ");
        Serial.println();
        
        return improvement > 3.0f;
    }
    
    /**
     * @brief Prueba un filtro IIR con una señal
     * 
     * @param filter Puntero al filtro IIR
     * @param signalTag Señal a probar
     * @param referenceTag Señal de referencia (default: "ecg_clean")
     * @return true si pasó la prueba (mejora > 3dB)
     */
    bool testIIR(IIRFilter* filter, const char* signalTag, const char* referenceTag = "ecg_clean") {
        if (!loadSignal(signalTag, input) || !loadSignal(referenceTag, reference)) {
            Serial.println("Error cargando señales");
            return false;
        }
        
        float32_t snrBefore = calculateSNR(input, reference);
        filter->processBuffer(input, output, maxSamplesNum);
        float32_t snrAfter = calculateSNR(output, reference);
        float32_t improvement = snrAfter - snrBefore;
        
        Serial.print("Señal: ");
        Serial.println(getSignalName(signalTag));
        Serial.print("  SNR antes:  ");
        Serial.print(snrBefore, 2);
        Serial.println(" dB");
        Serial.print("  SNR después: ");
        Serial.print(snrAfter, 2);
        Serial.println(" dB");
        Serial.print("  Mejora:     ");
        Serial.print(improvement, 2);
        Serial.println(" dB");
        Serial.print("  Estado:     ");
        Serial.println(improvement > 3.0f ? "✓ PASÓ" : "✗ FALLÓ");
        Serial.println();
        
        return improvement > 3.0f;
    }
    
    /**
     * @brief Prueba un filtro LMS adaptativo con una señal
     * 
     * @param filter Puntero al filtro LMS
     * @param signalTag Señal a probar
     * @param referenceSignal Señal de referencia para adaptación (ej: 60Hz puro)
     * @param cleanTag Señal limpia para comparar (default: "ecg_clean")
     * @return true si pasó la prueba (mejora > 3dB)
     * 
     * @note Para LMS, referenceSignal es la señal correlacionada con el ruido
     */
    bool testLMS(LMSFilter* filter, const char* signalTag, const float32_t* referenceSignal, 
                 const char* cleanTag = "ecg_clean") {
        if (!loadSignal(signalTag, input) || !loadSignal(cleanTag, reference)) {
            Serial.println("Error cargando señales");
            return false;
        }
        
        float32_t snrBefore = calculateSNR(input, reference);
        
        // Procesar con LMS usando señal de referencia
        // LMS requiere 4 parámetros: input, reference, output*, error*
        float32_t errorSignal[maxSamplesNum];
        for (int i = 0; i < maxSamplesNum; i++) {
            filter->processSample(input[i], referenceSignal[i], &output[i], &errorSignal[i]);
        }
        
        float32_t snrAfter = calculateSNR(output, reference);
        float32_t improvement = snrAfter - snrBefore;
        
        Serial.print("Señal: ");
        Serial.println(getSignalName(signalTag));
        Serial.print("  SNR antes:  ");
        Serial.print(snrBefore, 2);
        Serial.println(" dB");
        Serial.print("  SNR después: ");
        Serial.print(snrAfter, 2);
        Serial.println(" dB");
        Serial.print("  Mejora:     ");
        Serial.print(improvement, 2);
        Serial.println(" dB");
        Serial.print("  Estado:     ");
        Serial.println(improvement > 3.0f ? "✓ PASÓ" : "✗ FALLÓ");
        Serial.println();
        
        return improvement > 3.0f;
    }
    
    /**
     * @brief Prueba un filtro Wavelet con una señal (usando solo aproximación)
     * 
     * @param filter Puntero al filtro Wavelet
     * @param signalTag Señal a probar
     * @param referenceTag Señal de referencia (default: "ecg_clean")
     * @return true si pasó la prueba (mejora > 3dB)
     * 
     * @note El test usa solo los coeficientes de aproximación para denoising
     */
    bool testWavelet(WaveletFilter* filter, const char* signalTag, const char* referenceTag = "ecg_clean") {
        if (!loadSignal(signalTag, input) || !loadSignal(referenceTag, reference)) {
            Serial.println("Error cargando señales");
            return false;
        }
        
        float32_t snrBefore = calculateSNR(input, reference);
        
        // Procesar obteniendo solo aproximación (denoising)
        float32_t approxCoeffs[maxSamplesNum];
        float32_t detailCoeffs[maxSamplesNum];
        
        filter->processBuffer(input, approxCoeffs, detailCoeffs, maxSamplesNum);
        
        // Reconstruir solo con aproximación (supprime ruido de alta frecuencia)
        for (int i = 0; i < maxSamplesNum; i++) {
            output[i] = filter->reconstruct(approxCoeffs[i], 0.0f);
        }
        
        float32_t snrAfter = calculateSNR(output, reference);
        float32_t improvement = snrAfter - snrBefore;
        
        Serial.print("Señal: ");
        Serial.println(getSignalName(signalTag));
        Serial.print("  SNR antes:  ");
        Serial.print(snrBefore, 2);
        Serial.println(" dB");
        Serial.print("  SNR después: ");
        Serial.print(snrAfter, 2);
        Serial.println(" dB");
        Serial.print("  Mejora:     ");
        Serial.print(improvement, 2);
        Serial.println(" dB");
        Serial.print("  Estado:     ");
        Serial.println(improvement > 3.0f ? "✓ PASÓ" : "✗ FALLÓ");
        Serial.println();
        
        return improvement > 3.0f;
    }
    
    /**
     * @brief Prueba todas las señales con un filtro FIR
     */
    void testAllFIR(FIRFilter* filter) {
        Serial.println("========================================");
        Serial.println("  PRUEBAS AUTOMÁTICAS - FILTRO FIR");
        Serial.println("========================================");
        Serial.println();
        
        const char* signals[] = {"ecg_60hz_noised", "ecg_white_noised", "ecg_60hz_noised_fs240"};
        int passed = 0;
        
        for (int i = 0; i < 3; i++) {
            if (testFIR(filter, signals[i])) {
                passed++;
            }
        }
        
        Serial.println("========================================");
        Serial.print("Resultado: ");
        Serial.print(passed);
        Serial.print(" de 3 pruebas pasadas (");
        Serial.print((passed * 100) / 3);
        Serial.println("%)");
        Serial.println("========================================");
    }
    
    /**
     * @brief Prueba todas las señales con un filtro IIR
     */
    void testAllIIR(IIRFilter* filter) {
        Serial.println("========================================");
        Serial.println("  PRUEBAS AUTOMÁTICAS - FILTRO IIR");
        Serial.println("========================================");
        Serial.println();
        
        const char* signals[] = {"ecg_60hz_noised", "ecg_white_noised", "ecg_60hz_noised_fs240"};
        int passed = 0;
        
        for (int i = 0; i < 3; i++) {
            if (testIIR(filter, signals[i])) {
                passed++;
            }
        }
        
        Serial.println("========================================");
        Serial.print("Resultado: ");
        Serial.print(passed);
        Serial.print(" de 3 pruebas pasadas (");
        Serial.print((passed * 100) / 3);
        Serial.println("%)");
        Serial.println("========================================");
    }
    
    /**
     * @brief Prueba filtro LMS con señal de 60Hz
     */
    void testAllLMS(LMSFilter* filter, const float32_t* refSignal60Hz) {
        Serial.println("========================================");
        Serial.println("  PRUEBAS AUTOMÁTICAS - FILTRO LMS");
        Serial.println("========================================");
        Serial.println();
        
        const char* signals[] = {"ecg_60hz_noised", "ecg_60hz_noised_fs240"};
        int passed = 0;
        
        for (int i = 0; i < 2; i++) {
            if (testLMS(filter, signals[i], refSignal60Hz)) {
                passed++;
            }
        }
        
        Serial.println("========================================");
        Serial.print("Resultado: ");
        Serial.print(passed);
        Serial.print(" de 2 pruebas pasadas (");
        Serial.print((passed * 100) / 2);
        Serial.println("%)");
        Serial.println("========================================");
    }
    
    /**
     * @brief Prueba todas las señales con un filtro Wavelet
     */
    void testAllWavelet(WaveletFilter* filter) {
        Serial.println("========================================");
        Serial.println("  PRUEBAS AUTOMÁTICAS - FILTRO WAVELET");
        Serial.println("========================================");
        Serial.println();
        
        const char* signals[] = {"ecg_60hz_noised", "ecg_white_noised", "ecg_60hz_noised_fs240"};
        int passed = 0;
        
        for (int i = 0; i < 3; i++) {
            if (testWavelet(filter, signals[i])) {
                passed++;
            }
        }
        
        Serial.println("========================================");
        Serial.print("Resultado: ");
        Serial.print(passed);
        Serial.print(" de 3 pruebas pasadas (");
        Serial.print((passed * 100) / 3);
        Serial.println("%)");
        Serial.println("========================================");
    }
};

#endif // TEST_RUNNER_H