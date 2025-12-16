/**

* LMS Benchmark con métricas extendidas:
* * MSE real contra la señal limpia
* * RMS de la señal filtrada
* * Correlación señal filtrada vs limpia
* * CPU%
*
* Cambiar manualmente:
* NUM_TAPS = 64 o 128
* MU       = 0.1 o 0.01
  */

#include <BioFilterLib.h>

#define SAMPLE_RATE   1000
#define NUM_TAPS      128        // <-- CAMBIAR
#define MU            0.01f     // <-- CAMBIAR
#define BLOCK_SIZE    1

const float ECG_HR = 75.0f;
const float POWERLINE_FREQ = 60.0f;
const float INTERFERENCE_AMP = 0.3f;

float32_t lmsCoeffs[NUM_TAPS];
LMSFilter* adaptiveFilter;

float timeCounter = 0.0f;

// ========= MÉTRICAS =========
uint32_t sampleCount = 0;
uint64_t totalTimeMicros = 0;

// Energías
double E_clean = 0;
double E_noise = 0;
double E_residual = 0;

// Para MSE entre señal filtrada y limpia
double mse_accum = 0;

// RMS filtrado
double rms_filtered_accum = 0;

// Para correlación
double sum_clean = 0;
double sum_filt = 0;
double sum_clean2 = 0;
double sum_filt2 = 0;
double sum_clean_filt = 0;

// =============================
// Generación de señales
// =============================
float generateECG(float t) {
float hrHz = ECG_HR / 60.0f;
float phase = fmod(2.0f * PI * hrHz * t, 2.0f * PI);
float e = 0;

if (phase > 0.3 && phase < 0.8)
    e += 0.15f * sin((phase - 0.3f) * 2 * PI / 0.5f);

if (phase > 1.0 && phase < 1.6) {
    if (phase < 1.2)
        e -= 0.1f * sin((phase - 1.0f) * 2 * PI / 0.2f);
    else if (phase < 1.4)
        e += 1.0f * sin((phase - 1.2f) * 2 * PI / 0.2f);
    else
        e -= 0.2f * sin((phase - 1.4f) * 2 * PI / 0.2f);
}

if (phase > 2.0 && phase < 2.8)
    e += 0.25f * sin((phase - 2.0f) * 2 * PI / 0.8f);

return e;

}

float generateInterference(float t) {
return INTERFERENCE_AMP * sin(2 * PI * POWERLINE_FREQ * t);
}

float generateReference(float t) {
return sin(2 * PI * POWERLINE_FREQ * t);
}

// =============================
// SETUP
// =============================
void setup() {
Serial.begin(115200);
while (!Serial) {}

Serial.println("=======================================");
Serial.println(" LMS Benchmark Extendido");
Serial.println("=======================================");
Serial.print("NUM_TAPS = "); Serial.println(NUM_TAPS);
Serial.print("MU       = "); Serial.println(MU, 4);
Serial.println();

for (int i = 0; i < NUM_TAPS; i++) lmsCoeffs[i] = 0.0f;

adaptiveFilter = new LMSFilter(lmsCoeffs, NUM_TAPS, MU, BLOCK_SIZE);

Serial.println("Procesando ~5 segundos...");
delay(800);

}

// =============================
// LOOP
// =============================
void loop() {
if (sampleCount >= 5000) {
printResults();
while (1);
}

float clean = generateECG(timeCounter);
float noise = generateInterference(timeCounter);
float ref   = generateReference(timeCounter);
float contaminated = clean + noise;

float y = 0, err = 0;

uint32_t t0 = micros();
adaptiveFilter->processSample(ref, contaminated, &y, &err);
uint32_t dt = micros() - t0;

float cleaned = contaminated - y;

totalTimeMicros += dt;
sampleCount++;

// ========= Energías =========
E_clean += clean * clean;
E_noise += noise * noise;
E_residual += (cleaned - clean) * (cleaned - clean);

// ========= MSE filtrada vs limpia =========
mse_accum += (cleaned - clean) * (cleaned - clean);

// ========= RMS filtrada =========
rms_filtered_accum += cleaned * cleaned;

// ========= Correlación =========
sum_clean       += clean;
sum_filt        += cleaned;
sum_clean2      += clean   * clean;
sum_filt2       += cleaned * cleaned;
sum_clean_filt  += clean * cleaned;

timeCounter += 1.0f / SAMPLE_RATE;

}

// =============================
// RESULTADOS
// =============================
void printResults() {
Serial.println("\n=======================================");
Serial.println("           RESULTADOS EXTENDIDOS");
Serial.println("=======================================\n");

float timePerSample = (float)totalTimeMicros / sampleCount;
float realSPS = 1e6 / timePerSample;
float cpuPercent = (timePerSample / 1000.0f) * 100.0f;

Serial.println("---- Rendimiento ----");
Serial.print("Tiempo por muestra: "); Serial.print(timePerSample, 3); Serial.println(" us");
Serial.print("Muestras/s reales: ");  Serial.print(realSPS, 1); Serial.println(" SPS");
Serial.print("Uso CPU: ");           Serial.print(cpuPercent, 2); Serial.println(" %\n");

// Memoria
int ramCoefs  = NUM_TAPS * sizeof(float);
int ramBuffer = NUM_TAPS * sizeof(float);
int ramTotal  = ramCoefs + ramBuffer;

Serial.println("---- Memoria ----");
Serial.print("RAM coeficientes: "); Serial.print(ramCoefs); Serial.println(" bytes");
Serial.print("RAM buffer: ");       Serial.print(ramBuffer); Serial.println(" bytes");
Serial.print("RAM total LMS: ");    Serial.print(ramTotal);  Serial.println(" bytes\n");

// SNR
float SNR_in  = 10.0f * log10(E_clean / E_noise);
float SNR_out = 10.0f * log10(E_clean / E_residual);
float ISNR    = SNR_out - SNR_in;

Serial.println("---- Calidad del Filtrado ----");
Serial.print("SNR entrada: "); Serial.print(SNR_in, 2); Serial.println(" dB");
Serial.print("SNR salida:  "); Serial.print(SNR_out, 2); Serial.println(" dB");
Serial.print("ISNR:        "); Serial.print(ISNR, 2); Serial.println(" dB");

// MSE nueva
float MSE = mse_accum / sampleCount;
Serial.print("MSE(cleaned vs clean): "); Serial.println(MSE, 6);

// RMS
float RMS_filt = sqrt(rms_filtered_accum / sampleCount);
Serial.print("RMS señal filtrada: "); Serial.println(RMS_filt, 6);

// Correlación filtrada vs limpia
float N = sampleCount;
float cov = (sum_clean_filt / N) - (sum_clean / N) * (sum_filt / N);
float var_c = (sum_clean2 / N) - (sum_clean / N)*(sum_clean / N);
float var_f = (sum_filt2 / N) - (sum_filt / N)*(sum_filt / N);
float corr = cov / sqrt(var_c * var_f);

Serial.print("Correlacion(cleaned vs clean): "); Serial.println(corr, 4);

Serial.println("\nFIN DE LA PRUEBA.");

}
