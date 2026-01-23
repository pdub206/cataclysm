/**
* @file accounts.h
* Account loading/saving and utility routines.
* 
* This set of code was not originally part of the circlemud distribution.
*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "accounts.h"
#include "toml.h"
#include "toml_utils.h"

static void set_account_name(struct account_data *account, const char *name)
{
  char tmp[MAX_INPUT_LENGTH];

  if (!account || !name || !*name)
    return;

  strlcpy(tmp, name, sizeof(tmp));
  CAP(tmp);

  if (account->name)
    free(account->name);
  account->name = strdup(tmp);
}

static char *toml_dup_string(toml_table_t *tab, const char *key)
{
  toml_datum_t d = toml_string_in(tab, key);

  if (!d.ok)
    return NULL;

  return d.u.s;
}

static void toml_set_account_pc_name(struct account_data *account, const char *value)
{
  char tmp[MAX_INPUT_LENGTH];

  if (!value || !*value)
    return;

  strlcpy(tmp, value, sizeof(tmp));
  CAP(tmp);

  if (account->pc_name)
    free(account->pc_name);
  account->pc_name = strdup(tmp);
}

int account_has_pc(const struct account_data *account, const char *pc_name)
{
  int i;

  if (!account || !pc_name || !*pc_name)
    return 0;

  for (i = 0; i < account->pc_count; i++) {
    if (!str_cmp(account->pc_list[i], pc_name))
      return 1;
  }

  return 0;
}

void account_add_pc(struct account_data *account, const char *pc_name)
{
  char tmp[MAX_INPUT_LENGTH];
  int i;

  if (!account || !pc_name || !*pc_name)
    return;

  strlcpy(tmp, pc_name, sizeof(tmp));
  CAP(tmp);

  if (account_has_pc(account, tmp))
    return;

  i = account->pc_count;
  RECREATE(account->pc_list, char *, account->pc_count + 1);
  account->pc_list[i] = strdup(tmp);
  account->pc_count++;
}

struct account_data *account_create(const char *name)
{
  struct account_data *account;

  CREATE(account, struct account_data, 1);
  set_account_name(account, name);
  return account;
}

struct account_data *account_load(const char *name)
{
  struct account_data *account;
  FILE *fl;
  char filename[PATH_MAX];
  char errbuf[256];
  toml_table_t *tab = NULL;
  toml_array_t *arr = NULL;
  char *value = NULL;
  int i, count;

  if (!name || !*name)
    return NULL;

  if (!get_filename(filename, sizeof(filename), ACCT_FILE, name))
    return NULL;

  fl = fopen(filename, "r");
  if (!fl)
    return NULL;

  tab = toml_parse_file(fl, errbuf, sizeof(errbuf));
  fclose(fl);
  if (!tab) {
    log("SYSERR: Couldn't parse account file %s: %s.", filename, errbuf);
    return NULL;
  }

  account = account_create(NULL);

  value = toml_dup_string(tab, "name");
  if (value) {
    set_account_name(account, value);
    free(value);
  }

  value = toml_dup_string(tab, "password");
  if (value) {
    strlcpy(account->passwd, value, sizeof(account->passwd));
    free(value);
  }

  value = toml_dup_string(tab, "email");
  if (value) {
    if (account->email)
      free(account->email);
    account->email = value;
  }

  value = toml_dup_string(tab, "current_pc");
  if (value) {
    toml_set_account_pc_name(account, value);
    free(value);
  }

  arr = toml_array_in(tab, "pcs");
  if (arr) {
    count = toml_array_nelem(arr);
    for (i = 0; i < count; i++) {
      toml_datum_t d = toml_string_at(arr, i);
      if (!d.ok) {
        log("SYSERR: Invalid account character entry %d in %s.", i, filename);
        continue;
      }
      account_add_pc(account, d.u.s);
      free(d.u.s);
    }
  }

  toml_free(tab);

  if (!account->name)
    set_account_name(account, name);

  if (account->pc_name && account->pc_count == 0)
    account_add_pc(account, account->pc_name);

  if (!account->pc_name && account->pc_count > 0) {
    for (i = account->pc_count - 1; i >= 0; i--) {
      int pfilepos = get_ptable_by_name(account->pc_list[i]);
      if (pfilepos >= 0 && !IS_SET(player_table[pfilepos].flags, PINDEX_DELETED)) {
        account->pc_name = strdup(account->pc_list[i]);
        break;
      }
    }
  }

  return account;
}

int account_save(const struct account_data *account)
{
  FILE *fl;
  char filename[PATH_MAX];
  int i;

  if (!account || !account->name || !*account->name)
    return 0;

  if (!get_filename(filename, sizeof(filename), ACCT_FILE, account->name))
    return 0;

  if (!(fl = fopen(filename, "w"))) {
    log("SYSERR: Couldn't open account file %s for write.", filename);
    return 0;
  }

  fprintf(fl, "name = ");
  toml_write_string(fl, account->name);
  fprintf(fl, "\n");
  fprintf(fl, "password = ");
  toml_write_string(fl, account->passwd);
  fprintf(fl, "\n");
  if (account->email && *account->email)
  {
    fprintf(fl, "email = ");
    toml_write_string(fl, account->email);
    fprintf(fl, "\n");
  }
  if (account->pc_name && *account->pc_name)
  {
    fprintf(fl, "current_pc = ");
    toml_write_string(fl, account->pc_name);
    fprintf(fl, "\n");
  }
  if (account->pc_count > 0) {
    fprintf(fl, "pcs = [\n");
    for (i = 0; i < account->pc_count; i++) {
      fprintf(fl, "  ");
      toml_write_string(fl, account->pc_list[i]);
      fprintf(fl, "%s\n", (i + 1 < account->pc_count) ? "," : "");
    }
    fprintf(fl, "]\n");
  }

  fclose(fl);
  return 1;
}

void account_free(struct account_data *account)
{
  int i;

  if (!account)
    return;

  if (account->name)
    free(account->name);
  if (account->email)
    free(account->email);
  if (account->pc_name)
    free(account->pc_name);
  if (account->pc_list) {
    for (i = 0; i < account->pc_count; i++)
      free(account->pc_list[i]);
    free(account->pc_list);
  }

  free(account);
}

int account_has_alive_pc(const struct account_data *account)
{
  int pfilepos;

  if (!account || !account->pc_name || !*account->pc_name)
    return 0;

  pfilepos = get_ptable_by_name(account->pc_name);
  if (pfilepos < 0)
    return 0;
  if (IS_SET(player_table[pfilepos].flags, PINDEX_DELETED))
    return 0;

  return 1;
}

void account_set_pc(struct account_data *account, const char *pc_name)
{
  char tmp[MAX_INPUT_LENGTH];

  if (!account)
    return;

  if (account->pc_name)
    free(account->pc_name);
  account->pc_name = NULL;

  if (pc_name && *pc_name) {
    strlcpy(tmp, pc_name, sizeof(tmp));
    CAP(tmp);
    account->pc_name = strdup(tmp);
    account_add_pc(account, tmp);
  }
}

void account_clear_pc(struct account_data *account)
{
  if (!account)
    return;

  if (account->pc_name)
    free(account->pc_name);
  account->pc_name = NULL;
}

void account_refresh_pc(struct account_data *account)
{
  int i;

  if (!account || !account->pc_name || !*account->pc_name)
    goto ensure_active;

  if (account_has_alive_pc(account))
    return;

  account_clear_pc(account);
  account_save(account);

ensure_active:
  if (!account || account->pc_name || account->pc_count == 0)
    return;

  for (i = account->pc_count - 1; i >= 0; i--) {
    int pfilepos = get_ptable_by_name(account->pc_list[i]);
    if (pfilepos >= 0 && !IS_SET(player_table[pfilepos].flags, PINDEX_DELETED)) {
      account->pc_name = strdup(account->pc_list[i]);
      account_save(account);
      break;
    }
  }
}
