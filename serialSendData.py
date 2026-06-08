import serial
import time

# Konfigurácia sériového portu (zmeňte baudrate podľa potreby vášho zariadenia)
SERIAL_PORT = 'COM3'
BAUDRATE = 9600
DATA_TO_SEND = b"DATA" # 'b' definuje, že posielame čisté bajty (bytes)

try:
    # Otvorenie COM3 portu
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    print(f"Port {SERIAL_PORT} bol úspešne otvorený. Začínam posielať dáta...")

    # Zaznamenanie štartovacieho času
    start_time = time.time()
    duration = 5 # Doba vysielania v sekundách
    count = 0

    # Slučka beží presne 5 sekúnd bez akéhokoľvek delay / sleep
    ser.write(b"1") # posielame první balík dat hned na začátku
    while (time.time() - start_time) < duration:
        ser.write(DATA_TO_SEND)
        count += 1

    print(f"Vysielanie ukončené. Dáta sa posielali celých 5 sekúnd.")
    print(f"Počet odoslaných balíkov dát: {count}")

except serial.SerialException as e:
    print(f"Chyba: Nepodarilo sa otvoriť alebo zapisovať na {SERIAL_PORT}. Skontrolujte, či port nepoužíva iný program.")
    print(e)

finally:
    # Bezpečné zatvorenie portu po skončení alebo pri chybe
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Sériový port bol bezpečne zatvorený.")
