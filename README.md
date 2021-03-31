# Kopiowanie plików z dysku o strukturze FAT16

## Informacje ogólne

Autorzy:
Przemysław Grabarek
Nina Gorazd


## Problem:

Napisać program, który ma za zadanie skopiować pliki lub katalogi z dysku. Dane przechowane są w tablicy alokacji plików FAT16.

## Struktura FAT16


FAT (File allocation table) to typ architektury komputera  wykorzystuje system plików tabeli indeksów, tabeli alokacji plików , przydzielanej statycznie w czasie formatowania. Tabela zawiera:

wpisy dla każdego klastra , ciągłego obszaru pamięci dyskowej. 

Każda pozycja zawiera albo numer następnego klastra w pliku, albo znacznik wskazujący koniec pliku, nieużywane miejsce na dysku lub specjalne zarezerwowane obszary dysku.

Katalog główny dysku zawiera numer pierwszego klastra każdego pliku w tym katalogu; 
 
system operacyjny może następnie przechodzić przez tabelę FAT, szukając numeru klastra każdej kolejnej części pliku dysku jako łańcucha klastrów, aż do osiągnięcia końca pliku.

 W podobny sposób podkatalogi są implementowane jako specjalne pliki zawierające wpisy katalogów odpowiednich plików.

## Układ FAT16:


Region
Rozmiar
Zawartość
Sektory zastrzeżone 
(liczba zarezerwowanych sektorów)
Sektor rozruchowy,
Więcej zarezerwowanych sektorów
Region FAT
(liczba FAT) * (sektory na FAT)
Tabela alokacji plików nr 1
Tabela alokacji plików nr 2
Region katalogu głównego
(liczba pozycji root *bajtów na pozycję)/ bajtów na sektor)
Katalog główny
Region danych
(liczba klastrów) * sektory na klaster)
Region danych (do końca partycji lub dysku)




System plików FAT16 składa się z czterech regionów: 

Sektory zastrzeżone
Pierwszym zarezerwowanym sektorem (sektor logiczny 0) jest sektor rozruchowy (zwany także woluminowym rekordem rozruchowym lub po prostu VBR ). Zawiera obszar zwany blokiem parametrów systemu BIOS ( BPB ), który zawiera podstawowe informacje o systemie plików, w szczególności jego typ i wskaźniki lokalizacji innych sekcji, i zwykle zawiera kod modułu ładującego rozruch systemu operacyjnego .
Region FAT
Zwykle zawiera dwie kopie tabeli alokacji plików w celu sprawdzenia nadmiarowości, choć rzadko używane, nawet przez narzędzia do naprawy dysków.
Są to mapy regionu danych, wskazujące, które klastry są używane przez pliki i katalogi. W FAT12 i FAT16 natychmiast podążają za zarezerwowanymi sektorami.
Region katalogu głównego
Jest to tabela katalogów, która przechowuje informacje o plikach i katalogach znajdujących się w katalogu głównym. Nakłada na katalog główny stały maksymalny rozmiar, który jest wstępnie przydzielany podczas tworzenia tego woluminu. 
Region danych
Tutaj przechowywane są rzeczywiste dane pliku i katalogu, które zajmują większość partycji.

FAT używa format little-endian dla wszystkich pozycji w nagłówku.

Całkowita liczba sektorów (jak odnotowano w rekordzie rozruchowym) może być większa niż liczba sektorów wykorzystywanych przez dane (klastry × sektory na klaster), FAT (liczba FAT × sektory na FAT), katalog główny (nie dotyczy dla FAT32) oraz sektory ukryte, w tym sektor rozruchowy: spowodowałoby to nieużywanie sektorów na końcu woluminu. Jeśli partycja zawiera więcej sektorów niż całkowita liczba sektorów zajmowanych przez system plików, spowoduje to również nieużywanie sektorów na końcu partycji, po woluminie.



## Opis Dysku

Została utworzona i sformatowana partycja dysku twardego VDI 32MB o stałym rozmiarze. W trakcie partycjonowania dysku został wykorzystany tryb MBR (Master Boot Record) jako główny rekord rozruchowy. Rozmiar woluminu prostego to 29 MB.
Dysk otworzyliśmy w edytorze heksadecymalnym HxD pozwalającym na niskopoziomową edycję plików, dysków twardych i obrazów nośników CD/DVD.
Struktura dysku HDD przedstawia się w następujący sposób

