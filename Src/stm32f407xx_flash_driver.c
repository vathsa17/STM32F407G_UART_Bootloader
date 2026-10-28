
#include "stm32f407xx.h"

/* STM32F407 Flash register base address */
#define FLASH_REG_BASE       (0x40023C00UL)

typedef struct
{
    volatile uint32_t ACR;       /* 0x00 */
    volatile uint32_t KEYR;      /* 0x04 */
    volatile uint32_t OPTKEYR;   /* 0x08 */
    volatile uint32_t SR;        /* 0x0C */
    volatile uint32_t CR;        /* 0x10 */
    volatile uint32_t OPTCR;     /* 0x14 */
} FLASH_RegDef_t;

#define FLASH_REG \
    ((FLASH_RegDef_t *)FLASH_REG_BASE)

/* Flash control register bits */
#define FLASH_CR_PG             (1UL << 0)
#define FLASH_CR_SER            (1UL << 1)
#define FLASH_CR_MER            (1UL << 2)
#define FLASH_CR_SNB_Pos        3U
#define FLASH_CR_SNB_Msk        (0xFUL << FLASH_CR_SNB_Pos)
#define FLASH_CR_PSIZE_Msk      (3UL << 8)
#define FLASH_CR_PSIZE_WORD     (2UL << 8)
#define FLASH_CR_STRT           (1UL << 16)
#define FLASH_CR_LOCK           (1UL << 31)

/* Flash status register bits */
#define FLASH_SR_EOP            (1UL << 0)
#define FLASH_SR_OPERR           (1UL << 1)
#define FLASH_SR_WRPERR          (1UL << 4)
#define FLASH_SR_PGAERR          (1UL << 5)
#define FLASH_SR_PGPERR          (1UL << 6)
#define FLASH_SR_PGSERR          (1UL << 7)
#define FLASH_SR_RDERR           (1UL << 8)
#define FLASH_SR_BSY             (1UL << 16)

#define FLASH_SR_ERROR_MASK      (FLASH_SR_OPERR  | \
                                  FLASH_SR_WRPERR | \
                                  FLASH_SR_PGAERR | \
                                  FLASH_SR_PGPERR | \
                                  FLASH_SR_PGSERR | \
                                  FLASH_SR_RDERR)

#define FLASH_SR_CLEAR_MASK      (FLASH_SR_EOP | \
                                  FLASH_SR_ERROR_MASK)

#define FLASH_TIMEOUT            10000000UL

static Flash_Status_t Flash_WaitForLastOperation(void)
{
    volatile uint32_t timeout = FLASH_TIMEOUT;

    while ((FLASH_REG->SR & FLASH_SR_BSY) != 0U)
    {
        if (timeout-- == 0U)
        {
            return FLASH_ERROR_BUSY_TIMEOUT;
        }
    }

    if ((FLASH_REG->SR & FLASH_SR_ERROR_MASK) != 0U)
    {
        FLASH_REG->SR = FLASH_SR_CLEAR_MASK;
        return FLASH_ERROR_OPERATION;
    }

    return FLASH_OK;
}

static Flash_Status_t Flash_Unlock(void)
{
    if ((FLASH_REG->CR & FLASH_CR_LOCK) != 0U)
    {
        FLASH_REG->KEYR = 0x45670123UL;
        FLASH_REG->KEYR = 0xCDEF89ABUL;
    }

    if ((FLASH_REG->CR & FLASH_CR_LOCK) != 0U)
    {
        return FLASH_ERROR_UNLOCK;
    }

    return FLASH_OK;
}

static void Flash_Lock(void)
{
    FLASH_REG->CR |= FLASH_CR_LOCK;
}

static Flash_Status_t Flash_EraseSector(uint32_t sector)
{
    Flash_Status_t status;

    /* Never allow erasure of bootloader sectors */
    if ((sector < FLASH_APP_FIRST_SECTOR) ||
        (sector > FLASH_APP_LAST_SECTOR))
    {
        return FLASH_ERROR_INVALID_SECTOR;
    }

    status = Flash_WaitForLastOperation();

    if (status != FLASH_OK)
    {
        return status;
    }

    /* Clear previous completion and error flags */
    FLASH_REG->SR = FLASH_SR_CLEAR_MASK;

    /*
     * Configure sector erase.
     * PSIZE = 32-bit programming width setting.
     * This setting is appropriate for a 2.7–3.6 V supply.
     */
    FLASH_REG->CR &= ~(FLASH_CR_PG |
                       FLASH_CR_SER |
                       FLASH_CR_MER |
                       FLASH_CR_SNB_Msk |
                       FLASH_CR_PSIZE_Msk);

    FLASH_REG->CR |= FLASH_CR_SER |
                     (sector << FLASH_CR_SNB_Pos) |
                     FLASH_CR_PSIZE_WORD;

    /* Start erase */
    FLASH_REG->CR |= FLASH_CR_STRT;

    status = Flash_WaitForLastOperation();

    /* Disable sector erase mode */
    FLASH_REG->CR &= ~FLASH_CR_SER;

    return status;
}

