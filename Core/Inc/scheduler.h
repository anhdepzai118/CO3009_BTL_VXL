#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>
#include <stddef.h>

#define SCH_MAX_TASKS   20
#define NO_TASK_ID      0

/* Cấu trúc dữ liệu chuẩn theo mục 4.1 của hướng dẫn BTL */
typedef struct {
    void (*pTask)(void);    // Con trỏ tới hàm task
    uint32_t Delay;         // Số tick còn lại (tương đối so với task đứng trước)
    uint32_t Period;        // Chu kỳ lặp lại (0 = one-shot task)
    uint8_t RunMe;          // Cờ báo task sẵn sàng chạy
    uint32_t TaskID;        // Mã định danh của task
} sTask;

void     SCH_Init(void);
uint32_t SCH_Add_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD);
uint8_t  SCH_Delete_Task(uint32_t taskID);
void     SCH_Update(void);          // Gọi trong ngắt timer 10ms — Độ phức tạp O(1)
void     SCH_Dispatch_Tasks(void);  // Gọi trong vòng lặp while(1)

#endif /* INC_SCHEDULER_H_ */
