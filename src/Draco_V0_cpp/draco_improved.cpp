#include <Arduino.h>
#include <stdint.h>
#include <string.h>

// Konstanten für die Array-Größen
const int NFSR1_SIZE = 33;  // Bits
const int NFSR2_SIZE = 95;  // Bits
const int KEY_SIZE = 128;   // Bits
const int IV_SIZE = 96;     // Bits

const int KEY_LENGTH_BYTES = 16;
const int IV_LENGTH_BYTES = 12;
const int NFSR1_LENGTH_BYTES = 5;
const int NFSR2_LENGTH_BYTES = 12;

// Struktur für den initialisierten Zustand
struct DracoState {
    uint8_t nfsr1[NFSR1_LENGTH_BYTES];  // 33 bits
    uint8_t nfsr2[NFSR2_LENGTH_BYTES];  // 95 bits
    uint8_t* key;   // Pointer statt Array
    uint8_t* iv;    // Pointer statt Array  
    int t;          // Zeitschritt für KiS
};

// Grundlegende Bit-Operationen
bool getBit(uint8_t* array, int bitIndex) {
    int byteIndex = bitIndex / 8;
    int bitOffset = 7 - (bitIndex % 8);
    return (array[byteIndex] & (1 << bitOffset)) != 0;
}

void setBit(uint8_t* array, int bitIndex, bool value) {
    // int byteIndex = bitIndex / 8;
    // int bitOffset = 7 - (bitIndex % 8);
    // if (value) {
    //     array[byteIndex] |= (1 << bitOffset);
    // } else {
    //     array[byteIndex] &= ~(1 << bitOffset);
    // }
    int byteIndex = bitIndex / 8;
    array[byteIndex] = (array[byteIndex] & ~(1 << (7 - (bitIndex - byteIndex * 8)))) | ((uint8_t)value << (7 - (bitIndex - byteIndex * 8)));
}

void shiftLeft(uint8_t* array, int numBytes) {
    for (int i = 0; i < numBytes - 1; i++) {
        array[i] = (array[i] << 1) | (array[i + 1] >> 7);
    }
    array[numBytes - 1] <<= 1;
}

// NFSR Feedback-Funktionen
bool draco_nfsr1_feedback(uint8_t* nfsr1) {
    // bool allZero = true;
    // for (int i = 1; i < 33; i++) {
    //     if (getBit(nfsr1, i)) {
    //         allZero = false;
    //         break;
    //     }
    // }
    bool allZero = (nfsr1[0] & !(1 << 7)) == 0;
    for (int i = 1; i < NFSR1_LENGTH_BYTES; i++) {
        if (nfsr1[i] > 0) {
            allZero = false;
            break;
        }
    }

    bool one = getBit(nfsr1, 1);
    bool eight = getBit(nfsr1, 8);
    bool one_eight = one & eight;
    bool twelve = getBit(nfsr1, 12);
    bool sixteen = getBit(nfsr1, 16);
    bool twelve_sixteen = twelve & sixteen;
    bool seventeen = getBit(nfsr1, 17);
    bool fifteen = getBit(nfsr1, 15);
    bool fourteen = getBit(nfsr1, 14);
    bool eighteen = getBit(nfsr1, 18);
    bool twentyfour = getBit(nfsr1, 24);
    bool twentyfour_seventeen = twentyfour & seventeen;


    return getBit(nfsr1, 0) ^ getBit(nfsr1, 2) ^ getBit(nfsr1, 7) ^ getBit(nfsr1, 9) ^
           getBit(nfsr1, 10) ^ getBit(nfsr1, 15) ^ getBit(nfsr1, 23) ^ getBit(nfsr1, 25) ^
           getBit(nfsr1, 30) ^
           (eight & fifteen) ^
           (twelve_sixteen) ^
           (getBit(nfsr1, 13) & fifteen) ^
           (getBit(nfsr1, 13) & getBit(nfsr1, 25)) ^
           // Three-bit terms
           (one_eight & fourteen) ^
           (one_eight & eighteen) ^
           (eight & twelve_sixteen) ^
           (eight & fourteen & eighteen) ^
           (eight & fifteen & sixteen) ^
           (eight & fifteen & seventeen) ^
           (fifteen & twentyfour_seventeen) ^
           // Four-bit terms
           (one_eight & fourteen & seventeen) ^
           (one_eight & seventeen & eighteen) ^
           (one & fourteen & twentyfour_seventeen) ^
           (one & eighteen & twentyfour_seventeen) ^
           (eight & twelve_sixteen & seventeen) ^
           (eight & fourteen & seventeen & eighteen) ^
           (eight & fifteen & sixteen & seventeen) ^
           (twelve_sixteen & twentyfour_seventeen) ^
           (fourteen & twentyfour_seventeen & eighteen) ^
           (fifteen & sixteen & twentyfour_seventeen) ^
           allZero;
}

