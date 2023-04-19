#include "mbed.h"
#include "SPIFBlockDevice.h"
#include "LittleFileSystem.h"
#define FORCE_REFORMAT false
#define BUFFER_MAX_LEN 11
// #define DEBUG_WRITE
#define SPI_FREQ 104e6
#define ROOT "/fs/"

SPIFBlockDevice *bd = new SPIFBlockDevice(PA_12, PA_11, PA_5, PA_4, SPI_FREQ);
LittleFileSystem fs("fs");

DIR *d;
FILE *f;

BufferedSerial ser(USBTX, USBRX, 115200);

FileHandle *mbed::mbed_override_console(int fd) { return &ser; }

int err;

void test_SPIF();

void test_LFS();

void mountFS();

void unmountFS();

void openFile(string path, string operation = "r+");

void closeFile(string path);

void displayFile(string path);

void displayDirectory(string path);

void writeFile();

void onErrorMessageShow();

int main()
{
    test_LFS();
    // test_SPIF();
    return 0;
}

void test_SPIF()
{
    printf("SPIF test\n");

    // Initialize the SPI flash device, and print the memory layout
    bd->init();
    printf("spif size: %llu\n", bd->size());
    printf("spif read size: %llu\n", bd->get_read_size());
    printf("spif program size: %llu\n", bd->get_program_size());
    printf("spif erase size: %llu\n", bd->get_erase_size());

    // Write "Hello World!" to the first block
    char *buffer = (char *)malloc(sizeof("buseta\n"));
    char *r_buffer = (char *)malloc(sizeof("buseta\n"));
    sprintf(buffer, "buseta\n");
    bd->erase(0, bd->get_erase_size());
    bd->program(buffer, 0, bd->get_erase_size());

    // Read back what was stored
    bd->read(r_buffer, 0, bd->get_erase_size());
    printf("Escreveu: %s", r_buffer);

    // Deinitialize the device
    bd->deinit();
}

void test_LFS()
{
    printf("--- Starting LittleFileSystem Test ---\n");

    mountFS();

    string filePath = "/fs/numbers.txt";

    openFile(filePath);

    closeFile(filePath);

    displayDirectory("/fs/");

    displayFile(filePath);

    unmountFS();

    printf("Program ended correctly...\n");
}

void mountFS()
{
    // Try to mount the filesystem
    printf("Mounting the filesystem... \n");
    fflush(stdout);
    int err = fs.mount(bd);
    printf("%s\n", (err ? "Fail :(" : "OK"));
    if (err || FORCE_REFORMAT)
    {
        // Reformat if we can't mount the filesystem
        printf("formatting... \n");
        fflush(stdout);
        err = fs.reformat(bd);
        printf("%s\n", (err ? "Fail :(" : "OK"));
        if (err)
        {
            error("error: %s (%d)\n", strerror(-err), err);
        }
    }
}

void unmountFS()
{
    printf("Unmounting... ");
    fflush(stdout);
    err = fs.unmount();
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        error("error: %s (%d)\n", strerror(-err), err);
    }
}

void openFile(string path, string operation)
{
    f = fopen(path.c_str(), operation.c_str());
    if (!f)
    {
        // onErrorMessageShow();
        printf("No file found, creating a new file... \n");
        fflush(stdout);
        f = fopen(path.c_str(), "w+");
        if (!f)
        {
            onErrorMessageShow();
        }
        writeFile();
    }
}

void closeFile(string path)
{
    // Close the file which also flushes any cached writes
    printf("Closing \"%s\"... ", path.c_str());
    fflush(stdout);
    err = fclose(f);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        onErrorMessageShow();
    }
}

void displayDirectory(string path)
{
    // Display the root directory
    printf("Listing \"%s\"... \n", path.c_str());
    fflush(stdout);
    d = opendir(path.c_str());
    printf("%s\n", (!d ? "Fail :(" : "OK"));
    if (!d)
    {
        onErrorMessageShow();
    }
    while (true)
    {
        struct dirent *e = readdir(d);
        if (!e)
        {
            break;
        }

        printf("    %s\n", e->d_name);
    }

    fflush(stdout);
    err = closedir(d);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        onErrorMessageShow();
    }
}

void onErrorMessageShow()
{

    error("error: %s (%d)\n", strerror(errno), -errno);
}

void writeFile()
{
    for (int i = 0; i <= 10; i++)
    {
#ifdef DEBUG_WRITE
        printf("\rWriting numbers (%d/%d)... ", i, 10);
#endif
        fflush(stdout);
        err = fprintf(f, "    %d\n", i);
        if (err < 0)
        {
            printf("Fail :(\n");
            onErrorMessageShow();
        }
    }
#ifdef DEBUG_wRITE
    printf("\rWriting numbers (%d/%d)... OK\n", 10, 10);
#endif
    printf("Seeking file... ");
    fflush(stdout);
    err = fseek(f, 0, SEEK_SET);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        onErrorMessageShow();
    }
}

void displayFile(string path)
{
    // Display the numbers file
    printf("Displaying \"%s\"... \n", path.c_str());
    fflush(stdout);
    f = fopen(path.c_str(), "r");
    printf("%s\n", (!f ? "Fail :(" : "OK"));
    if (!f)
    {
        onErrorMessageShow();
    }
    // Listing numbers code block
    printf("numbers:\n");
    while (!feof(f))
    {
        int c = fgetc(f);
        printf("%c", c);
    }
    // Listing numbers code block
    fflush(stdout);
    err = fclose(f);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        onErrorMessageShow();
    }
}