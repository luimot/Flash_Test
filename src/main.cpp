#include "mbed.h"
#include "SPIFBlockDevice.h"

SPIFBlockDevice spif(PA_12, PA_11, PA_5, PA_4);

BufferedSerial ser(USBTX, USBRX, 115200);

FileHandle *mbed::mbed_override_console(int fd) { return &ser; }

int main()
{
    printf("SPIF test\n");

    // Initialize the SPI flash device, and print the memory layout
    spif.init();
    printf("spif size: %llu\n", spif.size());
    printf("spif read size: %llu\n", spif.get_read_size());
    printf("spif program size: %llu\n", spif.get_program_size());
    printf("spif erase size: %llu\n", spif.get_erase_size());

    // Write "Hello World!" to the first block
    char *buffer = (char *)malloc(sizeof("buseta\n"));
    char *r_buffer = (char *)malloc(sizeof("buseta\n"));
    sprintf(buffer, "buseta\n");
    spif.erase(0, spif.get_erase_size());
    spif.program(buffer, 0, spif.get_erase_size());

    // Read back what was stored
    spif.read(r_buffer, 0, spif.get_erase_size());
    printf("Escreveu: %s", r_buffer);

    // Deinitialize the device
    spif.deinit();
    return 0;
}
