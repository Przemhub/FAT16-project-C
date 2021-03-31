#include "FAT16.cpp"
#include <stdio.h>
#include <stdlib.h>



int main()
{
    fatFile fat;
    int ret;
    ret = openFat16File(&fat, "disk.vdi");
    if(ret == 1)
    {
        printf("Blad otwarcia pliku!\n");
    }
    else if(ret == 2)
    {
        printf("Blad wczytania bootsectora!\n");
    }
    else if(ret == 0)
    {
        ret = changeDir(&fat, "folder1");
        if(ret)
        {
            printf("Blad zmiany katalogu!\n");
        }
        else
        {
            ret = changeDir(&fat, "folder2");
            if(ret)
            {
                printf("Blad zmiany katalogu!\n");
            }
            else
            {
                char* content;
                ret = readFile(&fat, "plik.txt", &content);
                if(ret)
                {
                    printf("Blad odczytu pliku!\n");
                }
                else
                {
                    gotoRootDir(&fat);
                    ret = saveNewFile(&fat, "plik7.txt", content);
                    if(ret)
                    {
                        printf("Blad zapisu do pliku!\n");
                    }
                    else
                    {
                        printf("Sukces!\n");
                    }
                    free(content);
                }
            }
        }
    }
    closeFat16File(&fat);
    return 0;
}
