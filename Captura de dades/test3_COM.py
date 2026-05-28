import serial
import struct
import time

PORT = '/dev/ttyACM0'
BAUD = 921600
NUM_MOSTRES = 48000
fitxer_nom = "Dades_Senyal_Captura2.txt"

ser = serial.Serial(PORT, BAUD, timeout=1)
if ser.is_open:
    print("Connectat exitosament")
ser.reset_input_buffer() 
print("Esperant sincronització (contador ESP32 = 1)...")

try:
    with open(fitxer_nom, "w") as f:
        f.write("Mostres_ESP32,Valor1_mV, Valor2_mV\n")

        buffer_binari = bytearray()
        mostres_rebudes = 0
        sincronitzat = False
        temps_inici = None

        while mostres_rebudes < NUM_MOSTRES:
            chunk = ser.read(ser.in_waiting or 1)
            if chunk:
                buffer_binari.extend(chunk)
                #print("OK1")

            i = 0
            sortida = []
            while i <= len(buffer_binari) - 14:
                if buffer_binari[i] == 0xAA and buffer_binari[i+1] == 0xCD:
                    try:
                        data = buffer_binari[i+2:i+14]
                        contador_esp, valor1, valor2 = struct.unpack('<III', data)
                        #print("OK2")

                        # Esperem el contador == 1 per sincronitzar
                        if not sincronitzat:
                            if contador_esp == 1:
                                sincronitzat = True
                                temps_inici = time.perf_counter()
                                #print("OK3")
                                print("✅ Sincronitzat! Començant captura...")
                            else:
                                i += 14
                                continue

                        sortida.append(f"{mostres_rebudes+1},{valor1}, {valor2}")
                        mostres_rebudes += 1
                        i += 14
                        #print("OK4")
                        if mostres_rebudes >= NUM_MOSTRES:
                            break
                    except struct.error:
                        i += 1
                else:
                    i += 1

            if sortida:
                f.write("\n".join(sortida) + "\n")
            del buffer_binari[:i]
            #print("OK5")

    temps_total = time.perf_counter() - temps_inici
    freq_real = NUM_MOSTRES / temps_total

    print(f"Captura finalitzada: {mostres_rebudes} mostres rebudes.")
    print(f"Temps total: {temps_total:.3f} segons")
    print(f"Freqüència real: {freq_real:.1f} Hz")

finally:
    ser.close()
