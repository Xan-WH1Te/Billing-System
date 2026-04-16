#pragma once

/** @brief Initialize all business services and load persisted state. */
void service_init(void);
/** @brief Release resources owned by all business services. */
void service_cleanup(void);

/** @brief Register a new card. */
void service_add_card(void);
/** @brief Query card information. */
void service_query_card(void);
/** @brief Start a usage session for a card. */
void service_start_session(void);
/** @brief End a usage session and settle charge. */
void service_end_session(void);
/** @brief Recharge card balance. */
void service_recharge(void);
/** @brief Refund part of card balance. */
void service_refund(void);
/** @brief Query statistics from transaction records. */
void service_query_stats(void);
/** @brief Logical delete (deactivate) a card. */
void service_delete_card(void);

/** @brief Return whether admin is currently logged in. */
int service_is_admin_logged_in(void);
/** @brief Log in as admin. */
void service_admin_login(void);
/** @brief Log out current admin. */
void service_admin_logout(void);
/** @brief Edit card as admin (requires login). */
void service_admin_edit_card(void);
/** @brief Enter admin portal loop. */
void service_admin_portal(void);
