/**
 * @file Test_Diagnostico.ino
 * @brief Test mínimo para diagnosticar problema de cuelgue
 */

#include <Arduino.h>
#include <arm_math.h>
#include "BioFilterLib.h"

// Incluir solo lo necesario
extern const uint16_t waveformsTable[8][3000];
// const uint16_t maxSamplesNum = 3000;

float32_t testBuffer[100];  // Buffer pequeño para prueba

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== TEST DIAGNOSTICO ===");
    Serial.println("Iniciando...");
    Serial.flush();
    delay(500);
    
    // TEST 1: Verificar acceso a una posición
    Serial.println("\nTEST 1: Acceso a waveformsTable[7][0]");
    Serial.flush();
    delay(100);
    
    uint16_t valor = waveformsTable[7][0];
    Serial.print("Valor leido: 0x");
    Serial.println(valor, HEX);
    Serial.flush();
    delay(100);
    
    // TEST 2: Leer primeras 10 muestras
    Serial.println("\nTEST 2: Leyendo primeras 10 muestras");
    Serial.flush();
    delay(100);
    
    for (int i = 0; i < 10; i++) {
        uint16_t val = waveformsTable[7][i];
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] = 0x");
        Serial.println(val, HEX);
        Serial.flush();
        delay(50);
    }
    
    // TEST 3: Convertir a float
    Serial.println("\nTEST 3: Conversion a float");
    Serial.flush();
    delay(100);
    
    for (int i = 0; i < 5; i++) {
        uint16_t rawValue = waveformsTable[7][i];
        float32_t normalized = ((float32_t)rawValue - 2048.0f) / 2048.0f;
        Serial.print("  [");
        Serial.print(i);
        Serial.print("] Raw=");
        Serial.print(rawValue);
        Serial.print(" -> Float=");
        Serial.println(normalized, 4);
        Serial.flush();
        delay(50);
    }
    
    // TEST 4: Llenar buffer pequeño
    Serial.println("\nTEST 4: Llenando buffer de 100 muestras");
    Serial.flush();
    delay(100);
    
    for (int i = 0; i < 100; i++) {
        uint16_t rawValue = waveformsTable[7][i];
        testBuffer[i] = ((float32_t)rawValue - 2048.0f) / 2048.0f;
        
        if (i % 10 == 0) {
            Serial.print(".");
            Serial.flush();
        }
    }
    
    Serial.println(" OK");
    Serial.print("Primero: ");
    Serial.println(testBuffer[0], 4);
    Serial.print("Ultimo: ");
    Serial.println(testBuffer[99], 4);
    Serial.flush();
    
    // TEST 5: Probar con 900 muestras
    Serial.println("\nTEST 5: Intentando cargar 900 muestras");
    Serial.println("(Si se cuelga aqui, hay problema de memoria)");
    Serial.flush();
    delay(500);
    
    float32_t* bigBuffer = new float32_t[900];
    
    if (bigBuffer == nullptr) {
        Serial.println("ERROR: No se pudo asignar memoria para 900 muestras");
        while(1);
    }
    
    Serial.println("Memoria asignada OK, llenando...");
    Serial.flush();
    
    for (int i = 0; i < 900; i++) {
        uint16_t rawValue = waveformsTable[7][i];
        bigBuffer[i] = ((float32_t)rawValue - 2048.0f) / 2048.0f;
        
        if (i % 100 == 0) {
            Serial.print(".");
            Serial.flush();
            delay(10);
        }
    }
    
    Serial.println(" OK!");
    Serial.print("Primera muestra: ");
    Serial.println(bigBuffer[0], 4);
    Serial.print("Ultima muestra: ");
    Serial.println(bigBuffer[899], 4);
    Serial.flush();
    
    delete[] bigBuffer;
    
    Serial.println("\n=== TODOS LOS TESTS PASARON ===");
    Serial.println("El problema NO es el acceso a waveformsTable");
    Serial.println("Revisa loadSignal() en Waveforms.h");
}

void loop() {
    delay(1000);
}