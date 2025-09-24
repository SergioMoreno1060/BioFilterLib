#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"

// Parámetros del filtro
#define BLOCK_SIZE  32
#define NUM_TAPS    51
#define SAMPLE_RATE 1000  // Hz típico para ECG
#define CUTOFF_FREQ 50   // Hz - frecuencia de corte para eliminar ruido

// Coeficientes FIR simulados (pasa bajas, por ejemplo)
const float32_t ecgFilterCoeffs[NUM_TAPS] = {
  1.01602337e-03,  1.05219578e-03,  1.05485683e-03,  9.52665359e-04,
  6.39612342e-04, -6.52666866e-19, -1.05692964e-03, -2.55869546e-03,
  -4.43506165e-03, -6.49496992e-03, -8.42139827e-03, -9.78815640e-03,
  -1.00992192e-02, -8.84737843e-03, -5.58538708e-03,  2.65242472e-18,
  8.02209630e-03,  1.83472206e-02,  3.05752787e-02,  4.40532964e-02,
  5.79227190e-02,  7.11964426e-02,  8.28570731e-02,  9.19645190e-02,
  9.77592660e-02,  9.97478615e-02,  9.77592660e-02,  9.19645190e-02,
  8.28570731e-02,  7.11964426e-02,  5.79227190e-02,  4.40532964e-02,
  3.05752787e-02,  1.83472206e-02,  8.02209630e-03,  2.65242472e-18,
  -5.58538708e-03, -8.84737843e-03, -1.00992192e-02, -9.78815640e-03,
  -8.42139827e-03, -6.49496992e-03, -4.43506165e-03, -2.55869546e-03,
  -1.05692964e-03, -6.52666866e-19,  6.39612342e-04,  9.52665359e-04,
  1.05485683e-03,  1.05219578e-03,  1.01602337e-03
};

// Buffers para procesamiento
float32_t inputSignal[maxSamplesNum];
float32_t filteredSignal[maxSamplesNum];

// Instancia del filtro
FIRFilter* ecgFilter;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("=== BioFilterLib ECG Filtering Demo ===");
    Serial.println("Filtering noisy ECG signal with FIR low-pass filter");
    Serial.println("Filter specs: 51 taps, fc=40Hz, fs=250Hz");
    Serial.println();
    
    // Convertir señal ECG de uint16_t a float32_t y normalizar
    Serial.println("Converting and normalizing ECG signal...");
    for (int i = 0; i < maxSamplesNum; i++) {
        // Normalizar de rango 0-4095 (12-bit) a rango ±1.0
        inputSignal[i] = ((float32_t)waveformsTable[4][i] - 2048.0f) / 2048.0f;
    }
    
    // Inicializar filtro FIR
    Serial.println("Initializing FIR filter...");
    ecgFilter = new FIRFilter(ecgFilterCoeffs, NUM_TAPS, BLOCK_SIZE);
    
    // Procesar señal por bloques
    Serial.println("Processing signal...");
    uint32_t samplesProcessed = 0;
    
    while (samplesProcessed < maxSamplesNum) {
        uint32_t currentBlockSize = min((uint32_t)BLOCK_SIZE, maxSamplesNum - samplesProcessed);
        ecgFilter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredSignal[samplesProcessed], 
                                 currentBlockSize);
        samplesProcessed += currentBlockSize;
    }
    
    Serial.println("Filtering complete!");
    Serial.println();
    
    // Mostrar header para CSV
    Serial.println("Sample,Original,Filtered");
    
    // Mostrar datos en formato CSV
    for (int i = 0; i < maxSamplesNum; i++) {
        Serial.print(i);
        Serial.print(",");
        Serial.print(inputSignal[i], 6);
        Serial.print(",");
        Serial.println(filteredSignal[i], 6);
        
        // Pequeña pausa para evitar overflow del buffer serial
        if (i % 50 == 0) {
            delay(10);
        }
    }
    
    Serial.println();
    Serial.println("=== Data transmission complete ===");
    Serial.println("You can now save this data to CSV using:");
    Serial.println("python -c \"import serial; s=serial.Serial('COM6',115200); [print(s.readline().decode().strip()) for _ in range(1010)]\" > ecg_data.csv");
}

void loop() {
    // Demostración en tiempo real (opcional)
    static unsigned long lastTime = 0;
    static int sampleIndex = 0;
    
    if (millis() - lastTime > 1) {  // Simular fs=1000Hz (1ms entre muestras)
        if (sampleIndex < maxSamplesNum) {
            float32_t currentSample = inputSignal[sampleIndex];
            float32_t filteredSample = ecgFilter->processSample(currentSample);
            
            // Mostrar muestra en tiempo real (comentar si no se necesita)
            /*
            Serial.print("RT: ");
            Serial.print(sampleIndex);
            Serial.print(",");
            Serial.print(currentSample, 4);
            Serial.print(",");
            Serial.println(filteredSample, 4);
            */
            
            sampleIndex++;
            if (sampleIndex >= maxSamplesNum) {
                sampleIndex = 0;  // Reiniciar para loop continuo
            }
        }
        lastTime = millis();
    }
}
