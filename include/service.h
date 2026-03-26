#pragma once

void service_init(void);
void service_cleanup(void);

void service_add_card(void);
void service_query_card(void);
void service_start_session(void);
void service_end_session(void);
void service_recharge(void);
void service_refund(void);
void service_query_stats(void);
void service_delete_card(void);
