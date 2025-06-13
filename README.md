## 简介

```
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
```


由上图可见，存储区域(StorageArea)由2个逻辑区(LogicBlock)组成。每个逻辑区开始的地方有一个标头(header)，标头后紧跟着配置参数数据。

逻辑区由1个或多个NorFlash最小擦除扇区组成，每次写入数据前会擦除逻辑分区。

存储区域写满后会覆盖旧逻辑区循环写入。



## 特征

1. 纯C语言开发，没有其他依赖，没有特定平台代码。
2. 支持掉电保护。
3. 支持擦写均衡。
4. 支持数据备份回溯。



## 例子

```c
#include "simple_norf_config_storage.h"
//需要写入的配置数据
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

//定义nor flash的地址扇区及读写IO
const NorFCfgStorageArea_t c_cfg_area = 
{
   .area_start_addr = 0,
   .lblock_size = 40,      
   .fread = read,   
   .fwrite = write,    
   .ferase = erase 
};

//写入配置参数函数
int write_cfg(ConfigData_t * cfg_data)
{
    cfg_data->len = sizeof(ConfigData_t);
    cfg_data->crc = crc16(&cfg_data->len,sizeof(ConfigData_t)-sizeof(cfg_data->len));
    sp_norf_config_storage_write(&c_cfg_area,&g_cfg_status,cfg_data,sizeof(ConfigData_t));
}
//读取配置参数函数
int read_cfg(ConfigData_t * buffer)
{
    //读取数据
    NorFCfgStorageRtn_e e;
    e = sp_norf_config_storage_read(&c_cfg_area,&g_cfg_status,buffer,sizeof(ConfigData_t));
    if(e == CFG_RTN_ERROR_NONE)
    {
        short  crc;  
        //检查数据完整性及正确性
        crc = crc16(&buffer->len,sizeof(ConfigData_t)-sizeof(buffer->len));
        if(crc != buffer->crc)
        {
            //读取备份数据
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
    ConfigData_t  cfg_data;
    //初始化保存区域
    g_cfg_status = sp_norf_config_storage_area_init(&c_cfg_area);
    //第1次写入数据
    cfg_data.inc ++;
    write_cfg(&cfg_data);
    //第2次写入数据
    cfg_data.inc ++;
    write_cfg(&cfg_data);
    //第N次写入数据,数据在StorageArea中循环覆盖
    //cfg_data.inc ++;
    //write_cfg(&cfg_data);
    //读取数据
    memset(&cfg_data,0,sizeof(ConfigData_t));
    int r = read_cfg(&cfg_data);
    if(r == 0)
    {
        printf("%d",cfg_data.inc);
    }
}
```