bool draco_nfsr2_feedback(uint8_t* nfsr2) {
    return getBit(nfsr2, 0) ^ getBit(nfsr2, 26) ^ getBit(nfsr2, 56) ^
           getBit(nfsr2, 89) ^ getBit(nfsr2, 94) ^
           (getBit(nfsr2, 3) & getBit(nfsr2, 67)) ^
           (getBit(nfsr2, 11) & getBit(nfsr2, 13)) ^
           (getBit(nfsr2, 17) & getBit(nfsr2, 18)) ^
           (getBit(nfsr2, 27) & getBit(nfsr2, 59)) ^
           (getBit(nfsr2, 36) & getBit(nfsr2, 39)) ^
           (getBit(nfsr2, 40) & getBit(nfsr2, 48)) ^
           (getBit(nfsr2, 50) & getBit(nfsr2, 79)) ^
           (getBit(nfsr2, 54) & getBit(nfsr2, 71)) ^
           (getBit(nfsr2, 58) & getBit(nfsr2, 63)) ^
           (getBit(nfsr2, 61) & getBit(nfsr2, 65)) ^
           (getBit(nfsr2, 68) & getBit(nfsr2, 84)) ^
           (getBit(nfsr2, 8) & getBit(nfsr2, 46) & getBit(nfsr2, 87)) ^
           (getBit(nfsr2, 22) & getBit(nfsr2, 24) & getBit(nfsr2, 25)) ^
           (getBit(nfsr2, 70) & getBit(nfsr2, 78) & getBit(nfsr2, 82)) ^
           (getBit(nfsr2, 86) & getBit(nfsr2, 90) & getBit(nfsr2, 91) & getBit(nfsr2, 93));
}

bool draco_output(uint8_t* nfsr1, uint8_t* nfsr2) {
    // L calculation
    bool L = getBit(nfsr2, 7) ^ getBit(nfsr2, 15) ^ getBit(nfsr2, 32) ^
             getBit(nfsr2, 47) ^ getBit(nfsr2, 66) ^ getBit(nfsr2, 80) ^
             getBit(nfsr2, 92);

    // Q calculation
    bool Q = (getBit(nfsr2, 5) & getBit(nfsr2, 85)) ^
             (getBit(nfsr2, 12) & getBit(nfsr2, 74)) ^
             (getBit(nfsr2, 20) & getBit(nfsr2, 69)) ^
             (getBit(nfsr2, 34) & getBit(nfsr2, 57));

    // T1 calculation
    bool T1 = getBit(nfsr2, 53) ^
              (getBit(nfsr2, 38) & getBit(nfsr2, 44)) ^
              (getBit(nfsr2, 23) & getBit(nfsr2, 49) & getBit(nfsr2, 83)) ^
              (getBit(nfsr2, 6) & getBit(nfsr2, 33) & getBit(nfsr2, 51) & getBit(nfsr2, 73)) ^
              (getBit(nfsr2, 4) & getBit(nfsr2, 29) & getBit(nfsr2, 43) &
               getBit(nfsr2, 60) & getBit(nfsr2, 81)) ^
              (getBit(nfsr2, 9) & getBit(nfsr2, 14) & getBit(nfsr2, 35) &
               getBit(nfsr2, 42) & getBit(nfsr2, 55) & getBit(nfsr2, 77)) ^
              (getBit(nfsr2, 1) & getBit(nfsr2, 16) & getBit(nfsr2, 28) &
               getBit(nfsr2, 45) & getBit(nfsr2, 64) & getBit(nfsr2, 75) &
               getBit(nfsr2, 88));

    // T2 calculation
    bool T2 = getBit(nfsr1, 26) ^
              (getBit(nfsr1, 5) & getBit(nfsr1, 19)) ^
              (getBit(nfsr1, 11) & getBit(nfsr1, 22) & getBit(nfsr1, 31));

    // T3 calculation
    bool T3 = getBit(nfsr2, 76) ^
              (getBit(nfsr1, 3) & getBit(nfsr2, 10)) ^
              (getBit(nfsr1, 20) & getBit(nfsr2, 21) & getBit(nfsr2, 30)) ^
              (getBit(nfsr1, 6) & getBit(nfsr1, 29) & getBit(nfsr2, 62) & getBit(nfsr2, 72));

    return (L ^ Q ^ T1 ^ T2 ^ T3);
}