## disk.vdi
```
.
├──$RECYCLE.BIN
│   └── desktop.ini
├──folder1
│   ├──folder2
│   │   └──plik.txt
│   └── plik123.txt
├──plik126.txt
└──System Volume Information
    ├──IndexerVolumeGuid
    └──WPSettings.dat
```




Poszukiwany plik nazywa sie “plik.txt”. Wpisano do pliku krótki tekst o treści “Wyciągnij mnie!”.

Zawartość pliku wyświetlona w programie HxD.




## Opis programu

### Struktura sektora rozruchowego FAT16
```
typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors_short; // if zero, later field is used
    unsigned char media_descriptor;
    unsigned short fat_size_sectors;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned long hidden_sectors;
    unsigned long total_sectors_long;
 
    unsigned char drive_number;
    unsigned char current_head;
    unsigned char boot_signature;
    unsigned long volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat16BootSector;
```
- jmp[3] - instrukcja skoku; 3 bajty 
- oem[8] -  ta wartość określa, w którym systemie sformatowano dysk; 
- sector_size - Bajtów na sektor logiczny w potęgach dwóch; najczęstszą wartością jest 512; 2 bajty
- sectors_per_cluster - Sektory logiczne na klaster. Dozwolone wartości to 1, 2, 4, 8, 16, 32, 64 i 128
- reserved_sectors - Liczba zarezerwowanych sektorów logicznych . Liczba sektorów logicznych przed pierwszym FAT w obrazie systemu plików
- number_of_fats - Liczba tabel alokacji plików. Prawie zawsze 2
- root_dor_entries - Maksymalna liczba pozycji katalogu głównego FAT16.
- total_sectors_short - Suma sektorów logicznych
- media_descriptor - Deskryptor mediów. Ta wartość musi odzwierciedlać deskryptor nośnika zapisany (we wpisie dla klastra 0 ) w pierwszym bajcie każdej kopii FAT.
- fat_size_sectors - Sektory logiczne według tabeli alokacji plików dla FAT16; 2 bajty
- sectors_per_track - Sektory fizyczne na ścieżkę; 2 bajty
- number_of_heads - Liczba głowic dla dysków; 2 bajty
- hidden_sectors - Liczba ukrytych sektorów poprzedzających partycję zawierającą ten wolumin FAT; 2 bajty
- total_sectors_long - Suma sektorów logicznych, w tym sektorów ukrytych.; 2 bajty
- drive_number - Numer dysku fizycznego
- current_head - 
- boot_signature - Rozszerzony podpis rozruchowy
- volume_id - Identyfikator woluminu (numer seryjny)
- volume_label[11] - Etykieta woluminu partycji, wypełniona spacjam; 11 bajtów
- fs_type[8] - Typ systemu plików, wypełniony spacjami ; 8 bajtów
- boot_code[448] - 
- boot_sector_signature - Podpis sektora rozruchowego, wskazuje kod rozruchowy zgodny z IBM PC; 2 bajty
 


## Struktura pliku lub katalogu w FAT16
```
typedef struct {
    unsigned char filename[8];
    unsigned char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short starting_cluster;
    unsigned long file_size;
} __attribute((packed)) Fat16Entry;
```


- filename[8] - nazwa pliku. Pierwszy bajt nazwy pliku wskazuje jego status.; 8 bajtów
- ext[3] - rozszerzenie nazwy pliku; 3 bajty
- attributes -  informacje o pliku i jego uprawnieniach itp.  Każdy bit jest podany jako jego wartość liczbowa i są one łączone w celu uzyskania rzeczywistej wartości atrybutu:
  - 0x01 Wskazuje, że plik jest tylko do odczytu.
  - 0x02 Wskazuje ukryty plik. Takie pliki można wyświetlić, jeśli jest to naprawdę wymagane.
  - 0x04 Wskazuje plik systemowy. Te też są ukryte.
  - 0x08 Wskazuje specjalny wpis zawierający etykietę woluminu dysku zamiast opisu pliku. Ten rodzaj wpisu pojawia się tylko w katalogu głównym.
  - 0x10 Wpis opisuje podkatalog.
  - 0x20 To jest flaga archiwum. Może to być ustawione i wyczyszczone przez programistę lub użytkownika, ale zawsze jest ustawiane podczas modyfikacji pliku. Jest używany przez programy do tworzenia kopii zapasowych.
  - 0x40 Nieużywany; musi być ustawiony na 0.
  - 0x80 Nieużywany; musi być ustawiony na 0.
