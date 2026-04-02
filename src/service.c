#include <service.h>
#include <card_service.h>

/* 初始化各业务子服务。 */
void service_init(void)
{
    card_service_load();
}

/* 统一释放各业务子服务资源。 */
void service_cleanup(void)
{
    card_service_free();
}

/* 以下函数作为菜单层到卡业务层的转发门面。 */
void service_add_card(void)
{
    card_service_add_card();
}

void service_query_card(void)
{
    card_service_query_card();
}

void service_start_session(void)
{
    card_service_start_session();
}

void service_end_session(void)
{
    card_service_end_session();
}

void service_recharge(void)
{
    card_service_recharge();
}

void service_refund(void)
{
    card_service_refund();
}

void service_query_stats(void)
{
    card_service_query_stats();
}

void service_delete_card(void)
{
    card_service_delete_card();
}
