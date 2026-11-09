#include "scheduler.h"

static sTask    SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t SCH_count = 0;
static uint32_t next_task_id = 1;

/* Hàm nội bộ chèn task vào danh sách sắp xếp theo Delay tương đối (Delta List) */
static uint32_t SCH_Insert_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD, uint32_t taskID)
{
    if (SCH_count >= SCH_MAX_TASKS || pFunc == NULL) {
        return NO_TASK_ID;
    }

    uint32_t idx = 0;
    /* Tìm vị trí chèn và trừ dần Delay của các task đứng trước */
    while (idx < SCH_count && DELAY >= SCH_tasks_G[idx].Delay) {
        DELAY -= SCH_tasks_G[idx].Delay;
        idx++;
    }

    /* Dịch các phần tử từ vị trí idx sang phải 1 ô */
    for (uint32_t j = SCH_count; j > idx; j--) {
        SCH_tasks_G[j] = SCH_tasks_G[j - 1];
    }

    /* Ghi task mới vào vị trí idx */
    SCH_tasks_G[idx].pTask  = pFunc;
    SCH_tasks_G[idx].Delay  = DELAY;
    SCH_tasks_G[idx].Period = PERIOD;
    SCH_tasks_G[idx].RunMe  = (DELAY == 0) ? 1 : 0;
    SCH_tasks_G[idx].TaskID = taskID;

    SCH_count++;

    /* Cập nhật lại Delay tương đối của task đứng ngay phía sau (nếu có) */
    if (idx + 1 < SCH_count) {
        SCH_tasks_G[idx + 1].Delay -= DELAY;
    }

    return taskID;
}

void SCH_Init(void)
{
    SCH_count = 0;
    next_task_id = 1;
    for (uint32_t i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask  = NULL;
        SCH_tasks_G[i].Delay  = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe  = 0;
        SCH_tasks_G[i].TaskID = NO_TASK_ID;
    }
}

uint32_t SCH_Add_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD)
{
    uint32_t id = next_task_id++;
    if (id == NO_TASK_ID) {
        id = next_task_id++;
    }
    return SCH_Insert_Task(pFunc, DELAY, PERIOD, id);
}

uint8_t SCH_Delete_Task(uint32_t taskID)
{
    if (taskID == NO_TASK_ID || SCH_count == 0) {
        return 0;
    }

    for (uint32_t idx = 0; idx < SCH_count; idx++) {
        if (SCH_tasks_G[idx].TaskID == taskID) {
            /* Cộng trả lại phần Delay của task bị xóa cho task đứng ngay sau nó */
            if (idx + 1 < SCH_count) {
                SCH_tasks_G[idx + 1].Delay += SCH_tasks_G[idx].Delay;
            }

            /* Dịch các task phía sau lên 1 vị trí */
            for (uint32_t j = idx; j < SCH_count - 1; j++) {
                SCH_tasks_G[j] = SCH_tasks_G[j + 1];
            }

            SCH_count--;
            SCH_tasks_G[SCH_count].pTask  = NULL;
            SCH_tasks_G[SCH_count].Delay  = 0;
            SCH_tasks_G[SCH_count].Period = 0;
            SCH_tasks_G[SCH_count].RunMe  = 0;
            SCH_tasks_G[SCH_count].TaskID = NO_TASK_ID;

            if (SCH_count > 0 && SCH_tasks_G[0].Delay == 0) {
                SCH_tasks_G[0].RunMe = 1;
            }
            return 1;
        }
    }
    return 0;
}

/* Hàm SCH_Update bắt buộc đạt độ phức tạp O(1): chỉ xử lý task đầu danh sách */
void SCH_Update(void)
{
    if (SCH_count == 0) return;
    if (SCH_tasks_G[0].Delay > 0) SCH_tasks_G[0].Delay--;
    if (SCH_tasks_G[0].Delay == 0) SCH_tasks_G[0].RunMe = 1;
}

void SCH_Dispatch_Tasks(void)
{
    while (SCH_count > 0 && SCH_tasks_G[0].RunMe > 0) {
        sTask current = SCH_tasks_G[0];

        /* Xóa task đầu khỏi danh sách */
        for (uint32_t j = 0; j < SCH_count - 1; j++) {
            SCH_tasks_G[j] = SCH_tasks_G[j + 1];
        }
        SCH_count--;
        SCH_tasks_G[SCH_count].pTask  = NULL;
        SCH_tasks_G[SCH_count].Delay  = 0;
        SCH_tasks_G[SCH_count].Period = 0;
        SCH_tasks_G[SCH_count].RunMe  = 0;
        SCH_tasks_G[SCH_count].TaskID = NO_TASK_ID;

        /* Nếu task kế tiếp có Delay tương đối == 0 thì kích hoạt chạy cùng tick */
        if (SCH_count > 0 && SCH_tasks_G[0].Delay == 0) {
            SCH_tasks_G[0].RunMe = 1;
        }

        /* Nếu Period > 0 thì thêm lại với Delay = Period; nếu Period == 0 (one-shot) thì tự xóa */
        if (current.Period > 0) {
            SCH_Insert_Task(current.pTask, current.Period, current.Period, current.TaskID);
        }

        /* Thực thi hàm task */
        if (current.pTask != NULL) {
            (*current.pTask)();
        }
    }
}
