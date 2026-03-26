#include <service.h>
#include <billing_service.h>
#include <card_service.h>

void service_init(void)
{
    billing_service_init();
    card_service_load();
}

void service_cleanup(void)
{
    card_service_free();
    billing_service_cleanup();
}

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
