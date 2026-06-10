import serial
import time

# Konfigurácia sériového portu (zmeňte baudrate podľa potreby vášho zariadenia)
SERIAL_PORT = 'COM3'
BAUDRATE = 9600
# DATA_TO_SEND = b"DATA" # 'b' definuje, že posielame čisté bajty (bytes)
# DATA_TO_SEND = b"DATADATADATADATADATADATADATADATADATADATADATADATA14PINGxDATADATADATADATADATAR"
DATA_TO_SEND = b"DATADATADATADATADATADATADATADATADATADATADATADATA14PINGx"

try:
    # Otvorenie COM3 portu
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    print(f"Port {SERIAL_PORT} bol úspešne otvorený. Začínam posielať dáta...")

    # Zaznamenanie štartovacieho času
    start_time = time.time()
    duration = 1 # Doba vysielania v sekundách
    count = 0

#*********************************************************************************
    ser.write(b"2") # posielame první balík dat hned na začátku
    # Slučka beží presne 1 sekundu bez akéhokoľvek delay / sleep
    while (time.time() - start_time) < duration:
        ser.write(DATA_TO_SEND)
        count += 1
    print(f"Vysielanie ukončené. Dáta sa posielali celých 1 sekundu.")
    print(f"Počet odoslaných balíkov dát: {count}")

#*********************************************************************************
    time.sleep(0.2)  # 200 ms delay pred ďalším zápisom
    ser.write(b"1") # adresovany node
    ser.write(b"4PINGx")

except serial.SerialException as e:
    print(f"Chyba: Nepodarilo sa otvoriť alebo zapisovať na {SERIAL_PORT}. Skontrolujte, či port nepoužíva iný program.")
    print(e)

finally:
    # Bezpečné zatvorenie portu po skončení alebo pri chybe
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Sériový port bol bezpečne zatvorený.")
