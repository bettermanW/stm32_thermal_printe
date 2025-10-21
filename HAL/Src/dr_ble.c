
//
// Created by 温厂长 on 2025/9/12.
//
#include  "dr_ble.h"


bool bleConnected = true;  // 蓝牙连接状态
/**
 * @brief 数据包计数器
 * @description 用于统计通过蓝牙接收到的有效数据包数量
 * @note 主要用于：
 *       1. 监控蓝牙数据传输量
 *       2. 调试和数据统计
 *       3. 可能用于流量控制或性能监测
 *       初始值为0，每成功接收一个完整数据包（48字节）递增一次
 */
uint32_t pack_count = 0;


char *ble_in_at_mode = "+++";   //进入AT模式
char *ble_set_status = "AT+STATUS=0\r\n"; //关闭设备状态显示
char *ble_query_status = "AT+STATUS?\r\n"; //关闭设备状态显示
char *ble_query_name = "AT+NAME?\r\n"; //AT+NAME=RF-CRAZY\r\nOK
char *ble_set_name = "AT+NAME=Mini-Printer\r\n"; //OK 大写？
char *ble_out_at_mode = "AT+EXIT\r\n";  //退出

typedef enum {

    BLE_INIT_START, //
    BLE_IN_AT_MODE,
    BLE_IN_AT_MODE_SUCCESS,

    BLE_CLOSE_STATUS,
    BLE_CLOSE_STATUS_SUCCESS,
    BLE_QUERY_STATUS,
    BLE_QUERY_STATUS0_SUCCESS,

    BLE_QUERY_NAME,
    BLE_NEED_SET_NAME,
    BLE_NONEED_SET_NAME,
    BLE_SET_NAME,
    BLE_SET_NAME_SUCCESS,

    BLE_OUT_AT_MODE,
    BLE_INIT_FINISH,
    BLE_RESET,

}e_ble_init_step;

bool need_reboot_ble = false;   //是否需要重启
e_ble_init_step g_ble_init_step = BLE_INIT_START;   //初始化

/**
 * @brief 重置数据包计数器
 * @description 将packcount计数器清零
 * @usage 通常在设备重置、重新连接或开始新的传输会话时调用
 */
void clean_ble_pack_count() {
    pack_count = 0;
}

/**
 * @brief 获取当前数据包计数
 * @return 当前累计的数据包数量
 * @usage 用于查询当前传输进度或统计信息
 */
uint32_t get_ble_pack_count() {
    return pack_count;
}

bool get_ble_connect() {
    return bleConnected;
}

int cmd_index = 0;

/*缓冲区数据
 * 存放接收到的一整条命令或响应（例如 "OK\r\n"、"+CONN\r\n"）
 */
uint8_t cmd_buffer[100];
bool need_clean_ble_status = false;



int retry_count = 0;    // 尝试连接次数
/**
 * @brief 蓝牙模块初始化函数
 * @description 使用状态机机制完成蓝牙模块的AT命令配置流程
 * @note 此函数为阻塞式，会在while循环中执行直到初始化完成
 *       使用了全局变量 g_ble_init_step 来跟踪初始化状态
 */