- reserved[10] - Liczba zarezerwowanych sektorów logicznych; 10 bajtów 
- modify_time - czas utworzenia lub ostatniej aktualizacji pliku. Czas jest odwzorowywany w bitach w następujący sposób; pierwsza linia wskazuje przesunięcie bajtu, druga linia wskazuje (dziesiętnie) poszczególne liczby bitów w wartości 16 bitów, a trzecia linia wskazuje, co jest przechowywane w każdym bicie; 2 bajty
- modify_date - datę utworzenia lub ostatniej aktualizacji pliku. Data jest odwzorowana w bitach w następujący sposób; pierwsza linia wskazuje przesunięcie bajtu, druga linia wskazuje (dziesiętnie) poszczególne liczby bitów w wartości 16 bitów, a trzecia linia wskazuje, co jest przechowywane w każdym bicie; 2 bajty
- starting_cluster - Pierwszy klaster dla przestrzeni danych na dysku jest zawsze numerowany jako 0x0002. Ten dziwny układ wynika z tego, że pierwsze dwa wpisy w FAT są zarezerwowane do innych celów; 2 bajty
- file_size - rzeczywisty rozmiar pliku w bajtach; 2 bajty



## Struktura pomocnicza, ułatwiająca przekazywanie parametrów do funkcji
```
typedef struct {
    Fat16BootSector bs;
    FILE *file;
    Fat16Entry currentDirEntry;
    unsigned long fatStart;
    unsigned long rootStart;
    unsigned long dataStart;
} fatFile;
```
- bs - Sektor rozruchowy FAT16
- file - wskaźnik do pliku, do którego wczytana jest zawartość FAT16
- currentDirEntry - struktura reprezentująca katalog, w którym obecnie się znajdujemy
- fatStart - przesunięcie w bajtach do początku regionu FAT
- rootStart - przesunięcie w bajtach do początku regionu katalogu głównego
- dataStart - przesunięcie w bajtach do początku regionu danych FAT16 



## Opis funkcji:


```
int openFat16File(fatFile *fat, char *filename)
```
- funkcja otwiera plik FAT16 o nazwie filename odczytuje sektor rozruchowy i przydatne informacje z sektora  zapisuje do struktury fatFile *fat:
  - fatFile *fat - wskaźnik do struktury fatFile, 
  - char *filename - łańcuch znaków tworzący nazwę pliku otwieranego


Działanie funkcji przedstawia się w kilku krokach:
1. Plik FAT16 otwierany jest w trybie read/write bytes
2. Pierwszy w kolejności wczytany zostaje sektor rozruchowy
3. W oparciu o informacje zapewnione przez sektor rozruchowy obliczane są parametry:
  - fat->fatStart = obecna pozycja pliku + (ilość zarezerwowanych sektorow - 1) * rozmiar sektora
  - fat->rootStart = fat->fatStart + rozmiar sektorów * ilość tablic FAT * rozmiar sektora
  - fat->dataStart = fat->rootStart + fat->ilość katalogów głównych * rozmiar katalogu głównego ustawiany jest wskaźnik fat->currentDir na poczatek katalogu głównego


Wartość zwracana:
- Jeśli otwarcie pliku się nie powiedzie zwracana jest wartość 1
- Jeśli ładowanie bootsectora się nie powiedzie, zwracana jest wartość 2
- W przypadku sukcesu zwracana jest wartość 0


- void closeFat16File(fatFile *fat) -  funkcja zamyka plik FAT. Plik jest jednym z parametrów struktury fatFile:
  - fatFile *fat - wskaźnik do struktury fatFile, z której brany jest plik

