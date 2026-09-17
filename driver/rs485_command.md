# rs485_command

Vrstva nad `rs485` pre jednoduchý ASCII command-response protokol s adresovaním uzlov.

## Súbory
- `rs485_command.h` – verejné API
- `rs485_command.c` – implementácia

## Protokol

Rámec príkazu (binárny, odosielaný masterom):

```
[ node_id (1B) ][ length (1B) ][ command string (N B) ][ CRC (1B) ]
```

- `node_id` – adresa cieľového uzla (1 bajt, uložená vo Flash na adrese `0x0800FC00`)
- `length` – dĺžka command stringu
- `command` – ASCII reťazec (napr. `"PING"`)
- `CRC` – jednoduchý checksum

Odpoveď uzla je čistý ASCII reťazec vrátený callbackom (napr. `"PONG\r\n"`).

## Konfigurácia

```c
typedef struct {
    uint8_t  node_id;            // ID tohto uzla (0 = čítaj z Flash)
    uint32_t response_delay_ms;  // oneskorenie odpovede v ms
} RS485_commandConfig_t;
```

## Tabuľka príkazov

```c
typedef struct {
    const char              *command;   // ASCII názov príkazu
    RS485_commandCallback_t  callback;  // handler vracajúci odpoveď
} RS485_command_t;

typedef const char *(*RS485_commandCallback_t)(void);
```

## API

### `RS485_commandInit`
```c
void RS485_commandInit(const RS485_commandConfig_t *config);
```
Inicializuje modul, registruje RX callback do `rs485` drivera.

---

### `RS485_commandSetTable`
```c
void RS485_commandSetTable(const RS485_command_t *table, uint32_t count);
```
Nastaví tabuľku podporovaných príkazov.

---

### `RS485_commandProcess`
```c
void RS485_commandProcess(void);
```
Volať v hlavnej slučke. Skontroluje, či bol prijatý kompletný príkaz, vyhľadá ho v tabuľke a odošle odpoveď cez `RS485_send`.

## Použitie

```c
static const char *cmdPing(void) { return "PONG\r\n"; }
static const char *cmdTemp(void) { return "TEMP=25.0\r\n"; }

static const RS485_command_t table[] = {
    {"PING", cmdPing},
    {"TEMP", cmdTemp},
};

RS485_commandConfig_t cfg = { .node_id = 0u, .response_delay_ms = 3u };

RS485_init(&rs485_config);
RS485_commandInit(&cfg);
RS485_commandSetTable(table, 2u);

while (1) {
    RS485_commandProcess();
    __WFI();
}
```

## Poznámky
- Node ID `0` spôsobí načítanie adresy z Flash pamäte (`0x0800FC00`). Ak je Flash na tej adrese `0xFF`, použije sa default `'1'`.
- Buffer pre príkaz je `RS485_COMMAND_BUFFER_SIZE` = 64 bajtov.
- Príkazy sú case-sensitive.