Flash_Status_t Flash_EraseApplication(void)
{
    Flash_Status_t status;

    status = Flash_Unlock();

    if (status != FLASH_OK)
    {
        return status;
    }

    for (uint32_t sector = FLASH_APP_FIRST_SECTOR; sector <= FLASH_APP_LAST_SECTOR; sector++)
    {
        status = Flash_EraseSector(sector);

        if (status != FLASH_OK)
        {
            break;
        }
    }

    /*
     * Lock Flash again after the operation.
     * If a busy timeout occurs, leave the controller unlocked
     * rather than changing CR while an operation may still be active.
     */
    if ((FLASH_REG->SR & FLASH_SR_BSY) == 0U)
    {
        Flash_Lock();
    }

    return status;
}



Flash_Status_t Flash_TestUnlockLock(void)
{
    Flash_Status_t status = Flash_Unlock();

    if (status != FLASH_OK)
    {
        return status;
    }

    Flash_Lock();

    return FLASH_OK;
}



Flash_Status_t Flash_ProgramBytes( uint32_t address, const uint8_t *data, uint32_t length)
{
    Flash_Status_t status;
    uint32_t offset = 0U;

    /* Validate arguments. */
    if ((data == 0) || (length == 0U))
    {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    /* Check that the starting address is inside application Flash. */
    if ((address < APP_FLASH_START) ||
        (address >= APP_FLASH_END))
    {
        return FLASH_ERROR_INVALID_ADDRESS;
    }

    /* Check the requested byte range without risking integer overflow. */
    if (length > (APP_FLASH_END - address))
    {
        return FLASH_ERROR_INVALID_ADDRESS;
    }

    /* STM32F407 word programming requires 4-byte alignment. */
    if ((address & 0x3U) != 0U)
    {
        return FLASH_ERROR_INVALID_ADDRESS;
    }

    /*
     * A partial final word is padded with 0xFF.
     * Ensure that the complete physical word remains in application Flash.
     */
    uint32_t finalWordSize = (length + 3U) & ~3U;

    if (finalWordSize > (APP_FLASH_END - address))
    {
        return FLASH_ERROR_INVALID_ADDRESS;
    }

    status = Flash_Unlock();

    if (status != FLASH_OK)
    {
        return status;
    }

    status = Flash_WaitForLastOperation();

    if (status != FLASH_OK)
    {
        if ((FLASH_REG->SR & FLASH_SR_BSY) == 0U)
        {
            Flash_Lock();
        }

        return status;
    }

    /* Clear old status flags and configure 32-bit programming. */
    FLASH_REG->SR = FLASH_SR_CLEAR_MASK;

    FLASH_REG->CR &= ~(FLASH_CR_SER |
                       FLASH_CR_MER |
                       FLASH_CR_SNB_Msk |
                       FLASH_CR_PSIZE_Msk);

    FLASH_REG->CR |= FLASH_CR_PSIZE_WORD;

    /* Enable programming mode. */
    FLASH_REG->CR |= FLASH_CR_PG;

    while (offset < length)
    {
        uint32_t word = 0xFFFFFFFFUL;
        uint32_t remaining = length - offset;
        uint32_t bytesToProgram =
            (remaining >= 4U) ? 4U : remaining;

        /*
         * Construct a little-endian word.
         * Unused bytes in the final word remain 0xFF.
         */
        for (uint32_t i = 0U; i < bytesToProgram; i++)
        {
            uint32_t shift = i * 8U;

            word &= ~(0xFFUL << shift);
            word |= ((uint32_t)data[offset + i] << shift);
        }

        /* Clear status flags before starting this word. */
        FLASH_REG->SR = FLASH_SR_CLEAR_MASK;

        /* Program the word to Flash. */
        *(volatile uint32_t *)(address + offset) = word;

        status = Flash_WaitForLastOperation();

        if (status != FLASH_OK)
        {
            break;
        }

        /* Verify the programmed word. */
        if (*(volatile uint32_t *)(address + offset) != word)
        {
            status = FLASH_ERROR_OPERATION;
            break;
        }

        offset += bytesToProgram;
    }

    /*
     * Do not modify the controller while it is busy.
     * Otherwise, disable programming and lock Flash.
     */
    if ((FLASH_REG->SR & FLASH_SR_BSY) == 0U)
    {
        FLASH_REG->CR &= ~FLASH_CR_PG;
        Flash_Lock();
    }

    return status;
}