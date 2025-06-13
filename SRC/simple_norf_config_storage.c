/***************************************************************************//**
* <Modul Name>
* Copyright (C), 2025, FCWong.
*  
* @file   simple_norf_config_storage.c
* @brief   
* @details 
* @author FCWong
* @version 
* @date 2025-4-25
*  
* 修改历史 :        
*|日期|版本|修改者|修改描述|
*|:----------|:--------|:---------|:------------|
* 2025-4-25 | V | FCWong | 创建文档
*  
*******************************************************************************/
#include "simple_norf_config_storage.h"

#define     flash_addr(p_area,addr)    (addr) + (p_area)->area_start_addr
#define     config_data_flash_addr(p_area,addr)   (addr) + (p_area)->area_start_addr + sizeof(SpNorFCfgStorageHeader_t)


static SpNorFCfgStorageStatus_t     sp_norf_config_area_scan(const SpNorFCfgStorageArea_t * p_area)
{
    SpNorFCfgStorageStatus_t status ;
    SpNorFCfgStorageHeader_t header;
    int  next_seq = -1;
    status.freshest_addr = p_area->lblock_size;
    //读第2个逻辑区
    p_area->fread(flash_addr(p_area,status.freshest_addr),&header,sizeof(SpNorFCfgStorageHeader_t));
    if(header.block_magic == SP_NORF_CFG_BLOCK_MAGIC)
    {
        if(header.flag == 0)  
        {
            //无异常，有效数据
            if((header.sequences > 0) && (header.sequences % 2) == 1)    //检查sequences正确性
            {
                next_seq = header.sequences;
            }
        }
    }
    //读第1个逻辑区
    status.freshest_addr -= p_area->lblock_size;
    p_area->fread(flash_addr(p_area,status.freshest_addr),&header,sizeof(SpNorFCfgStorageHeader_t));
    if(header.block_magic == SP_NORF_CFG_BLOCK_MAGIC)
    {
        if(header.flag)  //数据无效写入
        {
            invalid:
            if(next_seq >= 0)    //第2逻辑区数据有效
            {
                status.freshest_addr = p_area->lblock_size;
            }
        }
        else
        {
            //无异常，有效数据
            if((header.sequences > 0) && (header.sequences % 2) == 0)    //检查sequences正确性
            {
                if(next_seq > 0)    //第2逻辑区数据有效
                {
                    if(next_seq > header.sequences) //第2逻辑区数据最新
                    {
                        status.freshest_addr = p_area->lblock_size;
                    }
                }
            }
            else
            {
                goto invalid;
            }
        }
    }
    return status;
}

SpNorFCfgStorageStatus_t    sp_norf_config_storage_area_init(const SpNorFCfgStorageArea_t * p_area)
{
    SpNorFCfgStorageStatus_t status = {-1,};
    if(p_area == 0) return status;
    if((p_area->area_start_addr < 0) || (p_area->lblock_size <= 0)) return status;
    if(p_area->fread == 0) return status;
    if(p_area->fwrite == 0) return status;
    if(p_area->ferase == 0) return status;
    return sp_norf_config_area_scan(p_area);
}

SpNorFCfgStorageRtn_e sp_norf_config_storage_read(const SpNorFCfgStorageArea_t * p_area,SpNorFCfgStorageStatus_t * p_status,void * buffer,int read_len)
{
    SpNorFCfgStorageHeader_t freshest_header;
    SpNorFCfgStorageStatus_t status = {0,};
    if(p_area == 0) return SP_CFG_RTN_ERROR_NULL;
    if((p_area->area_start_addr < 0) ||  (p_area->lblock_size <= 0)) return SP_CFG_RTN_ERROR_CFG;
    if(p_area->fread == 0) return SP_CFG_RTN_ERROR_NULL;
    if(buffer == 0) return SP_CFG_RTN_ERROR_NULL;
    if(p_status == 0)
    {
        status = sp_norf_config_area_scan(p_area);
        p_status = &status;
    }
    p_area->fread(flash_addr(p_area,p_status->freshest_addr),&freshest_header,sizeof(SpNorFCfgStorageHeader_t));
    if((freshest_header.flag)  || (freshest_header.block_magic != SP_NORF_CFG_BLOCK_MAGIC))return SP_CFG_RTN_ERROR_EMPTY;
    p_area->fread(config_data_flash_addr(p_area,p_status->freshest_addr),buffer,read_len);
    return SP_CFG_RTN_ERROR_NONE;
    
}

