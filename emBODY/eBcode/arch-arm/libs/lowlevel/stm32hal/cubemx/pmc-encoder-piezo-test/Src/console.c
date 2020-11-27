/*******************************************************************************************************************//**
 * @file    console.c
 * @author  G.Zini
 * @version 1.0
 * @date    2018 November, 9
 * @brief   Console management
 **********************************************************************************************************************/

/* Includes ***********************************************************************************************************/
#include <ctype.h>
#include "console.h"
#include "semphr.h"
#include "task.h"
#include "utilities.h"

#if defined USB_CONSOLE
    #include "usbd_vcom.h"
#elif defined USART1_CONSOLE
    #include "usart.h"
#elif defined USART3_CONSOLE
    #include "usart.h"
#else
    #error Console device must be defined in file config.h
#endif


/* Private macros *****************************************************************************************************/

// Maximum waiting time for the TX/RX operations
#define DEFAULT_TX_TIMEOUT  _ms_(100)
#define DEFAULT_RX_TIMEOUT  WAIT_FOREVER


/* Private variables **************************************************************************************************/

/* Mutex semaphore to lock the console */
static SemaphoreHandle_t coLockSemaphore = NULL ;


/* Exported functions *************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Initialize the console device
 * @param   void
 * @return  pdPASS      The console was successfully locked
 *          pdFAIL      It was not possible to lock the console in the time specified
 */
BaseType_t coInit( void )
{
#if defined USB_CONSOLE
    /* Init the virtual-com device */
    return (vcomInit() == VCOM_OK) ? pdPASS : pdFAIL ;
#elif defined USART1_CONSOLE
    /* Init USART1 device */
    return (USART_NO_ERRORS == USART1_Init())? pdPASS : pdFAIL ;
#elif defined USART3_CONSOLE
    /* Init USART3 device */
    return (USART_NO_ERRORS == USART3_Init())? pdPASS : pdFAIL ;
#else
    return pdPASS ;
#endif
}



/*******************************************************************************************************************//**
 * @brief   Transmit a character to the console device without translations
 * @param   ch      Character to be transmitted to the console device
 * @retval  char    Value of the argument 'ch' or '\0' in case of error
 */
char coTxChar( char ch )
{
#if defined USB_CONSOLE
    /* Transmit a charecter to virtual-com */
    return (char)vcomTxChar((uint8_t)ch) ;
#elif defined USART1_CONSOLE
    /* Transmit a charecter to USART1 */
    return (USART_NO_ERRORS == USART1_TxChar((const uint8_t *)&ch, DEFAULT_TX_TIMEOUT))? ch : '\0' ;
#elif defined USART3_CONSOLE
    /* Transmit a charecter to USART3 */
    return (USART_NO_ERRORS == USART3_TxChar((const uint8_t *)&ch, DEFAULT_TX_TIMEOUT))? ch : '\0' ;
#else
    return ch ;
#endif
}

/*******************************************************************************************************************//**
 * @brief   Receive a character from the console device without any translation
 * @param   void
 * @retval  char    Character received from the console or '\0' in case of error
 */
char coRxChar( void )
{
#if defined USB_CONSOLE
    /* Receive a charecter from virtual-com */
    return (char)vcomRxChar() ;
#elif defined USART1_CONSOLE
    char ch ;
    /* Receive a charecter from USART1 */
    return (USART_NO_ERRORS == USART1_RxChar((uint8_t *)&ch, DEFAULT_RX_TIMEOUT))? ch : '\0' ;
#elif defined USART3_CONSOLE
    char ch ;
    /* Receive a charecter from USART3 */
    return (USART_NO_ERRORS == USART3_RxChar((uint8_t *)&ch, DEFAULT_RX_TIMEOUT))? ch : '\0' ;
#else
    return '\0' ;
#endif
}

/*******************************************************************************************************************//**
 * @brief   Verify if one character is pending from the console device. The function does not block the caller
 * @param   void
 * @retval  - true  One or more characters from the console device have been received
 *          - false No characters are pending
 */
bool coRxReady( void )
{
#if defined USB_CONSOLE
    /* Check the virtual-com receiver fifo level */
    return (0 != vcomRxFifoLevel())? true : false ;
#elif defined USART1_CONSOLE
    /* Check the USART1 receiver fifo level */
    return (0 < USART1_RxFifoLevel())? true : false ;
#elif defined USART3_CONSOLE
    /* Check the USART3 receiver fifo level */
    return (0 < USART3_RxFifoLevel())? true : false ;
#else
    return false ;
#endif
}

/*******************************************************************************************************************//**
 * @brief   Verify the transmitter FIFO is empty. The function does not block the caller
 * @param   void
 * @retval  - true  All characters have been transmitted
 *          - false Some characters are still to be transmitted
 */
