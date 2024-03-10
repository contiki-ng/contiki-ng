#include <stdbool.h>
#include <sys/types.h>

#include "dev/spi.h"
#include "NuMicro.h"

#if 0
#define DEBUG_CMDNAME 1
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while(0)
#endif

void
gpio_hal_arch_port_set_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
    PRINTF("set pin %d, %d\n", port, pin);
    GPIO_PIN_DATA_NS(port, pin) = 1;
}

void
gpio_hal_arch_port_clear_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
    PRINTF("clear pin %d, %d\n", port, pin);
    GPIO_PIN_DATA_NS(port, pin) = 0;
}

bool
spi_arch_has_lock(const spi_device_t *dev)
{
    return true;
}

bool spi_arch_is_bus_locked(const spi_device_t *dev)
{
    return false;
}

SPI_T *GetSPIPort(const spi_device_t *dev)
{
    switch(dev->spi_controller) {
        case 1:
            return SPI1;
        case 2:
            return SPI2;
        case 3:
            return SPI3;
        default:
            break;
    }
    return SPI0;
}

spi_status_t spi_arch_lock_and_open(const spi_device_t *dev)
{
    static int flag = 1;
    SPI_T *spi = GetSPIPort(dev);
    if (flag) {
        PB9 = 1;
        clock_wait(10);
        // don't use dev->spi_pol. it's not configurable.
        SPI_Open(spi, SPI_MASTER, SPI_MODE_0, 8, dev->spi_bit_rate);
        SPI_DisableAutoSS(spi);
        flag = 0;
    }
    return SPI_DEV_STATUS_OK;
}

spi_status_t spi_arch_close_and_unlock(const spi_device_t *dev)
{
    //SPI_T *spi = GetSPIPort(dev);
    //SPI_Close(spi);
    return SPI_DEV_STATUS_OK;
}

void spi_arch_transfer_write(const spi_device_t *dev, const uint8_t *data, int wlen)
{
    int i;
    SPI_T *spi = GetSPIPort(dev);
    for(i=0; i<wlen; ++i) {
        SPI_WRITE_TX(spi, data[i]);
        while(SPI_IS_BUSY(spi));
    }
    SPI_ClearRxFIFO(spi);
}

int spi_arch_transfer_read(const spi_device_t *dev, uint8_t *buf, int rlen)
{
    int i;
    SPI_T *spi = GetSPIPort(dev);
    while(SPI_IS_BUSY(spi));
    for (i=0; i<rlen; ++i) {
        SPI_WRITE_TX(spi, 0);
        while(SPI_IS_BUSY(spi));
        buf[i] = (uint8_t)SPI_READ_RX(spi);
    }
    return i;
}

#if DEBUG_CMDNAME
char *cmdname(uint8_t b)
{
    if (b == 2) {
        return "Page Program";
    } else if (b == 3) {
        return "Read";
    } else if (b == 5) {
        return "Read Status";
    } else if (b == 4) {
        return "Write Disable";
    } else if (b == 6) {
        return "Write Enable";
    } else if (b == 0x15) {
        return "Read Configuration";
    } else if (b == 0x20) {
        return "Sector Erase";
    } else if (b == 0x90) {
        return "MDID";
    } else if (b == 0xb9) {
        return "PD";
    } else if (b == 0xab) {
        return "RPD";
    }
    return "";
}
#endif

spi_status_t spi_arch_transfer(const spi_device_t *dev,
                               const uint8_t *data, int wlen,
                               uint8_t *buf, int rlen,
                               int ignore_len)
{
    if (rlen == 0 && wlen > 0) {
        PRINTF("write cmd: 0x%x,[%s] %dB\n", data[0], cmdname(data[0]), wlen);
        spi_arch_transfer_write(dev, data, wlen);
    } else if (rlen > 0 && wlen == 0) {
        //PRINTF("%s: rlen %d\n", __func__, rlen);
        spi_arch_transfer_read(dev, buf, rlen);
        //PRINTF("rlen %d, buf[0] 0x%x\n", rlen, buf[0]);
    } else if (ignore_len > 0) {
        PRINTF("ignore_len %d\n", ignore_len);
    } else {
        // PRINTF("rlen %d wlen %d ignorelen %d\n", rlen, wlen, ignore_len);
    }

    return SPI_DEV_STATUS_OK;
}
