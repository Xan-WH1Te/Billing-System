#pragma once

#include <stdbool.h>

/** @brief Initialize admin credential storage and runtime state. */
void auth_init(void);
/** @brief Cleanup runtime auth state. */
void auth_cleanup(void);

/** @brief Whether current operator is admin. */
bool auth_is_admin(void);
/** @brief Return current admin name or "-" when not logged in. */
const char* auth_current_admin(void);

/** @brief Perform admin login interaction. */
void auth_login_admin(void);
/** @brief Logout current admin. */
void auth_logout_admin(void);

/** @brief Append one admin audit trail record. */
bool auth_append_audit(const char* action,
                       const char* target_card,
                       const char* before_value,
                       const char* after_value);