void init_ble()
{
    /* 无限循环，直到初始化完成（BLE_INIT_FINISH状态） */
    while(1) {
        /* 重试计数器递增，用于统计初始化尝试次数 */
        retry_count++;

        /* 延迟50ms，给蓝牙模块响应时间，避免发送过快 */
        HAL_Delay(50);

        /* 状态1: 初始状态或需要进入AT模式 */
        if(g_ble_init_step == BLE_INIT_START || g_ble_init_step == BLE_IN_AT_MODE) {
            printf("BLE:正进入AT模式\n");
            /* 发送"+++"命令进入AT命令模式 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_in_at_mode, strlen(ble_in_at_mode), 0xffff);
            /* 更新状态为"正在进入AT模式" */
            g_ble_init_step = BLE_IN_AT_MODE;
        }
        /* 状态2: AT模式进入成功或需要关闭状态显示 */
        else if(g_ble_init_step == BLE_IN_AT_MODE_SUCCESS || g_ble_init_step == BLE_CLOSE_STATUS) {
            printf("BLE:正设置status为0 关闭状态显示\n");
            /* 发送AT+STATUS=0命令关闭设备状态显示 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_set_status, strlen(ble_set_status), 0xffff);
            /* 更新状态为"正在关闭状态显示" */
            g_ble_init_step = BLE_CLOSE_STATUS;
        }
        /* 状态3: 状态显示关闭成功或需要查询状态 */
        else if(g_ble_init_step == BLE_CLOSE_STATUS_SUCCESS || g_ble_init_step == BLE_QUERY_STATUS) {
            printf("BLE:正查询状态是否为0\n");
            /* 发送AT+STATUS?命令查询当前状态设置 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_query_status, strlen(ble_query_status), 0xffff);
            /* 更新状态为"正在查询状态" */
            g_ble_init_step = BLE_QUERY_STATUS;
        }
        /* 状态4: 状态查询成功或需要查询设备名称 */
        else if(g_ble_init_step == BLE_QUERY_STATUS0_SUCCESS || g_ble_init_step == BLE_QUERY_NAME) {
            printf("BLE:正查询设备名称\n");
            /* 发送AT+NAME?命令查询当前设备名称 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_query_name, strlen(ble_query_name), 0xffff);
            /* 更新状态为"正在查询设备名称" */
            g_ble_init_step = BLE_QUERY_NAME;
        }
        /* 状态5: 需要设置名称或正在设置名称 */
        else if(g_ble_init_step == BLE_NEED_SET_NAME || g_ble_init_step == BLE_SET_NAME) {
            printf("BLE:正设置设备名称\n");
            /* 发送AT+NAME=Mini-Printer命令设置设备名称 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_set_name, strlen(ble_set_name), 0xffff);
            /* 更新状态为"正在设置设备名称" */
            g_ble_init_step = BLE_SET_NAME;
            /* 设置需要重启标志，因为修改名称后可能需要重启生效 */
            need_reboot_ble = true;
        }
        /* 状态6: 名称设置成功/无需设置名称/需要退出AT模式 */
        else if(g_ble_init_step == BLE_SET_NAME_SUCCESS || g_ble_init_step == BLE_NONEED_SET_NAME || g_ble_init_step == BLE_OUT_AT_MODE) {
            printf("BLE:正退出AT模式\n");
            /* 发送AT+EXIT命令退出AT命令模式 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_out_at_mode, strlen(ble_out_at_mode), 0xffff);
            /* 更新状态为"正在退出AT模式" */
            g_ble_init_step = BLE_OUT_AT_MODE;
        }
        /* 状态7: 初始化完成，退出循环 */
        else if(g_ble_init_step == BLE_INIT_FINISH) {
            /* 跳出while循环，初始化完成 */
            break;
        }
        /* 状态8: 需要重置初始化流程 */
        else if(g_ble_init_step == BLE_RESET) {
            printf("BLE:BLE RESET 退出AT模式\n");
            /* 发送退出AT模式命令，准备重新开始初始化 */
            HAL_UART_Transmit(&huart2, (uint8_t*)ble_out_at_mode, strlen(ble_out_at_mode), 0xffff);
            /* 注意：这里没有更新状态，需要在其他地方处理状态转换 */
        }

        /* 打印当前状态值，用于调试 */
        printf("g_ble_init_step = %d\n", g_ble_init_step);

        /* 运行LED指示灯，显示蓝牙初始化状态 */
        led_run_state(LED_BLE_INIT);
    }

    /* 初始化完成后的处理 */
    if(need_reboot_ble) {
        /* 如果需要重启，提示用户 */
        printf("配置完成-请重启设备使用\n");
    } else {
        /* 如果无需重启，提示正常使用 */
        printf("配置完成-可以正常使用\n");
    }

    /* 延迟1秒，确保所有操作完成 */
    HAL_Delay(1000);

    /* 重置命令缓冲区索引和内容 */
    cmd_index = 0;
    memset(cmd_buffer, 0, sizeof(cmd_buffer));
}

/**
 * @brief 串口数据接收处理函数
 * @description 处理从蓝牙模块接收到的每一个字节数据，根据当前蓝牙初始化状态
 *               分别处理AT命令响应和正常数据通信两种模式。
 * @param data 从串口接收到的单个字节数据
 * @note 此函数通常在串口接收中断回调中调用，需要高效执行
 */
