#include "mbed.h"
#include "SPIFBlockDevice.h"
#include "LittleFileSystem.h"
#define FORCE_REFORMAT true
#define BUFFER_MAX_LEN 11
#define SPI_FREQ 104e6
#define ROOT "/fs/"

SPIFBlockDevice *bd = new SPIFBlockDevice(PA_12, PA_11, PA_5, PA_4, SPI_FREQ);
LittleFileSystem fs("fs");
FILE *file;

BufferedSerial ser(USBTX, USBRX, 115200);

FileHandle *mbed::mbed_override_console(int fd) { return &ser; }

int err;

void test_SPIF();

void test_LFS();

void mountFS();

void openFile(char *path);

void messageOnCannotOpenFile(bool condition);

void writeData(char *path);

void messageOnError();

InterruptIn irq(USER_BUTTON);
void erase()
{
    printf("Initializing the block device... ");
    fflush(stdout);
    int err = bd->init();
    printf("%s\n", (err ? "Fail :(" : "OK"));
    if (err)
    {
        error("error: %s (%d)\n", strerror(-err), err);
    }

    printf("Erasing the block device... ");
    fflush(stdout);
    err = bd->erase(0, bd->size());
    printf("%s\n", (err ? "Fail :(" : "OK"));
    if (err)
    {
        error("error: %s (%d)\n", strerror(-err), err);
    }

    printf("Deinitializing the block device... ");
    fflush(stdout);
    err = bd->deinit();
    printf("%s\n", (err ? "Fail :(" : "OK"));
    if (err)
    {
        error("error: %s (%d)\n", strerror(-err), err);
    }
}

static auto erase_event = mbed_event_queue() -> make_user_allocated_event(erase);

int main()
{
    // test_LFS();
    test_SPIF();
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
    // Setup the erase event on button press, use the event queue
    // to avoid running in interrupt context
    irq.fall(std::ref(erase_event));

    char *fileName = "/fs/numbers.txt";
    mountFS();
    openFile(fileName);

    // Display the root directory
    printf("Opening the root directory... ");
    fflush(stdout);
    DIR *d = opendir("/fs/");
    printf("%s\n", (!d ? "Fail :(" : "OK"));
    if (!d)
    {
        messageOnError();
    }

    printf("root directory:\n");
    while (true)
    {
        struct dirent *e = readdir(d);
        if (!e)
        {
            break;
        }

        printf("    %s\n", e->d_name);
    }

    printf("Closing the root directory... ");
    fflush(stdout);
    err = closedir(d);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        messageOnError();
    }

    // Display the numbers file
    printf("Opening \"/fs/numbers.txt\"... ");
    fflush(stdout);
    file = fopen("/fs/numbers.txt", "r");
    printf("%s\n", (!file ? "Fail :(" : "OK"));
    if (!file)
    {
        messageOnError();
    }

    printf("numbers:\n");
    while (!feof(file))
    {
        int c = fgetc(file);
        printf("%c", c);
    }

    printf("\rClosing \"/fs/numbers.txt\"... ");
    fflush(stdout);
    err = fclose(file);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        messageOnError();
    }

    // Tidy up
    printf("Unmounting... ");
    fflush(stdout);
    err = fs.unmount();
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        error("error: %s (%d)\n", strerror(-err), err);
    }
}

void mountFS()
{
    printf("Mounting the File System... ");
    fflush(stdout);
    err = fs.mount(bd);
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

void openFile(char *path)
{
    // Opens file
    printf("Opening \"%s\"... \n", path);
    fflush(stdout);
    file = fopen(path, "r+");
    messageOnCannotOpenFile(file);
    writeData(path);
}

void messageOnCannotOpenFile(bool condition)
{
    if (!condition)
    {
        printf("Could not open file\n Maybe file doesn't exist or the path is incorrect\n\n");
    }
    else
    {
        printf("File open. OK.\n");
    }
}

void messageOnError()
{
    error("error: %s (%d)\n", strerror(errno), -errno);
}

void writeData(char *path)
{
    if (!file)
    { // Create the file if it doesn't exist
        printf("No file found, creating a new file... \n");
        fflush(stdout);
        file = fopen(path, "w+");
        messageOnCannotOpenFile(file);
        if (!file)
            messageOnError();

        for (int i = 0; i <= 10; i++)
        {
            printf("Writing numbers (%d/%d)... ", i, 10);
            fflush(stdout);
            err = fprintf(file, "    %d\n", i);
            if (err < 0)
            {
                printf("Fail :(\n");
                messageOnError();
            }
        }
        printf("OK\n");

        printf("Seeking file... \n");
        fflush(stdout);
        err = fseek(file, 0, SEEK_SET);
        printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
        if (err < 0)
        {
            messageOnError();
        }
    }
    // Go through and increment the numbers
    for (int i = 0; i <= 10; i++)
    {
        printf("\rIncrementing numbers (%d/%d)... ", i, 10);
        fflush(stdout);

        // Get current stream position
        long pos = ftell(file);

        // Parse out the number and increment
        char buf[BUFFER_MAX_LEN];
        if (!fgets(buf, BUFFER_MAX_LEN, file))
        {
            messageOnError();
        }
        char *endptr;
        int32_t number = strtol(buf, &endptr, 10);
        if (
            (errno == ERANGE) ||         // The number is too small/large
            (endptr == buf) ||           // No character was read
            (*endptr && *endptr != '\n') // The whole input was not converted
        )
        {
            continue;
        }
        number += 1;

        // Seek to beginning of number
        fseek(file, pos, SEEK_SET);

        // Store number
        fprintf(file, "    %d\n", number);

        // Flush between write and read on same file
        fflush(file);
    }
    printf("OK\n");

    // Close the file which also flushes any cached writes
    printf("Closing \"%s\"... ", path);
    fflush(stdout);
    err = fclose(file);
    printf("%s\n", (err < 0 ? "Fail :(" : "OK"));
    if (err < 0)
    {
        messageOnError();
    }
}