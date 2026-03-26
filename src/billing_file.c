#include <billing_file.h>

bool billing_file_save_all(const BillingNode* head, const char* file_path)
{
    (void)head;
    (void)file_path;
    return true;
}

BillingNode* billing_file_load_all(const char* file_path)
{
    (void)file_path;
    return 0;
}