bool coTxReady( void )
{
#if defined USB_CONSOLE
    /* Check the virtual-com transmitter fifo level */
    return (0 == vcomTxFifoLevel())? true : false ;
#elif defined USART1_CONSOLE
    /* Check the USART1 transmitter fifo level */
    return (0 == USART1_TxFifoLevel())? true : false ;
#elif defined USART3_CONSOLE
    /* Check the USART3 transmitter fifo level */
    return (0 == USART3_TxFifoLevel())? true : false ;
#else
    return false ;
#endif
}

/*******************************************************************************************************************//**
 * @brief   Transmit a character to the console device. The character '\n' is prefixed with '\r'
 * @param   ch      Character to be transmitted to the console device
 * @retval  char    Value of the argument 'ch'
 */
char coPutChar( char ch )
{
    if ('\n' == ch) coTxChar('\r') ;
    coTxChar( ch ) ;
    return ch ;
}

/*******************************************************************************************************************//**
 * @brief  Waits and receives a single character from the console device. The function filters-out all the control
 *         characters, with the esclusion of CARRIAGE-RETURN and BACK-SPACE. The TAB characters are translated into
 *         SPACES. The function does not echo the received character
 * @param  void
 * @retval char The character received from the console device
 */
char coGetChar( void )
{
    char ch ;
    while (1)
    {
        ch = coRxChar() ;
        // Returns any printable char, carriage-return and back-space
        if ((isprint(ch)) || ('\r' == ch) || ('\b' == ch)) return ch ;
        // Translate TAB into SPACE
        if ('\t' == ch) return (' ') ;
    }
}

/******************************************************************************************************************//**
 * @brief  Prints an ANSI-OEM (ASCII) string to the console device. The function translates the '\n' character into the
 *         '\r' '\n' sequence.
 * @param  str      Poiter to a zero-terminated string. If the pointer is NULL, the function doesn't perform any action
 * @retval char *   Value of the argument 'pCh'
 */
char *coPutString( const char *str )
{
    if (NULL != str)
    {
        char ch ;
        const char *pc ;
        for (pc = str ; ('\0' != (ch = *(pc++))) ; ) coPutChar( ch ) ;
    }
    return (char *)str ;
}

/*******************************************************************************************************************//**
 * @brief   Editing of a string through the console device
 * @param   *str        Pointer to the destination buffer of the string. The value NULL is not accepted
 * @param   size        Size of the destination buffer. It must be greater than or equal to 1. The destination buffer
 *                      should store the string and the termination character, so this argument must be set to one
 *                      character more than the maximum string length that the function should handle. If size is
 *                      exactly equal to 1 then the function does not accept characters other than 'new line' and
 *                      generates an empty string
 * @retval  char *      Value of the argument 'str' or NULL in case of errors
 */
char *coEditString( char *str, size_t size )
{
    return EditString((void (*)(char))coTxChar, (char (*)(void))coRxChar, str, size) ;
}

/*******************************************************************************************************************//**
 * @brief  Print a formatted string to the console device
 * @param  *fmt     Format string. The format is like the standard printf format, with the following limitations:
 *                  - Only the %d, %u, %x, %X, %f, %s, %S formats are accepted. The h and l flags are not implemented
 * @param  ...      Variable argument list
 * @retval int      Number of arguments accepted and printed
 */
int coprintf( const char *fmt, ...)
{
    int ret ;
    va_list ap ;
    va_start(ap, fmt) ;
    ret = tiny_vprintf((void (*)(char))coPutChar, fmt, ap) ;
    va_end(ap) ;
    return ret ;
}

/*******************************************************************************************************************//**
 * @brief Ask the operator to enter an unsigned decimal number
 * @param ask       Optional character string representing the prompt to the operator
 * @param pNumber   Pointer to the destination variable
 * @return          true if a valid unsigned decimal number was entered
 */
bool coAskUnsigned(const char *ask, uint32_t *pNumber)
{
    char buf[16] ;
    char *cur ;
    uint32_t Num ;
    if (NULL != ask) coprintf("%s", ask) ;
    if ((NULL != coEditString(cur = buf, sizeof(buf))) &&
        atoul((const char **)&cur, &Num) &&
        ('\0' == skipblank((const char **)&cur)))
    {
        if (NULL != pNumber) *pNumber = Num ;
        return true ;
    }
    return false ;
}

/*******************************************************************************************************************//**
 * @brief Ask the operator to enter a signed decimal number
 * @param ask       Optional character string representing the prompt to the operator
 * @param pNumber   Pointer to the destination variable
 * @return          true if a valid signed decimal number was entered
 */
bool coAskSigned(const char *ask, int32_t *pNumber)
{
    char buf[16] ;
    char *cur ;
    int32_t Num ;
    if (NULL != ask) coprintf("%s", ask) ;
    if ((NULL != coEditString(cur = buf, sizeof(buf))) &&
        atosl((const char **)&cur, &Num) &&
        ('\0' == skipblank((const char **)&cur)))
    {
        if (NULL != pNumber) *pNumber = Num ;
        return true ;
    }
    return false ;
}