SpNorFCfgStorageRtn_e sp_norf_config_storage_read_previous(const SpNorFCfgStorageArea_t * p_area,SpNorFCfgStorageStatus_t * p_status,void * buffer,int read_len)
{
    int freshest_addr;
    SpNorFCfgStorageHeader_t freshest_header;
    SpNorFCfgStorageStatus_t status = {0,};
    if(p_area == 0) return SP_CFG_RTN_ERROR_NULL;
    if((p_area->area_start_addr < 0) ||  (p_area->lblock_size <= 0)) return SP_CFG_RTN_ERROR_CFG;
    if(p_area->fread == 0) return SP_CFG_RTN_ERROR_NULL;
    if(buffer == 0) return SP_CFG_RTN_ERROR_NULL;
    if(p_status == 0) 
    {
        status = sp_norf_config_area_scan(p_area);
        p_status = &status;
        freshest_addr = p_status->freshest_addr;
    }
    else
    {
        freshest_addr = p_status->freshest_addr - p_area->lblock_size;
        if(freshest_addr < 0)
        {
            freshest_addr =  p_area->lblock_size;
        }
        if(p_status->freshest_addr == freshest_addr) return SP_CFG_RTN_ERROR_END;
    }
    p_area->fread(flash_addr(p_area,freshest_addr),&freshest_header,sizeof(SpNorFCfgStorageHeader_t));
    if((freshest_header.flag)  || (freshest_header.block_magic != SP_NORF_CFG_BLOCK_MAGIC)) return SP_CFG_RTN_ERROR_END;
    p_status->freshest_addr =   freshest_addr; 
    p_area->fread(config_data_flash_addr(p_area,freshest_addr),buffer,read_len);
    return SP_CFG_RTN_ERROR_NONE;
}


SpNorFCfgStorageRtn_e sp_norf_config_storage_write(const SpNorFCfgStorageArea_t * p_area,SpNorFCfgStorageStatus_t * p_status,void * buffer,int write_len)
{
    int end_addr;
    SpNorFCfgStorageHeader_t end_header;
    SpNorFCfgStorageHeader_t fre_header;
    SpNorFCfgStorageStatus_t status = {0,};
    if(p_area == 0) return SP_CFG_RTN_ERROR_NULL;
    if((p_area->area_start_addr < 0) || (p_area->lblock_size <= 0)) return SP_CFG_RTN_ERROR_CFG;
    if(p_area->fwrite == 0) return SP_CFG_RTN_ERROR_NULL;
    if(p_area->ferase == 0) return SP_CFG_RTN_ERROR_NULL;
    if(buffer == 0) return SP_CFG_RTN_ERROR_NULL;
    if(p_status == 0) 
    {
        status = sp_norf_config_area_scan(p_area);
        p_status = &status;
    }
    p_area->fread(flash_addr(p_area,p_status->freshest_addr),&fre_header,sizeof(SpNorFCfgStorageHeader_t));
    if((fre_header.block_magic != SP_NORF_CFG_BLOCK_MAGIC)  || (fre_header.flag)) 
    {
        fre_header.sequences = -1;
        end_addr = p_status->freshest_addr;
        end_header = fre_header;
    }
    else
    {
        end_addr = p_status->freshest_addr + p_area->lblock_size;
        if(end_addr >= (p_area->lblock_size << 1))
        {
            end_addr = 0;
        }
        p_area->fread(flash_addr(p_area,end_addr),&end_header,sizeof(SpNorFCfgStorageHeader_t));
    }
    if(end_header.block_magic == SP_NORF_CFG_BLOCK_MAGIC)    //准备好
    {
        if(end_header.sequences == -1)    
        {
            end_header.flag = -1;
            end_header.sequences = (fre_header.sequences + 1 ) < 0 ? 0 : fre_header.sequences + 1;
            p_area->fwrite(flash_addr(p_area,end_addr),&end_header,sizeof(SpNorFCfgStorageHeader_t));
            end_header.flag = 0;
            p_area->fwrite(flash_addr(p_area,end_addr),&end_header,sizeof(char));
        }
        else
        {
            goto erase;
        }
    }
    else
    {
        erase:
        p_area->ferase(flash_addr(p_area,end_addr));
        end_header.flag = -1;
        end_header.block_magic = SP_NORF_CFG_BLOCK_MAGIC;
        end_header.sequences = (fre_header.sequences + 1 ) < 0 ? 0 : fre_header.sequences + 1;
        p_area->fwrite(flash_addr(p_area,end_addr),&end_header,sizeof(SpNorFCfgStorageHeader_t));
        end_header.flag = 0;
        p_area->fwrite(flash_addr(p_area,end_addr),&end_header,sizeof(char));
    }
    p_area->fwrite(config_data_flash_addr(p_area,end_addr),buffer,write_len);
    p_status->freshest_addr =   end_addr; 
    return SP_CFG_RTN_ERROR_NONE;
}





