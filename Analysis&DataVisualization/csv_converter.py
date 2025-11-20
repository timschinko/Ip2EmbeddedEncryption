#Achtung: man darf den Serial Monitor in der IDE nicht geöffnet haben, sonst funktioniert das Programm nicht! 
#Ablauf: c++ Programm auf den Mikrocontroller laden -- evt. serieller Monitor schließen -- Python Programm starten -- reset auf dem Microcontroller drücken

import serial
import csv
import time

# Passe hier deinen Port an (Port 11 für ESP32, Port 3 für ESP8266, PortX für Raspberry Pi Pico2), den passenden Port findet man im Geräte-Manager
PORT = 'COM11'
BAUDRATE = 9600
FILENAME = 'Analysis&DataVisualization\\csv_storage\\messdaten.csv'

begin_write = False

try:
    # Verbindung öffnen
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    print(f"Verbunden mit {PORT}. Schreibe in: {FILENAME}. Drücke STRG+C zum Beenden.")

    # CSV Datei öffnen (Modus 'a' für append/anhängen, 'w' für überschreiben)
    with open(FILENAME, mode='w', newline='') as csv_file:
        writer = csv.writer(csv_file, delimiter=',')

        while True:
            if ser.in_waiting > 0:
                # Zeile lesen und dekodieren
                line = ser.readline().decode('utf-8', errors='replace').strip() #errors='replace' um fehlerhafte Zeichen zu vermeiden

                if line.startswith("type"): #erst nach dem reset in die CSV Datei schreiben
                    begin_write = True
                
                if begin_write:
                    if line:
                        # die Daten anhand der Kommata trennen
                        data = line.split(',')
                        writer.writerow(data)

                        # Verhindert Datenverlust bei Absturz/Stecker ziehen
                        csv_file.flush()
                    
except KeyboardInterrupt:
    print("\nAufzeichnung beendet.")
    ser.close()
except Exception as e:
    print(f"Fehler: {e}")
    if 'ser' in locals() and ser.is_open:
        ser.close()