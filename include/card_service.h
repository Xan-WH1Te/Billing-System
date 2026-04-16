#pragma once

/** @brief Load card list and billing rule from persistence. */
void card_service_load(void);
/** @brief Free all in-memory card resources. */
void card_service_free(void);

/** @brief Add a card. */
void card_service_add_card(void);
/** @brief Query card information. */
void card_service_query_card(void);
/** @brief Start card session. */
void card_service_start_session(void);
/** @brief End card session and settle billing. */
void card_service_end_session(void);
/** @brief Recharge card. */
void card_service_recharge(void);
/** @brief Refund card balance. */
void card_service_refund(void);
/** @brief Query card usage and revenue statistics. */
void card_service_query_stats(void);
/** @brief Logical delete card. */
void card_service_delete_card(void);
/** @brief Admin edit card fields directly. */
void card_service_admin_edit_card(void);
/** @brief Admin manage billing pricing rule. */
void card_service_admin_manage_pricing(void);