Wartość zwracana:
- void

```
void print_file_info(Fat16Entry *entry) 
```
- wypisuje informacje o pliku, który został odnaleziony w tablicy alokacji FAT16:
Fat16Entry *entry - wskaźnik do struktury Fat16Entry reprezentującej odnaleziony plik w tablicy alokacji FAT16

Wartość zwracana:
	void




```
Fat16Entry *findEntry(fatFile *fat, char *entryName, entryType type)
``` 
- funkcja przeszukuje region danych lub region katalogu głównego w poszukiwaniu wpisu katalogowego o nazwie entryName w obrębie katalogu w którym się znajduje
fatFile *fat - wskaźnik do struktury fatFile. Parametr fat->file 
char *entryName - łańcuch znaków będący nazwą poszukiwanego wpisu
entryType type - wyliczeniowy typ danych entryType, który oznacza typ wejścia. Program sprawdza atrybut każdego wpisu, po którym stwierdza z jakim typem wpisu ma do czynienia. Mamy następujące typy:
ENTRY_FREE - puste miejsce w tablicy FAT16
ENTRY_DIR - katalog; wartość atrybutu 0x10 
ENTRY_FILE - plik; wartość atrybutu 0x80

zwracana wartość:
	Jeśli operacja się nie powiedzie, zwracana jest wartośc NULL
	W przypadku sukcesu, zwracany jest wskaźnik do struktury Fat16Entry


```
int changeDir(fatFile *fat, char *dirname) 
```
- funkcja służy do przechodzenia w podrzędny katalog o podanej nazwie
fatFile *fat - wskaźnik do struktury fatFile. Wskaźnik fat->currentDirEntry jest ustawiany na pierwszy wpis poszukiwanego katalogu, natomiast wskaźnik do pliku ustawiany jest na początek klastra zawartością katalogu.
char *dirname - łańcuch znaków zawierający nazwę poszukiwanego katalogu podrzędnego
	
wartość zwracana:
	Jeśli szukany katalog nie istnieje zwracana jest wartość 1
	W przypadku sukcesu zwracana jest wartość 0


```
void gotoRootDir(fatFile *fat)
``` 
- funkcja służy do powrotu do głównego katalogu
fatFile *fat - wskaźnik do struktury fatFile. Parametr fat->currentDirEntry tej struktury otrzymuje nową wartość, którą jest struktura reprezentująca katalog główny.

wartość zwracana:
void


```
int readFile(fatFile *fat, char *filename, char **content) 
```
- funkcja ładuje całą zawartość pliku do struktury fatFile
fatFile *fat - wskaźnik do struktury fatFile. Parametry struktury wykorzystywane są do alokacji zasobów oraz do odczytu pliku.
char *filename - łancuch znaków zawierający nazwę pliku, z którego odczytywana jest zawartość
char **content - wskaźnik do łańcuchu znaków do którego będzie wpisana zawartość pliku

wartość zwracana:
	Jeśl nazwa pliku jest niepoprawna, zwracana jest wartość 1
	W przypadku sukcesu zwracana jest wartość 0


```
int saveNewFile(fatFile *fat, char *filename, char *content)
``` 
- funkcja zapisuje dane z pamięci do nowego pliku.
fatFile *fat - wskaźnik do struktury fatFile. Parametry struktury wykorzystywane są do zapisu do pliku.
char *filename - łańcuch znaków zawierający nazwę pliku
char *content - łańcuch znaków zawierający zawartość zawartość pliku

wartość zwracana:
	Jeśli otwarcie pliku się nie powiedzie, zwracana jest wartość 1
	W przypadku sukcesu zwracana jest wartość 0





Dodatkowe linki

https://codeandlife.com/2012/04/02/simple-fat-and-sd-tutorial-part-1/?fbclid=IwAR3uNni-DOeCwt97q92uRVUruy0YLwvA9ZqmvofaDl0Wrq0JZekAAEr-7Vo

http://elm-chan.org/docs/fat_e.html?fbclid=IwAR2wCcknc_6IVRkc1hVA3azd-OT9HBjjTOyKk99SxfrHjmbYjQ_WSeTdJNQ

https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system


