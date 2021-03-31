#include "FAT16.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define GOTO_CLUSTER(fat, cluster) fseek(fat->file, fat->dataStart + (cluster - 2) * fat->bs.sectors_per_cluster * fat->bs.sector_size, SEEK_SET)
 ///(fat->bs.sectors_per_cluster * fat->bs.sector_size) to rozmiar klastra w bajtach





char validCharacter(char c)
{
    const char allowedSpecChars[] = {'!', '#', '$', '%', '&', '\'', '(', ')', '-', '@', '^', '_', '`', '{', '}', '~', 0};

    if((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || (c >= 128 && c <= 255 && c != 229))// 229 == 0xE5
        return c;


    if(c >= 'a' && c <= 'z')//konwertowanie na du�e
        return c - 0x20; // 32


    for(int i = 0; allowedSpecChars[i] != 0; ++i)
    {
        if(c == allowedSpecChars[i])
            return c;
    }

    return 0;
}
char* validateEntryName(const char* name)
{ ///funkcja zwraca nazw� w postaci 8+3 (bez kropki) z uzupe�nionymi spacjami bez ma�ych liter, je�eli jest poprawna lub NULL, gdy jest niepoprawna
    if(!name) ///gdy wska�nik jest pusty
        return NULL;
	
    int len = strlen(name);
    if(len > 12 || len <= 0) ///nazwa pliku mo�e mie� maks. 8 znak�w bez rozszerzenia, lub 12 (8+3+1) z kropk�
        return NULL;

    char* formatedName = (char*) malloc(11);
    memset(formatedName, ' ', 11);

    if(len == 1 && (formatedName[0] = name[0]) == '.') ///jest to wpis do obecnego katalogu - poprawny
        return formatedName;
    if(len == 2 && (formatedName[0] = name[0]) == '.' && (formatedName[1] = name[1]) == '.') ///jest to wpis do katalogu nadrz�dnego - poprawny
        return formatedName;

    int dotPos = len; ///domy�lna warto�c, kt�ra oznacza brak kropki w pliku
    for(int i = 0; i < len; ++i)
    {
        if(name[i] == '.')
        {
            if(dotPos == len)
                dotPos = i;
            else
            {
                free(formatedName);
                return NULL; ///druga kropka w pliku - niedozwolona
            }
        }
    }
    
    if(dotPos > 8 || len - (dotPos + 1) > 3 || len - 1 == dotPos) ///za d�uga nazwa || za d�ugie rozszerzenie || kropka znajduje si� na ko�cu
    {
        free(formatedName);
        return NULL;
    }

    memcpy(formatedName, name, dotPos); ///nazwa
    if(dotPos < len)
        memcpy(formatedName + 8, name + dotPos + 1, len - (dotPos + 1)); ///rozszerzenie

    for(int i = 0; i < 11; ++i)
    {
        if(i == 0 && formatedName[0] == 0xE5)
            formatedName[0] = 0x05;
        else if( !(formatedName[i] = validCharacter(formatedName[i])) ) ///zwr�cona warto�� 0 oznacza niepoprawny znak
        {
            free(formatedName);
            return NULL;
        }
    }
    return formatedName;
}

unsigned short allocateNewCluster(fatFile *fat, unsigned short lastCluster)
{ ///funkcja alokuje nowy klaster i ustawia wska�nik w pliku na now� przestrze�; gdy nie zaalokuje pozycja wska�nika niezdefiniowana (prawdopodobnie pocz�tek katalogu root)
    int FATentriesCount = fat->bs.fat_size_sectors * fat->bs.sector_size / 2; ///ilo�� wis�w w tablicy FAT
	
    if(!fat || lastCluster >= FATentriesCount)
        return 0; ///niepoprawne dane

    unsigned short FATentry;
    fseek(fat->file, fat->fatStart + 2 * 2, SEEK_SET); ///ustaw wska�nik na drugi wpis w tablicy FAT
    for(unsigned short i = 2; i < FATentriesCount; ++i) ///poszukujemy pustego wpisu w tablicy FAT dop�ki nie sko�czy si� tablica lub nie znajdziemy pustego klastra
    {
        fread(&FATentry, sizeof(unsigned short), 1, fat->file);
        if(FATentry == 0) ///gdy znajdziemy pusty
        {
            for(int c = 0; c < fat->bs.number_of_fats; ++c) ///nadpisywanie we wszystkich kopiach FAT
            {
                fseek(fat->file, fat->fatStart + c * fat->bs.fat_size_sectors * fat->bs.sector_size + 2 * i, SEEK_SET);//cofamy si�
                unsigned short address = 0xFFFF;
                fwrite(&address, sizeof(unsigned short), 1, fat->file); ///cofnij si� i nadpisz 0x0000 warto�ci� 0xFFFF oznaczaj�c� koniec �a�cucha klastr�w
            }

            if(lastCluster >= 2) ///gdy plik mia� ju� jakie� klastry
            {
                for(int c = 0; c < fat->bs.number_of_fats; ++c) ///nadpisywanie we wszystkich kopiach FAT
                {
                    fseek(fat->file, fat->fatStart + c * fat->bs.fat_size_sectors * fat->bs.sector_size + 2 * lastCluster, SEEK_SET);
                    fwrite(&i, sizeof(unsigned short), 1, fat->file); ///wpisujemy na stary koniec indeks nowego klastra
                }
            }

            GOTO_CLUSTER(fat, i);
            for(int o = 0; o < fat->bs.sectors_per_cluster * fat->bs.sector_size; ++o)
                putc(0, fat->file); ///inicjalizujemy klaster zerami
            GOTO_CLUSTER(fat, i); ///ustawiamy wska�nik na pocz�tku nowego klastra
            return i;
        }
    }
    return 0; ///gdy nie znajdziemy pustego
}

void destroyClusterChain(fatFile *fat, unsigned short startCluster)
{
    if(!fat)
        return;
    int FATentriesCount = fat->bs.fat_size_sectors * fat->bs.sector_size / 2;
    unsigned short FATentry;
    while(startCluster >= 2 && startCluster < FATentriesCount) ///zerowanie wpis�w w tablicy FAT dop�ki nie napotkamy ko�ca lub niepoprawnego wpisu
    {
        fseek(fat->file, fat->fatStart + 2 * startCluster, SEEK_SET);
        fread(&FATentry, sizeof(unsigned short), 1, fat->file); ///odczytanie indeksu nast�pnego wpisu
        for(int c = 0; c < fat->bs.number_of_fats; ++c) ///zerowanie w ka�dej kopii FAT
        {
            fseek(fat->file, fat->fatStart + c * fat->bs.fat_size_sectors * fat->bs.sector_size + 2 * startCluster, SEEK_SET);
            unsigned short address = 0x00000;
            fwrite(&address, sizeof(unsigned short), 1, fat->file);
        }
        startCluster = FATentry;
    }
}

unsigned int nextCluster(fatFile *fat, unsigned short lastCluster, int allocateIfLast)
{ ///zwraca indeks nast�pnego klastra, ewentualnie alokuj�c nowy oraz ustawia wska�nik w pliku na klaster danych; w przeciwnym wypadku zwraca 0
    int FATentriesCount = fat->bs.fat_size_sectors * fat->bs.sector_size / 2; ///ilo�� wis�w w tablicy FAT

    if(!fat || lastCluster >= FATentriesCount) ///niepoprawne dane
        return 0;

    if(lastCluster < 2) ///je�eli "nie by�o" ostatniego klastra ...
    {
        if(allocateIfLast)
            return allocateNewCluster(fat, lastCluster); ///... to mo�na zaalokowoa� nowy...
        else
            return 0; ///... albo zwr�ci� 0, je�eli nie chcemy alokowa� nowego
    }

    unsigned short FATentry;
    fseek(fat->file, fat->fatStart + 2 * lastCluster, SEEK_SET);
    fread(&FATentry, sizeof(unsigned short), 1, fat->file); ///sprawwd� jaka warto�� si� znjaduje w tablicy FAT
    if((FATentry & 0xFFF8) == 0xFFF8) //0xFFF8 - 0xFFFF to EOC (end of chain)
    {
        if(allocateIfLast)
            return allocateNewCluster(fat, lastCluster); ///je�eli mo�na alokowa�, to alokujemy
        else
            return 0; ///koniec �a�cucha - to by� ostatni klaster
    }
    if(FATentry >= 2 && FATentry < FATentriesCount)
    {
        GOTO_CLUSTER(fat, FATentry);
        return FATentry;
    }
    return 0;
}



int openFat16File(fatFile *fat, char *filename) ///done ============
{
    fat->file = fopen(filename, "r+b"); ///otwiera plik w trybie binarnego zapis/odczytu je�li istnieje
    if(!fat->file) ///w przypadku b��du otwarcia, zwraca 1
        return 1;

    if(fread(&(fat->bs), sizeof(Fat16BootSector), 1, fat->file) != 1) ///w przypadku nie odczytania boot sectora, zwraca 2
    {
        fclose(fat->file); ///zamyka od razu plik
        return 2;
    }

    ///ustawia odpowiednie offset'y
    int entry_dir_size = 32;
    fat->fatStart = ftell(fat->file) + (fat->bs.reserved_sectors - 1) * fat->bs.sector_size;
    fat->rootStart = fat->fatStart + fat->bs.fat_size_sectors * fat->bs.number_of_fats * fat->bs.sector_size;
    fat->dataStart = fat->rootStart + fat->bs.root_dir_entries * entry_dir_size; 
	//printf("\n%d",fat->bs.reserved_sectors);
    gotoRootDir(fat); ///ustawia fat->file oraz fat->currentDirEntry
    return 0;
}

void closeFat16File(fatFile* fat) ///zamyka plik - nic interesuj�cego
{
    fclose(fat->file);
}

void print_file_info(Fat16Entry *entry) ///raczej done
{
    if(memcmp(entry->ext, "   ", 3))
        printf("File name: %.8s.%.3s\n", entry->filename, entry->ext);
    else
        printf("File name: %.8s\n", entry->filename);
    printf("Attributes: 0x%02x\n", entry->attributes);
    //printf("Reserved: 0x%020x\n", entry->reserved);
    printf("Modify: %hi-%hi-%hi %hi:%02hi:%02hi\n", (entry->modify_date >> 9 & 0x7F) + 1980, entry->modify_date >> 5 & 0xF, entry->modify_date & 0x1F, entry->modify_time >> 11, (entry->modify_time >> 5) & 0x3F, (entry->modify_time & 0x1F) * 2);
    printf("Starting cluster: 0x%04x\n", entry->starting_cluster);
    printf("File size: %li\n\n", entry->file_size);
}

Fat16Entry *findEntry(fatFile *fat, char *entryName, entryType type)
{
    if(!fat || !entryName)
        return NULL;

    char* validName = validateEntryName(entryName);
    if(!validName)
        return NULL;

    Fat16Entry *entry = (Fat16Entry*)malloc(sizeof(Fat16Entry));
    unsigned short currCluster = fat->currentDirEntry.starting_cluster; ///indeks obecnego klastra
    int inRoot = (currCluster == 0); ///czy znajdujemy si� w root
    int N; ///ilo�� wpis�w w root lub w pojedynczym klastrze dla zwyk�ego podkatalogu
    if(inRoot)
    {
        N = fat->bs.root_dir_entries; ///katalog root ma sta�� maksymaln� ilo�� wpis�w katalogowych
        fseek(fat->file, fat->rootStart, SEEK_SET);
    }
    else
    {
        N = fat->bs.sectors_per_cluster * fat->bs.sector_size / 32; ///ilo�� wpis�w w pojedynczym klastrze
        GOTO_CLUSTER(fat, currCluster);
       
    }

    int i = 0;
    int stop = 0; ///flaga: czy ko�czy� szukanie?

    ///przydatne dla szukania pustego wpisu:
    int found = 0; ///flaga: czy znaleziono?
    unsigned long entryPos; ///pozycja znalezionego wpisu

    while(!stop)
    {
        fread(entry, sizeof(Fat16Entry), 1, fat->file);
        if(entry->filename[0] == 0) ///je�eli pierwszy bajt nazwy jest zerem, to ko�czymy przeszukiwanie
        {
            stop = 1;
        }
        else
        {
            if(type == ENTRY_FREE && !found && entry->filename[0] == 0xE5) ///usuni�ty plik
            {
                found = 1;
                entryPos = ftell(fat->file);
            }
            if(!memcmp(entry->filename, validName, 11)) ///je�eli wpis posiada szukan� nazw�
            {
                if(type == ENTRY_FREE) ///plik o danej nazwie ju� istnieje
                {
                    free(validName);
                    free(entry);
                    return NULL;
                }
                if(entry->attributes & 0x10 ? type == ENTRY_DIR : type == ENTRY_FILE) ///jesli wpis = katalog, to czy szukamy katalogu
                {
                    free(validName);
                    fseek(fat->file, -sizeof(Fat16Entry), SEEK_CUR);
                    return entry;
                }
            }
            ++i;
            if(i >= N) ///doszli�my do ko�ca...
            {
                if(inRoot) ///...katalogu root
                {
                    stop = 1;
                }
                else ///... zwyk�ego podaktalogu
                {
                	
                    if( (currCluster = nextCluster(fat, currCluster, (type == ENTRY_FREE) && !found)) ) ///przechodzimy do nast�pnego klastra, je�eli istnieje; pozwalamy zaalokowa� nowy klaster, je�eli szukamy pustego, a jeszce nie znale�li�my
                    {
                        i -= N;
                    }
                    else ///nie ma nast�pnego klastra (ewentualnie nie uda�o si� go zaalokowa� w przypadku poszukiwania pustego wpisu)
                    {
                        stop = 1;
                    }
                }
            }
        }
    }

    if(type == ENTRY_FREE) ///dla pozosta�ych typ�w, je�eli zosta� znaleziony wpis, warto�� zosta�a zwr�cona wewn�trz p�tli
    {
        memset(entry, 0, sizeof(Fat16Entry));
        memcpy(entry, validName, 11); ///wyzerowanie wpisu i skopiowanie do niego nazwy
        if(found)
        {
            fseek(fat->file, entryPos, SEEK_SET);
        }
        fseek(fat->file, -sizeof(Fat16Entry), SEEK_CUR); ///ustawienie wska�nika w odpowiednim miejscu
        free(validName);
        return entry;
    }

    free(validName);
    free(entry);
    return NULL;
}

int changeDir(fatFile *fat, char *dirname) ///done ============
{
    Fat16Entry *entry = findEntry(fat, dirname, ENTRY_DIR); ///szuka wpisu, je�eli nie znajdzie zwraca 1
    if(!entry)
        return 1;

    memcpy(&fat->currentDirEntry, entry, sizeof(Fat16Entry)); ///ustawia fat->currentDirEntry na znaleziony wpis
    free(entry);

    if(!fat->currentDirEntry.starting_cluster) ///sprawdza, czy to root (starting_cluster == 0)
        fseek(fat->file, fat->rootStart, SEEK_SET); ///je�eli tak, to ustawia na wska�nik w pliku na rootStart
    else ///je�eli zwyk�y podkatalog, to ustawia na odpowiedni klaster
        GOTO_CLUSTER(fat, fat->currentDirEntry.starting_cluster);
    return 0;
}

void gotoRootDir(fatFile *fat) ///done ============
{
    memset(&fat->currentDirEntry, 0, sizeof(Fat16Entry));
    fat->currentDirEntry.attributes |= 0x10; ///tworzy sztuczny wpis reprezentuj�cy katalog root

    fseek(fat->file, fat->rootStart, SEEK_SET); ///ustawia wska�nik w pliku na rootStart
}

int readFile(fatFile *fat, char *filename, char **content) //odczytuje szukany plik o nazwie filename i wpisuje zawartosc do content
{
    Fat16Entry *entry = findEntry(fat, filename, ENTRY_FILE);
    if(!entry)
        return 1;
	
    unsigned short currCluster = entry->starting_cluster;
    if(currCluster < 0x0002 || currCluster > 0xFFEF) ///pusty plik lub niepoprawna warto�� pocz�tkowego klastra
    {
        free(entry);
        return 1;
    }
    GOTO_CLUSTER(fat, currCluster); ///przej�cie do pocz�tkowego klastra
    unsigned long size = entry->file_size; ///ilo�� bajt�w w pliku
    *content = (char*)malloc(size + 1);
    (*content)[entry->file_size] = 0; ///znak 0 ko�cz�cy �a�cuch znak�w

    unsigned int clusterSz = fat->bs.sectors_per_cluster * fat->bs.sector_size; ///rozmiar klastra

    int i = 0;
    for(; size > clusterSz; ++i, size -= clusterSz) ///wczytywanie klaster po klastrze
    {
        fread(*content + i * clusterSz, clusterSz, 1, fat->file);
        if( !(currCluster = nextCluster(fat, currCluster, 0)) ) ///przej�cie do kolejnego klastra
        {
            free(entry);
            free(*content);
            return 1; ///b��d przy przej�ciu do kolejnego klastra
        }
    }
		
    fread(*content + i * clusterSz, size, 1, fat->file); ///wczytanie ostatniego, niepe�nego klastra
    free(entry);
    return 0;
}

int saveNewFile(fatFile *fat, char *filename, char *content)
{
    Fat16Entry *entry = findEntry(fat, filename, ENTRY_FREE);
    if(!entry)
        return 1;
    unsigned long entryPos = ftell(fat->file); ///zapmai�tanie miejsca wolnego wpisu

    const unsigned long len = strlen(content); ///d�ugo�� zawarto�ci
    unsigned long size = len; ///d�ugo�� zawarto�ci
    if(!size) ///plik pusty
    {
        free(entry);
        return 0; ///nie alokujemy �adnych klastr�w, tworzymy jedynie wpis
    }
    const unsigned short startCluster = allocateNewCluster(fat, 0); ///alokuje klaster i ustawia wska�nik w pliku na jego pocz�tek
    if(!startCluster)
    {
        free(entry);
        return 1;
    }
    unsigned short currCluster = startCluster;

    unsigned int clusterSz = fat->bs.sectors_per_cluster * fat->bs.sector_size; ///rozmiar klastra

    int i = 0;
    for(; size > clusterSz; ++i, size -= clusterSz) ///zapisywanie klaster po klastrze
    {
        fwrite(content + i * clusterSz, clusterSz, 1, fat->file);
        if( !(currCluster = allocateNewCluster(fat, currCluster)) ) ///alokacja i przej�cie do kolejnego klastra
        {
            destroyClusterChain(fat, startCluster);
            free(entry);
            return 1; ///b��d przy alokacji kolejnego klastra
        }
    }
    fwrite(content + i * clusterSz, size, 1, fat->file); ///zapisanie ostatniego, niepe�nego klastra

    time_t t = time(NULL);
    struct tm *T = localtime(&t);
    entry->modify_date = T->tm_year - 80;
    entry->modify_date = (entry->modify_date << 4) | (T->tm_mon + 1);
    entry->modify_date = (entry->modify_date << 5) | T->tm_mday;
    entry->modify_time = T->tm_hour;
    entry->modify_time = (entry->modify_time << 6) | T->tm_min;
    entry->modify_time = (entry->modify_time << 5) | (T->tm_sec / 2);
    free(T);

    entry->attributes |= 0x20;
    entry->starting_cluster = startCluster;
    entry->file_size = len;
    fseek(fat->file, entryPos, SEEK_SET);
    fwrite(entry, sizeof(Fat16Entry), 1, fat->file);
    free(entry);
    return 0;
}
