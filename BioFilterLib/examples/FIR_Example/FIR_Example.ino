#include <BioFilterLib.h>
#include "Waveforms.h"

// Parámetros del filtro
#define BLOCK_SIZE  32
#define NUM_TAPS    51

// Coeficientes FIR simulados (pasa bajas, por ejemplo)
const float32_t firCoeffs[NUM_TAPS] = {
   -0.0011, -0.0013, -0.0016, -0.0018, -0.0017, -0.0011,  0.0000,  0.0014,  0.0032,  0.0051,
    0.0069,  0.0082,  0.0087,  0.0082,  0.0065,  0.0039,  0.0007, -0.0026, -0.0056, -0.0079,
   -0.0090, -0.0088, -0.0073, -0.0047, -0.0015,  0.0018,  0.0046,  0.0065,  0.0070,  0.0061,
    0.0039,  0.0009, -0.0025, -0.0055, -0.0075, -0.0080, -0.0069, -0.0044, -0.0008,  0.0032,
    0.0071,  0.0101,  0.0116,  0.0110,  0.0082,  0.0035, -0.0023, -0.0086, -0.0141, -0.0179,
   -0.0190
};

// Buffers
float32_t inputBuffer[MAX_SAMPLES];   // Convertido de uint16_t
float32_t outputSignal[MAX_SAMPLES];  // Salida filtrada

FIRFilter* myFilter;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== FIR Filter Example ===");

  // Convertir señal uint16_t -> float32_t
  for (uint16_t i = 0; i < testSignalLength; i++) {
    inputBuffer[i] = (float32_t)waveSin[i];
  }

  // Inicializar filtro
  myFilter = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE);

  // Procesar por bloques
  for (uint16_t i = 0; i < testSignalLength; i += BLOCK_SIZE) {
    uint16_t currentBlock = min(BLOCK_SIZE, testSignalLength - i);
    myFilter->processBuffer(&inputBuffer[i], &outputSignal[i], currentBlock);
  }

  // Mostrar resultados
  for (uint16_t i = 0; i < testSignalLength; i++) {
    Serial.print("Input: ");
    Serial.print(inputBuffer[i], 4);
    Serial.print("  --> Output: ");
    Serial.println(outputSignal[i], 4);
    delay(5);  // ralentiza para facilitar lectura
  }

  Serial.println("=== Fin del filtrado ===");
}

void loop() {
  // Nada
}