void uart_cmd_handle(uint8_t data) {
    // 将新数据存入缓冲区并更新索引
    cmd_buffer[cmd_index++] = data;
    char *ptr_char = (char*)cmd_buffer;

    /**
     * 模式判断：根据蓝牙初始化状态选择处理逻辑
     * - BLE_INIT_FINISH: 正常工作模式，处理业务数据
     * - 其他状态: AT配置模式，处理AT命令响应
     */
    if (g_ble_init_step == BLE_INIT_FINISH) {
        /******************************
         * 正常工作模式 - 业务数据处理 *
         ******************************/

        /**
         * 蓝牙状态事件处理（CONNECTED/DISCONNECTED等）
         * 这些是蓝牙模块主动上报的状态信息，需要过滤掉
         */
        if (strstr(ptr_char, "CONNECTED") != NULL) {
            need_clean_ble_status = true;  // 标记需要清理状态数据
            led_run_state(LED_CONNECTED);          // 点亮连接指示灯
            bleConnected = true;           // 更新连接状态（建议添加）
        }
        if (strstr(ptr_char, "DISCONNECTED") != NULL) {
            need_clean_ble_status = true;  // 标记需要清理状态数据
            led_run_state(LED_DISCONNECTED);       // 点亮断开指示灯
            bleConnected = false;          // 更新连接状态（建议添加）
        }
        if (strstr(ptr_char, "DEVICE ERROR") != NULL) {
            need_clean_ble_status = true;  // 标记需要清理状态数据
            led_run_state(LED_WARONG);          // 注意：这里可能是LED_WARN更合适
        }

        /**
         * 控制命令解析 - 精确匹配5字节命令帧
         * 格式1: A5 A5 A5 A5 [密度值] - 热敏密度设置命令
         */
        if (cmd_index == 5) {
            // 热敏打印机密度设置命令
            if (cmd_buffer[0] == 0xA5 && cmd_buffer[1] == 0xA5 &&
                cmd_buffer[2] == 0xA5 && cmd_buffer[3] == 0xA5) {

                switch (cmd_buffer[4]) {
                    case 1:  set_heat_density(30); break;  // 低密度
                    case 2:  set_heat_density(60); break;  // 中密度
                    default: set_heat_density(100); break; // 高密度（默认）
                }
                // 清理缓冲区并返回
                cmd_index = 0;
                memset(cmd_buffer, 0, sizeof(cmd_buffer));
                return;
            }

            // 格式2: A6 A6 A6 A6 [未知] - 读取完成命令（可能用于调试）
            if (cmd_buffer[0] == 0xA6 && cmd_buffer[1] == 0xA6 &&
                cmd_buffer[2] == 0xA6 && cmd_buffer[3] == 0xA6) {
                set_read_ble_finish(true);  // 设置读取完成标志
                printf("---->read finish 1 = %ld\n", pack_count); // 调试信息
                cmd_index = 0;
                memset(cmd_buffer, 0, sizeof(cmd_buffer));
                return;
            }
        }

        /**
         * 另一种格式的读取完成命令 - 动态匹配
         * 格式: ... A6 A6 A6 A6 01 - 在数据流中匹配结束标志
         * 这种格式允许命令出现在数据包的任意位置
         */
        if (cmd_index >= 5) {
            if (cmd_buffer[cmd_index - 1] == 0x01) {  // 结束字节
                // 检查前4个字节是否为A6 A6 A6 A6
                if (cmd_buffer[cmd_index - 2] == 0xA6 &&
                    cmd_buffer[cmd_index - 3] == 0xA6 &&
                    cmd_buffer[cmd_index - 4] == 0xA6 &&
                    cmd_buffer[cmd_index - 5] == 0xA6) {

                    printf("---->read finish 2 = %ld\n", pack_count);
                    cmd_index = 0;
                    memset(cmd_buffer, 0, sizeof(cmd_buffer));
                    set_read_ble_finish(true);
                    return;
                }
            }
        }

        /**
         * 打印数据处理 - 48字节数据包
         * 当积累到48字节时，作为完整数据包处理
         */
        if (cmd_index >= 48) {
            pack_count++;  // 数据包计数器递增
            write_to_print_buffer(cmd_buffer, cmd_index);  // 写入打印缓冲区
            cmd_index = 0;  // 重置缓冲区索引
            memset(cmd_buffer, 0, sizeof(cmd_buffer));  // 清空缓冲区
            // printf("packcount = %d\n",packcount); // 调试用，已注释
        }
    } else {
        /******************************
         * AT配置模式 - 命令响应处理 *
         ******************************/

        /**
         * AT命令成功响应处理 - "OK\r\n"
         * 根据当前初始化状态进行状态转换
         */
        // 在字符串 ptr_char 中查找子串 "OK\r\n"
        if (strstr(ptr_char, "OK\r\n") != NULL) {
            // 状态机转换逻辑
            if (g_ble_init_step == BLE_IN_AT_MODE) {
                g_ble_init_step = BLE_IN_AT_MODE_SUCCESS;  // 进入AT模式成功
            } else if (g_ble_init_step == BLE_CLOSE_STATUS) {
                g_ble_init_step = BLE_CLOSE_STATUS_SUCCESS;  // 关闭状态成功
            } else if (g_ble_init_step == BLE_QUERY_NAME) {
                // 检查设备名称，决定是否需要重命名
                if (strstr(ptr_char, "RF-CRAZY") != NULL) {
                    g_ble_init_step = BLE_NEED_SET_NAME;  // 需要设置名称
                } else {
                    g_ble_init_step = BLE_NONEED_SET_NAME;  // 名称正确
                }
            } else if (g_ble_init_step == BLE_SET_NAME) {
                g_ble_init_step = BLE_SET_NAME_SUCCESS;  // 设置名称成功
            } else if (g_ble_init_step == BLE_OUT_AT_MODE) {
                g_ble_init_step = BLE_INIT_FINISH;  // 退出AT模式成功，初始化完成
            } else if (g_ble_init_step == BLE_RESET) {
                g_ble_init_step = BLE_INIT_START;  // 重置后重新开始
            } else if (g_ble_init_step == BLE_QUERY_STATUS) {
                // 检查状态查询结果
                if (strstr(ptr_char, "AT+STATUS=0") != NULL) {
                    g_ble_init_step = BLE_QUERY_STATUS0_SUCCESS;  // 状态已关闭
                } else {
                    g_ble_init_step = BLE_CLOSE_STATUS;  // 需要重新关闭状态
                }
            }

            // 处理完OK响应后清空缓冲区
            cmd_index = 0;
            memset(cmd_buffer, 0, sizeof(cmd_buffer));
            return;
        }

        /**
         * AT命令错误响应处理 - "ERROR\r\n"
         * 发生错误时重置初始化流程
         */
        if (strstr(ptr_char, "ERROR\r\n") != NULL) {
            g_ble_init_step = BLE_RESET;  // 进入重置状态
        }

        /**
         * 缓冲区溢出保护
         * 防止在AT模式下达不到OK/ERROR条件时缓冲区无限增长
         */
        if (cmd_index >= sizeof(cmd_buffer)) {
            cmd_index = 0;  // 重置缓冲区索引
        }
    }
}


/**
 * @brief 在设备和 BLE 模块保持连接时，将设备的运行状态发送出去
 */
void ble_report(){
    if (get_ble_connect()){
        device_state_t *pdevice = get_device_state();
        uint8_t status[4];
        status[0] = pdevice->battery;
        status[1] = pdevice->temperature;
        status[2] = pdevice->paper_state;
        status[3] = pdevice->printer_state;
        HAL_UART_Transmit(&huart2,(uint8_t*)&status,sizeof(status),0xffff);
    }
}

//这步操作是因为厂家的蓝牙模组，现在status只关了busy、connect timeout、device start、wake up
//所以需要把CONNECTED DISCONNECTED DEVICE ERROR这些业务无关数据清掉
void ble_status_data_clean(){
    if(need_clean_ble_status){
        vTaskDelay(200);
        printf("clean --->%s\n",cmd_buffer);
        cmd_index = 0;
        memset(cmd_buffer,0,sizeof(cmd_buffer));
        need_clean_ble_status = false;
    }

}