bool draco_kis(uint8_t* key, uint8_t* iv, uint8_t* nfsr1, int t) {
    int ivpos = t % 97;
    bool ivbit = (ivpos == 0) ? false : getBit(iv, ivpos - 1);

    if (t <= 255) {
        return ivbit;
    }

    return getBit(key, t % 32) ^ ivbit;
}

// Initialisierungsfunktion
DracoState initialize_draco(uint8_t* key, uint8_t* iv) {
    DracoState state = {};

    // Direkte Pointer-Zuweisung statt memcpy
    state.key = key;
    state.iv = iv;

    //Initialize NFSR2 with first 95 key bits
    memcpy(state.nfsr2, key, NFSR2_LENGTH_BYTES);
    setBit(state.nfsr2, 0, !getBit(state.nfsr2, 0));

    // Initialize NFSR1 with remaining key bits
    for (int i = 0; i < 33; i++) {
        setBit(state.nfsr1, i, getBit(key, i + 95));
    }

    state.t = 0;

    // Initialization phase
    while (state.t < 512) {
        bool nfsr1_feedbackBit = draco_nfsr1_feedback(state.nfsr1);
        bool nfsr2_feedbackBit = draco_nfsr2_feedback(state.nfsr2);
        bool outputBit = draco_output(state.nfsr1, state.nfsr2);

        bool kis = draco_kis(state.key, state.iv, state.nfsr1, state.t);

        shiftLeft(state.nfsr2, 12);
        setBit(state.nfsr2, 94, nfsr2_feedbackBit ^ getBit(state.nfsr1, 0) ^ kis ^ outputBit);

        shiftLeft(state.nfsr1, 5);
        setBit(state.nfsr1, 32, nfsr1_feedbackBit ^ outputBit);

        state.t++;
    }

    return state;
}

// Keystream-Generierungsfunktion
void generate_keystream(DracoState& state, uint8_t* keystream, int lengthInBits) {
    memset(keystream, 0, (lengthInBits + 7) / 8);

    for (int i = 0; i < lengthInBits; i++) {
        bool output = draco_output(state.nfsr1, state.nfsr2);
        setBit(keystream, i, output);

        bool nfsr1_feedbackBit = draco_nfsr1_feedback(state.nfsr1);
        bool nfsr2_feedbackBit = draco_nfsr2_feedback(state.nfsr2);
        bool kis = draco_kis(state.key, state.iv, state.nfsr1, state.t);

        shiftLeft(state.nfsr2, 12);
        setBit(state.nfsr2, 94, nfsr2_feedbackBit ^ getBit(state.nfsr1, 0) ^ kis);

        shiftLeft(state.nfsr1, 5);
        setBit(state.nfsr1, 32, nfsr1_feedbackBit);

        state.t++;
    }
}

// Test Framework
struct TestVector {
    uint8_t key[16];
    uint8_t iv[12];
    uint8_t expected[16];
};

