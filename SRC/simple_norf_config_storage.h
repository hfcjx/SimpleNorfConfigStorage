/***************************************************************************//**
* <Modul Name>
* Copyright (C), 2025, FCWong.
*  
* @file   simple_norf_config_storage.h
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
#ifndef __SIMPLE_NORF_CONFIG_STORAGE_H__
#define __SIMPLE_NORF_CONFIG_STORAGE_H__

#define     SP_NORF_CFG_BLOCK_MAGIC        0x5AA6C5      /**< 魔数 */

enum  SpNorFCfgStorageRtn
{
    SP_CFG_RTN_ERROR_EMPTY = -3,       /**< 块中没有配置数据 */
    SP_CFG_RTN_ERROR_CFG = -2,         /**< NorFCfgStorageArea_t字段数值错误 A*/
    SP_CFG_RTN_ERROR_NULL = -1,      /**< 指针传参为空 */
    SP_CFG_RTN_ERROR_NONE = 0,    /**< 无异常 */
    SP_CFG_RTN_ERROR_END = 1,    /**< 读取前一个数据到达结尾 */
};
typedef enum  SpNorFCfgStorageRtn  SpNorFCfgStorageRtn_e;


#pragma pack(1)
struct  SpNorFCfgStorageHeader
{
    int       flag : 8;        /**< 标记 */
    int       block_magic : 24;    /**< 魔数 */
    int       sequences;    /**< 顺序序号,每写入一次自增 */
};
typedef struct   SpNorFCfgStorageHeader  SpNorFCfgStorageHeader_t;
#pragma pack()


struct  SpNorFCfgStorageStatus
{
    int    freshest_addr;    /**< 最新数据地址 */
};
typedef struct   SpNorFCfgStorageStatus  SpNorFCfgStorageStatus_t;


typedef     int (*NorFlashRead)(unsigned int addr , void* data , int data_len);
typedef     int (*NorFlashWrite)(unsigned int addr , void* data , int data_len);
typedef     int (*LBlockErase)(unsigned int addr);

struct  SpNorFCfgStorageArea
{
    int    area_start_addr;    /**< 保存区域开始地址 */
    int    lblock_size;    /**< 逻辑块大小 */
    NorFlashRead    fread;    /**< flash读函数 */
    NorFlashWrite   fwrite;    /**< flash写函数 */
    LBlockErase     ferase;    /**< flash擦除函数 */
};
typedef struct  SpNorFCfgStorageArea  SpNorFCfgStorageArea_t;


SpNorFCfgStorageStatus_t    sp_norf_config_storage_area_init(const SpNorFCfgStorageArea_t * p_area);
SpNorFCfgStorageRtn_e sp_norf_config_storage_read(const SpNorFCfgStorageArea_t * p_area,SpNorFCfgStorageStatus_t * p_status,void * buffer,int read_len);
SpNorFCfgStorageRtn_e sp_norf_config_storage_read_previous(const SpNorFCfgStorageArea_t * p_area,SpNorFCfgStorageStatus_t * p_status,void * buffer,int read_len);
SpNorFCfgStorageRtn_e sp_norf_config_storage_write(const SpNorFCfgStorageArea_t * p_area,SpNorFCfgStorageStatus_t * p_status,void * buffer,int write_len);


/**

@mainpage NorFlash 简易配置参数存储库

@author FCWong

@section description 简介

          StorageArea
+--------+--------------------+
| header |    d               |
+--------+    a               |
|             t               | LogicBlock1
|             a               |
|                             |
+--------+--------------------+
| header |    d               |
+--------+    a               |
|             t               | LogicBlock2
|             a               |
|                             |
+-----------------------------+


由上图可见，存储区域(StorageArea)由2个逻辑区(LogicBlock)组成。每个逻辑区开始的地方有一个标头(header)，标头后紧跟着配置参数数据。

逻辑区由1个或多个NorFlash最小擦除扇区组成，每次写入数据前会擦除逻辑分区。

存储区域写满后会覆盖旧逻辑区循环写入。


@section feature 特征

    1. 纯C语言开发，没有其他依赖，没有特定平台代码。
    
    2. 支持掉电保护。

    3. 支持擦写均衡。

    4. 支持数据回溯。


@code
    #include "simple_norf_config_storage.h"

    struct  ConfigData
    {
        short  crc;    
        int    len;    
        int    inc;    
    };
    typedef struct  ConfigData  ConfigData_t;

    NorFCfgStorageStatus_t g_cfg_status;

    //nor flash IO回调函数
    int read(unsigned int addr , void* data , int data_len)
    {
        return 0;
    }
    int write(unsigned int addr , void* data , int data_len)
    {
        return 0;
    }
    int erase(unsigned int addr)
    {
        return 0;
    }

    const NorFCfgStorageArea_t c_cfg_area = 
    {
       .area_start_addr = 0,
       .lblock_size = 40,      
       .fread = read,   
       .fwrite = write,    
       .ferase = erase 
    };

    int write_cfg(ConfigData_t * cfg_data)
    {
        cfg_data->len = sizeof(g_cfg_data);
        cfg_data->crc = crc16(&cfg_data->len,sizeof(ConfigData_t)-sizeof(cfg_data->len));
        sp_norf_config_storage_write(&c_cfg_area,&g_cfg_status,cfg_data,sizeof(g_cfg_data));
    }
    int read_cfg(ConfigData_t * buffer)
    {
        //读取数据
        NorFCfgStorageRtn_e e;
        e = sp_norf_config_storage_read(&c_cfg_area,&g_cfg_status,buffer,sizeof(ConfigData_t));
        if(e == CFG_RTN_ERROR_NONE)
        {
            short  crc;    
            crc = crc16(&buffer->len,sizeof(ConfigData_t)-sizeof(buffer->len));
            if(crc != buffer->crc)
            {
                //数据回溯
                e = norf_config_storage_read_previous(&c_cfg_area,&g_cfg_status,buffer,sizeof(ConfigData_t));
                if(e == CFG_RTN_ERROR_NONE)
                {
                    crc = crc16(&buffer->len,sizeof(ConfigData_t)-sizeof(buffer->len));
                    if(crc != buffer->crc)
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
        return 1;
    }
    
    int main(int argc, char** argv) 
    {
        //初始化保存区域
        g_cfg_status = sp_norf_config_storage_area_init(&c_cfg_area);
        //第1次写入数据
        g_cfg_data.inc ++;
        write_cfg(&g_cfg_data);
        //第2次写入数据
        g_cfg_data.inc ++;
        write_cfg(&g_cfg_data);
        //读取数据
        ConfigData_t  cfg_data;
        int r = read_cfg(&cfg_data);
    }

@endcode


*/









#endif /* ifndef __SIMPLE_NORF_CONFIG_STORAGE_H__.2025-4-25 17:35:26 FCWong*/