/*******************************************************************************************************************//**
 * @brief Ask the operator to enter an hexadecimal number
 * @param ask       Optional character string representing the prompt to the operator
 * @param pNumber   Pointer to the destination variable
 * @return          true if a valid hexadecimal number was entered
 */
bool coAskHex(const char *ask, uint32_t *pNumber)
{
    char buf[16] ;
    char *cur ;
    uint32_t Num ;
    if (NULL != ask) coprintf("%s", ask) ;
    if ((NULL != coEditString(cur = buf, sizeof(buf))) &&
        xtoul((const char **)&cur, &Num) &&
        ('\0' == skipblank((const char **)&cur)))
    {
        if (NULL != pNumber) *pNumber = Num ;
        return true ;
    }
    return false ;
}

/*******************************************************************************************************************//**
 * @brief   Lock the console device to the caller task. The other tasks are prevented to lock the console until the
 *          caller doesn't unlock the resource.
 * @param   xTicksToWait    Time, expressed in tick periods, to wait for the console to be unlocked. The value
 *                          WAIT_FOREVER will cause the calling task to wait indefinitely.
 * @return  pdPASS      The console was successfully locked
 *          pdFAIL      It was not possible to lock the console in the time specified
 *          errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY   System error: insufficient memory
 */
BaseType_t coLock( TickType_t xTicksToWait )
{
    // Must create the semaphore if it wasn't already created
    taskENTER_CRITICAL() ;
    if (NULL == coLockSemaphore)
        coLockSemaphore = xSemaphoreCreateMutex() ;
    taskEXIT_CRITICAL() ;
    // Probably the heap is full of dirty mess
    if (NULL == coLockSemaphore) return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY ;
    // The semaphore exists. Try to take it
    return xSemaphoreTake(coLockSemaphore, xTicksToWait) ;
}

/*******************************************************************************************************************//**
 * @brief   Unlock the console device previously locked by the caller task. Other task waiting for this resource now
 *          can lock it
 * @param   void
 * @return  void
 */
void coUnLock( void )
{
    xSemaphoreGive(coLockSemaphore) ;
}

/*******************************************************************************************************************//**
 * @brief  Locked version of function coPutString
 * @param  xTicksToWait    Time, expressed in tick periods, to wait for the console to be unlocked. The value
 *                          WAIT_FOREVER will cause the calling task to wait indefinitely.
 * @param  str    Poiter to a zero-terminated string. If the pointer is NULL, the function doesn't perform any action
 * @return pdPASS The console was successfully locked
 *         pdFAIL It was not possible to lock the console in the time specified
 *         errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY   System error: insufficient memory
 */
BaseType_t coLockedPutString( TickType_t xTicksToWait, const char *str )
{
    BaseType_t result ;
    if (pdPASS == (result = coLock(xTicksToWait)) )
    {
        coPutString( str ) ;
        coUnLock() ;
    }
    return result ;
}

/*******************************************************************************************************************//**
 * @brief  Locked version of function coprintf.
 * @param  xTicksToWait    Time, expressed in tick periods, to wait for the console to be unlocked. The value
 *                          WAIT_FOREVER will cause the calling task to wait indefinitely.
 * @param  *fmt     Format string. The format is like the standard printf format, with the following limitations:
 *                  - Only the %d, %u, %x, %X, %f, %s, %S formats are accepted. The h and l flags are not implemented
 * @param  ...      Variable argument list
 * @return pdPASS The console was successfully locked
 *         pdFAIL It was not possible to lock the console in the time specified
 *         errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY   System error: insufficient memory
 */
BaseType_t coLockedPrintf( TickType_t xTicksToWait, const char *fmt, ... )
{
    BaseType_t result ;
    if (pdPASS == (result = coLock(xTicksToWait)) )
    {
        va_list ap ;
        va_start(ap, fmt) ;
        tiny_vprintf((void (*)(char))coPutChar, fmt, ap) ;
        va_end(ap) ;
        coUnLock() ;
    }
    return result ;
}


/*******************************************************************************************************************//**
 * @brief   Locked version of the function coEditString. ATTENTION: the console deveice will be locked until the manual
 *          editing has terminated.
 * @param   *str        Pointer to the destination buffer of the string. The value NULL is not accepted
 * @param   size        Size of the destination buffer. It must be greater than or equal to 1. The destination buffer
 *                      should store the string and the termination character, so this argument must be set to one
 *                      character more than the maximum string length that the function should handle. If size is
 *                      exactly equal to 1 then the function does not accept characters other than 'new line' and
 *                      generates an empty string
 * @retval  char *      Value of the argument 'str' or NULL in case of errors
 */
char *coLockedEditString( TickType_t xTicksToWait, char *str, size_t size )
{
    if ((NULL != str) && (0 < size))
    {
        if (pdPASS == coLock(xTicksToWait))
        {
            str = EditString((void (*)(char))coTxChar, (char (*)(void))coRxChar, str, size) ;
        }
        else str = NULL ;
        coUnLock() ;
    }
    else str = NULL ;
    return str ;
}


/* END OF FILE ********************************************************************************************************/