void printHex(const uint8_t* data, int length) {
    for (int i = 0; i < length; i++) {
        if (data[i] < 0x10) Serial.print('0');
        Serial.print(data[i], HEX);
    }
    Serial.println();
}
void testVectors() {
    Serial.println(F("\n=== Test Vectors ==="));
    
    TestVector vectors[] = {
        {
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
             0x00, 0x00, 0x00, 0x00},
            {0x6F, 0xB3, 0xAB, 0x21, 0xA9, 0xB0, 0x05, 0x07,
             0xCE, 0x18, 0x71, 0x0E, 0x35, 0xFB, 0x40, 0xAB}
        },
        {
            {0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
             0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F},
            {0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
             0xF0, 0xF0, 0xF0, 0xF0},
            {0xD0, 0x65, 0xAC, 0x7B, 0x05, 0x8A, 0x2B, 0x56,
             0x52, 0x3B, 0xAC, 0x08, 0xDE, 0x9E, 0x93, 0xA4}
        },
        {
            {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
             0x98, 0x76, 0x54, 0x32, 0x10, 0xFE, 0xDC, 0xBA},
            {0xAA, 0xBC, 0xDE, 0xF0, 0x12, 0x34, 0x56, 0x78,
             0x9A, 0xBC, 0xDE, 0xFF},
            {0x45, 0xA8, 0x4D, 0xC6, 0xF5, 0x66, 0x23, 0xEF,
             0x48, 0x29, 0x89, 0xB1, 0x5E, 0x92, 0x4E, 0xD8}
        }
    };

    uint8_t keystream[16];
    
    for (int i = 0; i < 3; i++) {
        Serial.print(F("Test Vector "));
        Serial.print(i + 1);
        Serial.print(F(": "));
        
        DracoState state = initialize_draco(vectors[i].key, vectors[i].iv);
        generate_keystream(state, keystream, 128);
        
        bool match = memcmp(keystream, vectors[i].expected, 16) == 0;
        
        if (match) {
            Serial.println(F("PASS"));
        } else {
            Serial.println(F("FAIL"));
            Serial.print(F("Expected: ")); printHex(vectors[i].expected, 16);
            Serial.print(F("Got: ")); printHex(keystream, 16);
        }
    }
}
void testBitOperations() {
    Serial.println(F("\n=== Bit Operations Test ==="));

    uint8_t testByte = 0xA5; // 10100101
    Serial.print(F("Testing getBit: "));
    bool getBitSuccess = true;
    if (getBit(&testByte, 0) != 1) getBitSuccess = false;
    if (getBit(&testByte, 1) != 0) getBitSuccess = false;
    if (getBit(&testByte, 7) != 1) getBitSuccess = false;
    Serial.println(getBitSuccess ? F("PASS") : F("FAIL"));

    // Test setBit
    uint8_t testSet = 0x00;
    setBit(&testSet, 0, true);
    Serial.print(F("Testing setBit: "));
    Serial.println(testSet == 0x80 ? F("PASS") : F("FAIL"));

    // Test shiftLeft
    uint8_t shiftTest[2] = {0xA5, 0x5A};
    Serial.print(F("Testing shiftLeft: "));
    shiftLeft(shiftTest, 2);
    Serial.println((shiftTest[0] == 0x4A && shiftTest[1] == 0xB4) ? F("PASS") : F("FAIL"));
}

void testNFSRFeedback() {
    Serial.println(F("\n=== NFSR Feedback Test ==="));

    // Test NFSR1
    uint8_t nfsr1[5] = {0}; // Alle Bits auf 0
    Serial.print(F("NFSR1 Zero State: "));
    bool feedback1 = draco_nfsr1_feedback(nfsr1);
    Serial.println(feedback1 ? F("PASS") : F("FAIL"));

    // Test NFSR2
    uint8_t nfsr2[12] = {0}; // Alle Bits auf 0
    Serial.print(F("NFSR2 Zero State: "));
    bool feedback2 = draco_nfsr2_feedback(nfsr2);
    Serial.println(!feedback2 ? F("PASS") : F("FAIL"));
}

void calculateVariance(float* times, int count, float mean, const char* testName) {
    float variance = 0.0;
    for(int i = 0; i < count; i++) {
        float diff = times[i] - mean;
        variance += diff * diff;
    }
    variance /= count;

    Serial.print(F("Varianz "));
    Serial.print(testName);
    Serial.print(F(": "));
    Serial.print(sqrt(variance));
    Serial.println(F(" Mikrosekunden"));
}

