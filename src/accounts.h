#ifndef _ACCOUNTS_H_
#define _ACCOUNTS_H_

struct account_data;

struct account_data *account_load(const char *name);
struct account_data *account_create(const char *name);
int account_save(const struct account_data *account);
void account_free(struct account_data *account);
int account_has_alive_pc(const struct account_data *account);
void account_set_pc(struct account_data *account, const char *pc_name);
void account_clear_pc(struct account_data *account);
void account_refresh_pc(struct account_data *account);

#endif
