/*
 * Copyright (C) 2017 iCub Facility - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/


// --------------------------------------------------------------------------------------------------------------------
// - public interface
// --------------------------------------------------------------------------------------------------------------------

#include "embot_hw_flash.h"
#include "stm32hal.h"


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include <cstring>
#include <vector>

using namespace std;


// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------


namespace embot { namespace hw { namespace flash {
      
    
#if     defined(STM32HAL_BOARD_NUCLEO64)
    
    const std::uint32_t startOfFLASH            = 0x08000000;
    const std::uint32_t sizeOfFLASH             = 1024*1024;
    const std::uint32_t sizeOfPAGE              = 2*1024;
    const std::uint32_t maxNumOfPAGEs           = 512; 
    
#elif   defined(STM32HAL_BOARD_MTB4)

    const std::uint32_t startOfFLASH            = 0x08000000;
    const std::uint32_t sizeOfFLASH             = 256*1024;
    const std::uint32_t sizeOfPAGE              = 2*1024;
    const std::uint32_t maxNumOfPAGEs           = 128;    
    
#elif   defined(STM32HAL_BOARD_STRAIN2)

    const std::uint32_t startOfFLASH            = 0x08000000;
    const std::uint32_t sizeOfFLASH             = 256*1024;
    const std::uint32_t sizeOfPAGE              = 2*1024;
    const std::uint32_t maxNumOfPAGEs           = 128;     

#elif   defined(STM32HAL_BOARD_RFE)

    const std::uint32_t startOfFLASH            = 0x08000000;
    const std::uint32_t sizeOfFLASH             = 256*1024;
    const std::uint32_t sizeOfPAGE              = 2*1024;
    const std::uint32_t maxNumOfPAGEs           = 128;     

#else
    #error embot::hw::flash::startOfFLASH etc must be defined 
#endif    
    
    bool isaddressvalid(std::uint32_t address)
    {
        if((address >= startOfFLASH) && (address < (startOfFLASH+sizeOfFLASH)))
        {
            return true;
        }
        return false;
    }
    
    std::uint32_t address2page(std::uint32_t address)
    {
        if(false == isaddressvalid(address))
        {
            return maxNumOfPAGEs;
        }
        
        return ((address - startOfFLASH) % sizeOfPAGE);
    }

    
    bool erase(std::uint32_t page)
    {
        if(page >= maxNumOfPAGEs)
        {
            return false;
        }
        
        FLASH_EraseInitTypeDef erase = {0};
        erase.TypeErase = FLASH_TYPEERASE_PAGES;
        erase.Banks = FLASH_BANK_1;
        erase.Page = page;
        erase.NbPages = 1;
        uint32_t pagenum = 0;
        HAL_FLASH_Unlock();
        HAL_StatusTypeDef r = HAL_FLASHEx_Erase(&erase, &pagenum);
        HAL_FLASH_Lock();
        return (HAL_OK == r) ? true : false;                
    }
    
    bool erase(std::uint32_t address, std::uint32_t size)
    {
        // we still erase by page. we need first page and number of them
        if(false == isaddressvalid(address))
        {
            return false;
        }
        if(false == isaddressvalid(address+size))
        {
            return false;
        }
        
        uint32_t firstpage = address2page(address);
        uint32_t lastpage = address2page(address+size-1);
        uint32_t npages = lastpage - firstpage + 1;
        
        for(uint32_t page=firstpage; page<npages; page++)
        {
            if(false == erase(page))
            {
                return false;
            }
        }
        
        return true;
    }
    
    
    bool read(std::uint32_t address, std::uint32_t size, void *data)
    {        
        if(false == isaddressvalid(address))
        {
            return false;
        }
        if(false == isaddressvalid(address+size))
        {
            return false;
        }
        
        if(NULL == data)
        {
            return false;
        }
        
        // can read directly from flash
        void *st = (void*) address;
        std::memmove(data, st, size); 
        
        return true;
    }
    
    bool write(std::uint32_t address, std::uint32_t size, const void *data)
    {
        if(false == isaddressvalid(address))
        {
            return false;
        }
        if(false == isaddressvalid(address+size))
        {
            return false;
        }
        
        if(NULL == data)
        {
            return false;
        }
        
        if(0 != (address & 0x00000007))
        {   // address must be 8-aligned. size must be multiple of 8.
            return false;
        }

        
        HAL_FLASH_Unlock();
    
        uint32_t tmpadr = address;
        uint32_t n64bitwords = sizeOfPAGE / 8;
        const uint64_t *buffer = reinterpret_cast<const std::uint64_t*>(data);
        for(uint32_t i=0; i<n64bitwords; i++)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, tmpadr, buffer[i]);
            tmpadr += 8;
        }

        volatile uint32_t r =  HAL_FLASH_GetError();
        
        HAL_FLASH_Lock();
        
        return (HAL_FLASH_ERROR_NONE == r) ? true : false;
    }        
    
    
    
    
}}} // namespace embot { namespace hw { namespace flash {
    

// - end-of-file (leave a blank line after)----------------------------------------------------------------------------