void benchmarkFull() {
    const int NUM_LENGTHS = 4;
    const int lengths[NUM_LENGTHS] = {1, 8, 128, 1024}; // Länge in Bits
    const int numTests = 100;

    uint8_t baseKey[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                          0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint8_t baseIV[12] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                         0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    uint8_t key[16];
    uint8_t iv[12];
    uint8_t keystream[128];
    uint8_t plaintext[128];
    uint8_t ciphertext[128];

    // CSV-Header nur einmal drucken
    Serial.println(F("type,randomKey,randomIV,keystreamLength,timeMicros"));

    // Für jede Keystream-Länge
    for (int l = 0; l < NUM_LENGTHS; l++) {
        yield();

        // Für jede Konfiguration
        for (int randomKey = 0; randomKey <= 1; randomKey++) {
            for (int randomIV = 0; randomIV <= 1; randomIV++) {
                yield();

                // Tests ausführen
                for (int test = 0; test < numTests; test++) {
                    // Key und IV setzen
                    if (randomKey) {
                        for (int i = 0; i < 16; i++) {
                            key[i] = test + i * 2;
                        }
                    } else {
                        memcpy(key, baseKey, 16);
                    }

                    if (randomIV) {
                        for (int i = 0; i < 12; i++) {
                            iv[i] = test + i * 3;
                        }
                    } else {
                        memcpy(iv, baseIV, 12);
                    }
                    
                    DracoState state = initialize_draco(key, iv);

                    // Benchmark starten (nach der Initialisierung)
                    unsigned long start = micros();

                    // 1. Keystream generieren
                    generate_keystream(state, keystream, lengths[l]);

                    // 2. Verschlüsselung
                    for (int i = 0; i < (lengths[l] + 7) / 8; i++) {
                        ciphertext[i] = plaintext[i] ^ keystream[i];
                    }

                    unsigned long duration = micros() - start;

                    // CSV-Zeile ausgeben
                    Serial.print(F("full,"));
                    Serial.print(randomKey ? "true" : "false");
                    Serial.print(F(","));
                    Serial.print(randomIV ? "true" : "false");
                    Serial.print(F(","));
                    Serial.print(lengths[l]);
                    Serial.print(F(","));
                    Serial.println(duration);

                    if (test % 10 == 0) {
                        yield();
                    }
                }
            }
        }
    }
}

// Nur Keystream-Generierung
void benchmarkKeystream() {
    const int NUM_LENGTHS = 4;
    const int lengths[NUM_LENGTHS] = {1, 8, 128, 1024};
    const int numTests = 100;

    uint8_t baseKey[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                          0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint8_t baseIV[12] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                         0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    uint8_t key[16];
    uint8_t iv[12];
    uint8_t keystream[128];

    // For each length
    for (int l = 0; l < NUM_LENGTHS; l++) {
        yield();

        // For each configuration
        for (int randomKey = 0; randomKey <= 1; randomKey++) {
            for (int randomIV = 0; randomIV <= 1; randomIV++) {
                yield();

                // Run tests
                for (int test = 0; test < numTests; test++) {
                    // Set up key and IV
                    if (randomKey) {
                        for (int i = 0; i < 16; i++) {
                            key[i] = test + i * 2;
                        }
                    } else {
                        memcpy(key, baseKey, 16);
                    }

                    if (randomIV) {
                        for (int i = 0; i < 12; i++) {
                            iv[i] = test + i * 3;
                        }
                    } else {
                        memcpy(iv, baseIV, 12);
                    }

                    // Initialize state once
                    DracoState state = initialize_draco(key, iv);

                    // Test current length
                    unsigned long start = micros();
                    generate_keystream(state, keystream, lengths[l]);
                    unsigned long duration = micros() - start;

                    // Output CSV line
                    Serial.print(F("struct_keystream_gen,"));
                    Serial.print(randomKey ? "true" : "false");
                    Serial.print(F(","));
                    Serial.print(randomIV ? "true" : "false");
                    Serial.print(F(","));
                    Serial.print(lengths[l]);
                    Serial.print(F(","));
                    Serial.println(duration);

                    if (test % 10 == 0) {
                        yield();
                    }
                }
            }
        }
    }
}

// Nur Initialisierung
void benchmarkInit() {
    const int numTests = 100;

    uint8_t baseKey[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                          0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint8_t baseIV[12] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
                         0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    uint8_t key[16];
    uint8_t iv[12];

    // CSV Header wenn noch nicht gedruckt
    Serial.println(F("type,randomKey,randomIV,timeMicros"));

    // For each configuration
    for (int randomKey = 0; randomKey <= 1; randomKey++) {
        for (int randomIV = 0; randomIV <= 1; randomIV++) {
            yield();

            // Run tests
            for (int test = 0; test < numTests; test++) {
                // Set up key and IV
                if (randomKey) {
                    for (int i = 0; i < 16; i++) {
                        key[i] = test + i * 2;
                    }
                } else {
                    memcpy(key, baseKey, 16);
                }

                if (randomIV) {
                    for (int i = 0; i < 12; i++) {
                        iv[i] = test + i * 3;
                    }
                } else {
                    memcpy(iv, baseIV, 12);
                }

                // Run benchmark
                unsigned long start = micros();
                DracoState state = initialize_draco(key, iv);
                unsigned long duration = micros() - start;

                if (state.nfsr1[0] == 0) {
                    Serial.print("");
                }

                // Output CSV line
                Serial.print(F("struct_init,"));
                Serial.print(randomKey ? "true" : "false");
                Serial.print(F(","));
                Serial.print(randomIV ? "true" : "false");
                Serial.print(F(","));
                Serial.println(duration);

                if (test % 10 == 0) {
                    yield();
                }
            }
        }
    }
}

void setup() {
    Serial.begin(9600);
    delay(10000);

    // CSV Header nur einmal am Anfang
    //Serial.println(F("type,randomKey,randomIV,keystreamLength,timeMicros")); //ist auskommentiert, da es den Header auch in jeder Methode gibt
    //testVectors();
    // Hier die gewünschten Benchmarks aktivieren/deaktivieren
    benchmarkFull();
    //benchmarkInit();
    //benchmarkKeystream();
}

void loop() {

}
