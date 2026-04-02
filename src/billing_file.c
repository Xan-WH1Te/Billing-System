#include <billing_file.h>

/* 保存全部账单记录到文件（当前为占位实现）。 */
bool billing_file_save_all(const BillingNode* head, const char* file_path)
{
    (void)head;
    (void)file_path;
    return true;
}

/* 从文件加载全部账单记录（当前为占位实现）。 */
BillingNode* billing_file_load_all(const char* file_path)
{
    (void)file_path;
    return 0;
}
